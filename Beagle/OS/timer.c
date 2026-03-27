#include "timer.h"
#include "../lib/uart.h"

#define TIMER_1MS_TICKS 24000  // 24MHz / 1000ms

void timer_init(uint32_t ms) {
    uint32_t total_ticks = ms * TIMER_1MS_TICKS;
    uint32_t load_value = 0xFFFFFFFF - total_ticks + 1;

    // Reloj de 24MHz
    PUT32(CLKSEL_TIMER2_CLK, 0x1); 

    // Wait for the clock to be ready (optional but recommended)
    while(GET32(CLKSEL_TIMER2_CLK) != 0x1);

    // Enable timer clock
    PUT32(CM_PER_TIMER2_CLKCTRL, 0x2);

    // Unmask IRQ 68 in the interrupt controller
    PUT32(INTC_MIR_CLEAR2, 0x10);

    // Configure interrupt priority (0 = highest priority)
    PUT32(INTC_ILR68, 0x0);

    // Stop the timer
    PUT32(TCLR, 0x0);

    // Clear any pending interrupts
    PUT32(TISR, 0x7);

    // Load timer value for 2 second interval
    PUT32(TLDR, load_value);

    // Set the counter to the same value
    PUT32(TCRR, load_value);

    // Enable overflow interrupt (bit 1)
    PUT32(TIER, 0x2);

    // Start timer in auto-reload mode (bit 0 = start, bit 1 = auto-reload)
    PUT32(TCLR, 0x3);

    os_write("  - Timer started\n");
}

void timer_irq_handler(void) {
    // TODO: Implement timer interrupt handler

    PUT32(TISR, 0x2);
    PUT32(INTC_CONTROL, 0x1);
    os_write("Tick\n");
}