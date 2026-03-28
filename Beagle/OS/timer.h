#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

// BeagleBone Black DMTIMER2 base address
#define DMTIMER2_BASE    0x48040000
#define TCLR             (DMTIMER2_BASE + 0x38)  // Timer Control Register
#define TCRR             (DMTIMER2_BASE + 0x3C)  // Timer Counter Register
#define TISR             (DMTIMER2_BASE + 0x28)  // Timer Interrupt Status Register
#define TIER             (DMTIMER2_BASE + 0x2C)  // Timer Interrupt Enable Register
#define TLDR             (DMTIMER2_BASE + 0x40)  // Timer Load Register

// Clock Manager base address
#define CM_PER_BASE      0x44E00000
#define CM_PER_TIMER2_CLKCTRL (CM_PER_BASE + 0x80)  // Timer2 Clock Control
#define CONTROL_MODULE_BASE 0x44E10000
#define CLKSEL_TIMER2_CLK   (CONTROL_MODULE_BASE + 0x508)

// INTC registers (defined in os.h, included by timer.c)
// These are here for standalone compilation if needed
#ifndef INTC_MIR_CLEAR2
#define INTCPS_BASE      0x48200000
#define INTC_MIR_CLEAR2  (INTCPS_BASE + 0xC8)
#define INTC_CONTROL     (INTCPS_BASE + 0x48)
#define INTC_ILR68       (INTCPS_BASE + 0x110)
#endif

void timer_init(uint32_t ms);
void timer_irq_handler(void);

#endif