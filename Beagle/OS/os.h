#ifndef OS_H
#define OS_H

#include <stdint.h>

// Watchdog registers
#define WDT_BASE   0x44E35000
#define WDT_WSPR   (WDT_BASE + 0x48)
#define WDT_WWPS   (WDT_BASE + 0x34)

// Number of processes (OS + P1 + P2 + P3)
#define MAX_PROCESSES      4
#define FIRST_USER_PID     1
#define RR_QUANTUM_TICKS   1

// Process IDs
#define PID_OS             0
#define PID_P1             1
#define PID_P2             2
#define PID_P3             3

// Memory addresses for process stacks (from README)
#define OS_STACK_TOP       0x82010000
#define P1_STACK_TOP       0x82110000
#define P2_STACK_TOP       0x82210000
#define P3_STACK_TOP       0x82310000

// Entry points for user processes
#define P1_ENTRY           0x82100000
#define P2_ENTRY           0x82200000
#define P3_ENTRY           0x82300000

#define P1_MEM_START       0x82100000
#define P1_MEM_END         0x82110000
#define P2_MEM_START       0x82200000
#define P2_MEM_END         0x82210000
#define P3_MEM_START       0x82300000
#define P3_MEM_END         0x82310000

#define USER_MODE_CPSR     0x10

/* * CRÍTICA 2: El documento dice "opcional", pero el estado NO es opcional 
 * si quieres un planificador (scheduler) decente. Si no sabes qué procesos 
 * están listos y cuáles bloqueados, tu Round-Robin colapsará en prácticas futuras.
 */
typedef enum {
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED
} ProcessState;

typedef struct {
    uint32_t pid;
    ProcessState state;
    uint32_t sp;         // Puntero a la pila donde está todo el contexto
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
// Prototipos de funciones en Assembly (root.s)
void start_process_asm(uint32_t sp);
void enable_irq(void);

// Prototipos del Timer y OS
void timer_init(uint32_t ms);
void timer_irq_handler(void);

#endif // OS_H
