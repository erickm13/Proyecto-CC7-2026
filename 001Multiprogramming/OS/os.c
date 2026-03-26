#include "os.h"

// BeagleBone Black UART0 base address
#define UART0_BASE     0x44E09000
#define UART_THR       (UART0_BASE + 0x00)  // Transmit Holding Register
#define UART_LSR       (UART0_BASE + 0x14)  // Line Status Register
#define UART_LSR_THRE  0x20                 // Transmit Holding Register Empty

// BeagleBone Black DMTIMER2 base address
#define DMTIMER2_BASE    0x48040000
#define TCLR             (DMTIMER2_BASE + 0x38)  // Timer Control Register
#define TCRR             (DMTIMER2_BASE + 0x3C)  // Timer Counter Register
#define TISR             (DMTIMER2_BASE + 0x28)  // Timer Interrupt Status Register
#define TIER             (DMTIMER2_BASE + 0x2C)  // Timer Interrupt Enable Register
#define TLDR             (DMTIMER2_BASE + 0x40)  // Timer Load Register

// BeagleBone Black Interrupt Controller (INTCPS) base address
#define INTCPS_BASE      0x48200000
#define INTC_MIR_CLEAR2  (INTCPS_BASE + 0xC8)    // Interrupt Mask Clear Register 2
#define INTC_CONTROL     (INTCPS_BASE + 0x48)    // Interrupt Controller Control
#define INTC_ILR68       (INTCPS_BASE + 0x110)   // Interrupt Line Register 68

// Clock Manager base address
#define CM_PER_BASE      0x44E00000
#define CM_PER_TIMER2_CLKCTRL (CM_PER_BASE + 0x80)  // Timer2 Clock Control

// Watchdog Timer 1 (WDT1) base address
#define WDT1_BASE        0x44E35000
#define WSPR             (WDT1_BASE + 0x48)        // Watchdog Start/Stop Register
#define WWPS             (WDT1_BASE + 0x34)        // Watchdog Write Posting Bits

// OS Data and Stack Regions
#define OS_DATA  0x82000000 // SIZE 64KB
#define OS_STACK 0x82010000 // SIZE 8KB

// Process 1
#define P1_DATA  0x82100000 // SIZE 64KB
#define P1_STACK 0x82110000 // SIZE 8KB

// "Process 2" will be replaced by a Tick task
#define P2_STACK 0x82210000 // SIZE 8KB

// Context frame size:
// r0-r12 = 13 regs, lr = 1 reg => 14 * 4 = 56 bytes
#define CONTEXT_SIZE 56


// ============================================================================
// PCB
// ============================================================================

typedef struct {
    unsigned int sp;
} PCB;

static PCB pcb_p1;
static PCB pcb_tick;
static PCB *current_pcb;


// ============================================================================
// Watchdog Functions
// ============================================================================

// Disable watchdog as early as possible
void disable_watchdog(void) {
    PUT32(WSPR, 0xAAAA);
    while (GET32(WWPS) != 0);

    PUT32(WSPR, 0x5555);
    while (GET32(WWPS) != 0);
}


// ============================================================================
// UART Functions
// ============================================================================

// Function to send a single character via UART
void uart_putc(char c) {
    // Wait until Transmit Holding Register is empty
    while ((GET32(UART_LSR) & UART_LSR_THRE) == 0);
    PUT32(UART_THR, c);
}

// Function to send a string via UART
void os_write(const char *s) {
    while (*s) {
        if (*s == '\n') {
            uart_putc('\r');
        }
        uart_putc(*s++);
    }
}


// ============================================================================
// Tick Task
// ============================================================================

void tick_task(void) {
    while (1) {
        os_write("Tick\n");

        for (volatile int i = 0; i < 1000000; i++) {
        }
    }
}


// ============================================================================
// Process / Scheduler Functions
// ============================================================================

