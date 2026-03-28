#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

/*
 * =============================================================================
 * QEMU versatilepb Timer and Interrupt Controller Definitions
 * =============================================================================
 * 
 * The QEMU versatilepb board uses:
 * 1. SP804 Dual Timer Module - Two independent timers
 * 2. VIC (Vectored Interrupt Controller) - ARM PrimeCell PL190
 * 
 * TIMER0 (used for OS scheduler):
 * - Base Address: 0x101E2000
 * - IRQ Number: 4 (connected to VIC IRQ 4)
 * 
 * VIC (Interrupt Controller):
 * - Base Address: 0x10140000
 * - Manages 32 interrupt sources (IRQ 0-31)
 * 
 * CHANGES FROM BEAGLEBONE VERSION:
 * - Different timer hardware (SP804 vs DMTimer2)
 * - Different interrupt controller (VIC vs OMAP INTC)
 * - Different register layouts and addresses
 * =============================================================================
 */

/* ============================================================================
 * SP804 Timer Registers
 * ============================================================================
 * The SP804 is a dual timer module. We use Timer 0.
 * Each timer is 32-bit and can generate periodic interrupts.
 */
#define TIMER0_BASE     0x101E2000

#define TIMER0_LOAD     (TIMER0_BASE + 0x00)  // Load Register (reload value)
#define TIMER0_VALUE    (TIMER0_BASE + 0x04)  // Current Value Register (read-only)
#define TIMER0_CTRL     (TIMER0_BASE + 0x08)  // Control Register
#define TIMER0_INTCLR   (TIMER0_BASE + 0x0C)  // Interrupt Clear Register
#define TIMER0_RIS      (TIMER0_BASE + 0x10)  // Raw Interrupt Status
#define TIMER0_MIS      (TIMER0_BASE + 0x14)  // Masked Interrupt Status
#define TIMER0_BGLOAD   (TIMER0_BASE + 0x18)  // Background Load Register

/* Timer Control Register bits */
#define TIMER_CTRL_EN       0x80    // Bit 7: Timer enable
#define TIMER_CTRL_PERIODIC 0x40    // Bit 6: Periodic mode (1) vs one-shot (0)
#define TIMER_CTRL_INTEN    0x20    // Bit 5: Interrupt enable
#define TIMER_CTRL_PRESCALE 0x00    // Bits 3-2: Prescaler (00 = 1:1)
#define TIMER_CTRL_32BIT    0x00    // Bit 1: 32-bit mode (0) vs 16-bit (1)
#define TIMER_CTRL_ONESHOT  0x00    // Bit 0: Wrapper mode (0)

/* Timer1 (available but not used in this project) */
#define TIMER1_BASE     0x101E2020
#define TIMER1_LOAD     (TIMER1_BASE + 0x00)
#define TIMER1_VALUE    (TIMER1_BASE + 0x04)
#define TIMER1_CTRL     (TIMER1_BASE + 0x08)
#define TIMER1_INTCLR   (TIMER1_BASE + 0x0C)
#define TIMER1_RIS      (TIMER1_BASE + 0x10)
#define TIMER1_MIS      (TIMER1_BASE + 0x14)
#define TIMER1_BGLOAD   (TIMER1_BASE + 0x18)

/* ============================================================================
 * VIC (Vectored Interrupt Controller) Registers
 * ============================================================================
 * The PL190 VIC manages 32 interrupt sources.
 * Timer0 is connected to IRQ slot 4 on versatilepb.
 */
#define VIC_BASE        0x10140000

#define VIC_IRQSTATUS   (VIC_BASE + 0x000)  // IRQ status (active interrupts)
#define VIC_FIQSTATUS   (VIC_BASE + 0x004)  // FIQ status
#define VIC_RAWINTR     (VIC_BASE + 0x008)  // Raw interrupt status (before masking)
#define VIC_INTSELECT   (VIC_BASE + 0x00C)  // IRQ/FIQ selection (0=IRQ, 1=FIQ)
#define VIC_INTENABLE   (VIC_BASE + 0x010)  // Interrupt Enable Register
#define VIC_INTENCLR    (VIC_BASE + 0x014)  // Interrupt Enable Clear
#define VIC_SOFTINT   (VIC_BASE + 0x018)  // Software Interrupt (force interrupt)
#define VIC_SOFTINTCLR (VIC_BASE + 0x01C)  // Software Interrupt Clear
#define VIC_VECTADDR    (VIC_BASE + 0x030)  // Vector Address Register (acknowledge IRQ)

/* VIC Interrupt Numbers for versatilepb */
#define VIC_TIMER0_IRQ  4       // Timer0 is IRQ 4
#define VIC_TIMER1_IRQ  5       // Timer1 is IRQ 5

/* ============================================================================
 * Helper Macros
 * ============================================================================
 */

// Timer control value for periodic interrupt mode
#define TIMER0_CTRL_PERIODIC_INT (TIMER_CTRL_EN | TIMER_CTRL_PERIODIC | TIMER_CTRL_INTEN | TIMER_CTRL_PRESCALE | TIMER_CTRL_32BIT)

// Function prototypes
void timer_init(uint32_t ms);
void timer_irq_handler(void);

#endif // TIMER_H
