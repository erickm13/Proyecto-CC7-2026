#ifndef OS_H
#define OS_H

#include <stdint.h>

// Number of processes (OS + P1 + P2)
#define MAX_PROCESSES      3

// Process IDs
#define PID_OS             0
#define PID_P1             1
#define PID_P2             2

// Memoria alineada correctamente (múltiplos de 4 bytes)
#define OS_STACK_TOP       0x00010000
#define P1_STACK_TOP       0x00300000
#define P2_STACK_TOP       0x00400000

// Entry points for user processes (code start addresses)
#define P1_ENTRY           0x00200000
#define P2_ENTRY           0x00300000

typedef enum {
    READY,
    RUNNING,
    BLOCKED
} ProcessState;

typedef struct {
    uint32_t pid;
    ProcessState state;
    uint32_t sp;         // Stack Pointer
    uint32_t pc;         // Program Counter
    uint32_t lr;         // Link Register
    uint32_t spsr;       // Saved Program Status Register
    uint32_t r0_r12[13]; // General-purpose registers R0-R12
} PCB;

// Function prototypes
void watchdog_disable(void);
uint32_t schedule(void);
void create_process(uint32_t index_pcb, uint32_t pid, uint32_t entry_point, uint32_t stack_top);
void first_context_switch(void);
uint32_t c_context_switch(uint32_t current_sp);

// Assembly functions (root.s)
void start_process_asm(uint32_t sp);
void enable_irq(void);

// Timer functions (timer.c)
void timer_init(uint32_t ms);
void timer_irq_handler(void);

// UART helper functions (uart.c)
void uart_putc(char c);
void os_write(const char *s);
void os_write_hex(uint32_t d);

#endif // OS_H
