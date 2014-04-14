from rrutil import *

send_gdb('b sighandler\n')
expect_gdb('Breakpoint 1')

send_gdb('c\n')
expect_gdb('Program received signal SIGILL')
expect_gdb('ud2')

send_gdb('c\n')
expect_gdb('Breakpoint 1, sighandler')

ok()
