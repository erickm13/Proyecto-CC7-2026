#ifndef OS_H
#define OS_H

#include <stdint.h>

// Watchdog registers
#define WDT_BASE   0x44E35000
#define WDT_WSPR   (WDT_BASE + 0x48)
#define WDT_WWPS   (WDT_BASE + 0x34)

// Number of processes (OS + P1 + P2)
#define MAX_PROCESSES      3

// Process IDs
#define PID_OS             0
#define PID_P1             1
#define PID_P2             2

// Memory addresses for process stacks (from README)
#define OS_STACK_TOP       0x82010000
#define P1_STACK_TOP       0x82110000
#define P2_STACK_TOP       0x82210000

// Entry points for user processes
#define P1_ENTRY           0x82100000
#define P2_ENTRY           0x82200000

/* * CRÍTICA 2: El documento dice "opcional", pero el estado NO es opcional 
 * si quieres un planificador (scheduler) decente. Si no sabes qué procesos 
 * están listos y cuáles bloqueados, tu Round-Robin colapsará en prácticas futuras.
 */
typedef enum {
    READY,
    RUNNING,
    BLOCKED   // Te lo añado desde ahora. Lo necesitarás si un proceso hace I/O.
} ProcessState;

typedef struct {
    uint32_t pid;
    ProcessState state;
    uint32_t sp;         // Puntero a la pila donde está todo el contexto
} PCB;

// Function prototypes
void watchdog_disable(void);
uint32_t schedule(void);
void create_process(uint32_t index_pcb, uint32_t pid, uint32_t entry_point, uint32_t stack_top);
void first_context_switch(void);
uint32_t c_context_switch(uint32_t current_sp);
// Prototipos de funciones en Assembly (root.s)
void start_process_asm(uint32_t sp);
void enable_irq(void);

// Prototipos del Timer y OS
void timer_init(uint32_t ms);
void timer_irq_handler(void);

#endif // OS_H