/* -*- Mode: C; tab-width: 8; c-basic-offset: 8; indent-tabs-mode: t; -*- */

#include "rrutil.h"

int main(int argc, char *argv[]) {
	long pagesize = sysconf(_SC_PAGESIZE);
	long ncpus = sysconf(_SC_NPROCESSORS_ONLN);

	atomic_printf("sysconf says page size is %ld bytes\n", pagesize);
	test_assert(4096 == pagesize);

	atomic_printf("sysconf says %ld processors are online\n", ncpus);
	/* TODO: change this when rr supports parallel recording. */
	test_assert(1 == ncpus);

	atomic_puts("EXIT-SUCCESS");
	return 0;
}
