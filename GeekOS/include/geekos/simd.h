#include <geekos/kthread.h>
#include <geekos/kassert.h>
#include <geekos/screen.h>
#include <geekos/irq.h>
#include <geekos/io.h>
#include <geekos/ktypes.h>
void Init_SimD(void);
int Start_IO(struct Kernel_Thread * kt, int block_no, ulong_t user_ptr);
