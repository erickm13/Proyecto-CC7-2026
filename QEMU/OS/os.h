#ifndef OS_H
#define OS_H

#include <stdint.h>

// Number of processes (OS + P1 + P2)
#define MAX_PROCESSES      4
#define FIRST_USER_PID     1
#define RR_QUANTUM_TICKS   1

// Process IDs
#define PID_OS             0
#define PID_P1             1
#define PID_P2             2
#define PID_P3             3

// Memoria alineada correctamente (múltiplos de 4 bytes)
#define OS_STACK_TOP       0x00010000
#define P1_STACK_TOP       0x00300000
#define P2_STACK_TOP       0x00400000
#define P3_STACK_TOP       0x00500000

// Entry points for user processes (code start addresses)
#define P1_ENTRY           0x00200000
#define P2_ENTRY           0x00300000
#define P3_ENTRY           0x00400000

#define P1_MEM_START       0x00200000
#define P1_MEM_END         0x00300000
#define P2_MEM_START       0x00300000
#define P2_MEM_END         0x00400000
#define P3_MEM_START       0x00400000
#define P3_MEM_END         0x00500000

#define USER_MODE_CPSR     0x10

typedef enum {
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED
} ProcessState;

typedef struct {
    uint32_t pid;            // Process ID
    ProcessState state;      // Current state
    uint32_t sp;             // Stack pointer (points to saved context)
    uint32_t user_sp;
    uint32_t mem_start;
    uint32_t mem_end;
    int32_t exit_code;
    uint32_t fault_type;
} PCB;

// Function prototypes
void watchdog_disable(void);
uint32_t schedule(uint32_t force_switch);
void create_process(uint32_t index_pcb, uint32_t pid, uint32_t entry_point, uint32_t stack_top);
void first_context_switch(void);
uint32_t c_context_switch(uint32_t current_sp, uint32_t user_sp);
uint32_t c_svc_handler(uint32_t current_sp, uint32_t user_sp);
uint32_t c_fault_handler(uint32_t current_sp, uint32_t user_sp, uint32_t fault_type);
uint32_t get_current_user_sp(void);

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
