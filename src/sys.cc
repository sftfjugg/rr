/* -*- Mode: C++; tab-width: 8; c-basic-offset: 8; indent-tabs-mode: t; -*- */

#include "sys.h"

#include <assert.h>
#include <fcntl.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/personality.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
/* This header has to be included after sys/ptrace.h. */
#include <asm/ptrace-abi.h>
#include <sys/time.h>
#include <sys/wait.h>

#include "dbg.h"
#include "recorder_sched.h"
#include "task.h"
#include "trace.h"
#include "util.h"

static void sys_exit()
{
	log_err("Exiting");
	Task::killall();
	close_trace_files();
	abort();
}

FILE* sys_fopen(const char* path, const char* mode)
{
	FILE* file = fopen(path, mode);

	if (file == NULL) {
		log_err("error while opening file: %s -- bailing out!\n", path);
		sys_exit();
	}

	return file;
}

int sys_open(const char* path, int flags, mode_t mode)
{
	int file = (mode < 0) ? open(path, flags) : open(path, flags, mode);

	if (file < 0) {
		log_err("error while opening file: %s -- bailing out!\n", path);
		sys_exit();
	}

	return file;
}

void sys_fclose(FILE* file)
{
	if (fclose(file) < 0) {
		log_err("error while closing file -- bailing out\n");
		sys_exit();
	}
}

void sys_close(int filedes)
{
	if (close(filedes) < 0) {
		log_err("error while closing file -- bailing out\n");
		sys_exit();
	}
}

/**
 * Prepare this process and its ancestors for recording/replay by
 * preventing direct access to sources of nondeterminism, and ensuring
 * that rr bugs don't adversely affect the underlying system.
 */
static void set_up_process(void)
{
	int orig_pers;

	/* TODO tracees can probably undo some of the setup below
	 * ... */

	/* Disable address space layout randomization, for obvious
	 * reasons, and ensure that the layout is otherwise well-known
	 * ("COMPAT").  For not-understood reasons, "COMPAT" layouts
	 * have been observed in certain recording situations but not
	 * in replay, which causes divergence. */
	if (0 > (orig_pers = personality(0xffffffff))) {
		fatal("error getting personaity");
	}
	if (0 > personality(orig_pers | ADDR_NO_RANDOMIZE |
			    ADDR_COMPAT_LAYOUT)) {
		fatal("error disabling randomization");
	}
	/* Trap to the rr process if a 'rdtsc' instruction is issued.
	 * That allows rr to record the tsc and replay it
	 * deterministically. */
	if (0 > prctl(PR_SET_TSC, PR_TSC_SIGSEGV, 0, 0, 0)) {
		fatal("error setting up prctl -- bailing out");
	}
	/* If the rr process dies, prevent runaway tracee processes
	 * from dragging down the underlying system. */
	if (0 > prctl(PR_SET_PDEATHSIG, SIGKILL)) {
		fatal("Couldn't set parent-death signal");
	}

	/* XXX is it faster to mask off a CPU affinity when this
	 * process is the only intensive one in the system? */
}

static void sys_ptrace_traceme()
{
	ptrace(PTRACE_TRACEME, 0, 0, 0);
}

void sys_start_trace(const char* executable, char** argv, char** envp)
{
	set_up_process();
	sys_ptrace_traceme();

	/* signal the parent that the child is ready */
	kill(getpid(), SIGSTOP);

	execvpe(executable, argv, envp);
	fatal("Failed to exec %s", executable);
}

/*
 * Returns the time difference t2 - t1 in microseconds.
 */
static inline long time_difference(struct timeval *t1, struct timeval *t2) {
	return (t2->tv_sec - t1->tv_sec) * 1000000 + (t2->tv_usec - t1->tv_usec);
}

void sys_fcntl(int fd, int cmd, long arg1)
{
	if (fcntl(fd, cmd, arg1) < 0) {
		fatal("fcntl(%d, %ld) failed", cmd, arg1);
	}
}

int sys_mkdir(const char *path, mode_t mode)
{
    struct stat st;
    int status = 0;

    if (stat(path, &st) != 0) {
        // Directory does not exist. EEXIST for race condition
        if (mkdir(path, mode) != 0 && errno != EEXIST)
            status = -1;
    } else if (!S_ISDIR(st.st_mode)) {
        errno = ENOTDIR;
        status = -1;
    }

    return(status);
}

/**
** sys_mkpath - ensure all directories in path exist
** Algorithm takes the pessimistic view and works top-down to ensure
** each directory in path exists, rather than optimistically creating
** the last element and working backwards.
*/
int sys_mkpath(const char *path, mode_t mode)
{
    char           *pp;
    char           *sp;
    int             status;
    char           *copypath = (char*)malloc(strlen(path) + 1);

    strcpy(copypath,path);
    status = 0;
    pp = copypath;
    while (status == 0 && (sp = strchr(pp, '/')) != 0)
    {
        if (sp != pp)
        {
            /* Neither root nor double slash in path */
            *sp = '\0';
            status = sys_mkdir(copypath, mode);
            *sp = '/';
        }
        pp = sp + 1;
    }

    free(copypath);
    assert(status == 0);
    return status;
}
