from rrutil import *

# XXX this test is racy, because we don't have a way to halt replay
# until some condition is satisfied.  Maybe we should add that.
send_gdb('c\n')
expect_rr('spinning')
interrupt_gdb()

ok()
