#include <geekos/simd.h>

#define DEBUG_KEYBOARD(x...)
// #define DEBUG_KEYBOARD(x...) Print("KBD: " x)

/* ----------------------------------------------------------------------
 * Private data and functions
 * ---------------------------------------------------------------------- */
static int s_queueHead_simd, s_queueTail_simd;

/*
 * Wait queue for thread(s) waiting for keyboard events.
 */
static struct Thread_Queue s_waitQueue_simd;





/* ----------------------------------------------------------------------
 * Public functions
 * ---------------------------------------------------------------------- */

void Init_SimD(void) {
    
    s_queueHead_simd = s_queueTail_simd = 0;

    /* Install interrupt handler */
   /* Install_IRQ(KB_IRQ, Keyboard_Interrupt_Handler);

 //   Enable IRQ1 (keyboard) 
    irqMask = Get_IRQ_Mask();
    irqMask &= ~(1 << KB_IRQ);
    Set_IRQ_Mask(irqMask);*/

}


