#include "timer.h"
#include "uart.h"

/* 
 * The VersatilePB SP804 timer usually runs at 1MHz in QEMU.
 * 1MHz = 1,000,000 ticks per second.
 * 1,000,000 / 1000ms = 1000 ticks per ms.
 */
#define TIMER_1MS_TICKS 1000 

extern void PUT32(uint32_t, uint32_t);
extern uint32_t GET32(uint32_t);

void timer_init(uint32_t ms) {
    uint32_t ticks = ms * TIMER_1MS_TICKS;

    // 1. Disable the timer before configuring
    PUT32(TIMER0_CTRL, 0);

    // 2. Load the value (SP804 counts down from this value)
    PUT32(TIMER0_LOAD, ticks);
    PUT32(TIMER0_BGLOAD, ticks);

    // 3. Clear any existing interrupts
    PUT32(TIMER0_INTCLR, 1);

    // 4. Configure VIC (Interrupt Controller)
    // Enable IRQ 4 (Timer 0) by writing to the Enable register
    // In the VIC, writing a '1' to a bit enables that interrupt source
    PUT32(VIC_INTENABLE, (1 << VIC_TIMER0_IRQ));

    /* 5. Start the timer
     * Bits used: 
     * 0x80 (Enable) 
     * 0x40 (Periodic mode) 
     * 0x20 (Interrupt Enable) 
     * 0x02 (32-bit counter)
     */
    uint32_t ctrl = (1 << 7) | (1 << 6) | (1 << 5) | (1 << 1);
    PUT32(TIMER0_CTRL, ctrl);

    os_write("  - VersatilePB Timer started\n");
}

void timer_irq_handler(void) {
    // 1. Clear the timer interrupt (write any value to INTCLR)
    PUT32(TIMER0_INTCLR, 0x1);

    // 2. Acknowledge the interrupt in the VIC
    // Writing to VICVECTADDR is standard for EOI.
    // Some implementations also use VICADDRESS (0xF00).
    PUT32(VIC_VECTADDR, 0);

    os_write("Tick\n");
}