static void init_task_stack(PCB *pcb, unsigned int stack_base, unsigned int entry) {
    unsigned int *sp;

    // top of stack (stack grows downward)
    sp = (unsigned int *)(stack_base + 0x2000);

    // reserve space for r0-r12 + lr
    sp = (unsigned int *)((unsigned int)sp - CONTEXT_SIZE);

    // initialize r0-r12 to 0
    for (int i = 0; i < 13; i++) {
        sp[i] = 0;
    }

    // synthetic lr so that "subs pc, lr, #4" lands on entry
    sp[13] = entry + 4;

    pcb->sp = (unsigned int)sp;
}

void process_init(void) {
    init_task_stack(&pcb_p1,   P1_STACK, (unsigned int)P1_DATA);
    init_task_stack(&pcb_tick, P2_STACK, (unsigned int)tick_task);

    current_pcb = &pcb_p1;
}

unsigned int schedule_next_sp(unsigned int current_sp) {
    current_pcb->sp = current_sp;

    if (current_pcb == &pcb_p1) {
        current_pcb = &pcb_tick;
    } else {
        current_pcb = &pcb_p1;
    }

    return current_pcb->sp;
}


// ============================================================================
// Timer Functions
// ============================================================================

// TODO: Implement timer initialization
// This function should:
// 1. Enable the timer clock (CM_PER_TIMER2_CLKCTRL = 0x2)
// 2. Unmask IRQ 68 in the interrupt controller (INTC_MIR_CLEAR2)
// 3. Configure interrupt priority (INTC_ILR68 = 0x0)
// 4. Stop the timer (TCLR = 0)
// 5. Clear any pending interrupts (TISR = 0x7)
// 6. Set the load value for 2 seconds (TLDR = 0xFE91CA00)
// 7. Set the counter to the same value (TCRR = 0xFE91CA00)
// 8. Enable overflow interrupt (TIER = 0x2)
// 9. Start timer in auto-reload mode (TCLR = 0x3)
void timer_init(void) {

    // 1) Enable Timer2 clock
    PUT32(CM_PER_TIMER2_CLKCTRL, 0x2);

    // 2) Unmask IRQ 68 (IRQ68 is bit 4 in MIR_CLEAR2 because 68-64=4)
    PUT32(INTC_MIR_CLEAR2, (1u << 4));

    // 3) Priority/type for IRQ 68 (0 = IRQ, priority 0)
    PUT32(INTC_ILR68, 0x0);

    // 4) Stop timer
    PUT32(TCLR, 0x0);

    // 5) Clear pending interrupts
    PUT32(TISR, 0x7);

    // 6-7) Load value for ~2 seconds @ 24MHz (given by lab)
    PUT32(TLDR, 0xFE91CA00);
    PUT32(TCRR, 0xFE91CA00);

    // 8) Enable overflow interrupt
    PUT32(TIER, 0x2);

    // 9) Start + auto-reload (ST=1, AR=1)
    PUT32(TCLR, 0x3);
}


// TODO: Implement timer interrupt handler
// This function should:
// 1. Clear the timer interrupt flag (TISR = 0x2)
// 2. Acknowledge the interrupt to the controller (INTC_CONTROL = 0x1)
// 3. Perform scheduling
void timer_irq_handler(void) {

    // 1) Clear overflow interrupt flag
    PUT32(TISR, 0x2);

    // 2) Acknowledge to interrupt controller
    PUT32(INTC_CONTROL, 0x1);

    // 3) Real context switch is done in root.s
    // This function only clears the interrupt.
}


// ============================================================================
// Main Program
// ============================================================================

int main(void) {

    // TODO: Print initialization message
    os_write("Starting...\n");

    // Disable watchdog as early as possible
    disable_watchdog();
    os_write("Watchdog disabled\n");

    // Initialize task PCBs
    process_init();
    os_write("Processes initialized\n");

    // TODO: Initialize the timer using timer_init()
    timer_init();
    os_write("Timer initialized\n");

    // TODO: Enable interrupts using enable_irq()
    enable_irq();
    os_write("Interrupts enabled\n");

    // Start first task (Process1)
    start_first_task(pcb_p1.sp);

    // Should never return
    while (1) {
    }

    return 0;
}