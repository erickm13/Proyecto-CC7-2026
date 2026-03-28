#include "os.h"
#include "../lib/uart.h"
#include "../lib/string.h"
#include "timer.h"

// Global PCB array for all processes
PCB pcb[MAX_PROCESSES];

// Index of currently running process
uint32_t current_process = 0;

void watchdog_disable(void) {
    // Disable watchdog timer (WDT1) on AM335x
    // The watchdog will reset the board if not disabled or serviced periodically
    // Sequence: Write 0xAAAA to WDT_WSPR, wait for WWPS, then write 0x5555
    
    // First disable sequence
    PUT32(WDT_WSPR, 0xAAAA);
    // Wait for write posting to complete
    while (GET32(WDT_WWPS) != 0);
    
    // Second disable sequence
    PUT32(WDT_WSPR, 0x5555);
    // Wait for write posting to complete
    while (GET32(WDT_WWPS) != 0);
    
    // Verify watchdog is disabled (optional, but good for debugging)
    // WDT_WCLR should be 0 when disabled
}

void create_process(uint32_t index_pcb, uint32_t pid, uint32_t entry_point, uint32_t stack_top) {
    uint32_t* sp = (uint32_t*)stack_top;

    // El último en salir (pop) debe ser el primero en entrar (push)
    // 1. R0-R12 (Los más profundos en la pila)
    for(int i = 0; i < 13; i++) {
        *(--sp) = 0; 
    }

    // 2. LR (Punto de retorno)
    *(--sp) = entry_point;

    // 3. SPSR (Lo primero que sacará el pop {r12} en irq_handler)
    *(--sp) = 0x00000013; // SVC mode, IRQ enabled

    pcb[index_pcb].sp = (uint32_t)sp;
    pcb[index_pcb].pid = pid;
    pcb[index_pcb].state = READY;
}

uint32_t c_context_switch(uint32_t current_sp) {
    // 1. Guardar SP actual
    pcb[current_process].sp = current_sp;

    uint32_t* stack_ptr = (uint32_t*)current_sp;
    pcb[current_process].spsr = stack_ptr[0];
    pcb[current_process].lr = stack_ptr[1];   // This is the saved PC
    pcb[current_process].pc = stack_ptr[1];   // PC = LR saved from interrupt
    pcb[current_process].r0_r12[12] = stack_ptr[2];
    pcb[current_process].r0_r12[11] = stack_ptr[3];
    pcb[current_process].r0_r12[10] = stack_ptr[4];
    pcb[current_process].r0_r12[9]  = stack_ptr[5];
    pcb[current_process].r0_r12[8]  = stack_ptr[6];
    pcb[current_process].r0_r12[7]  = stack_ptr[7];
    pcb[current_process].r0_r12[6]  = stack_ptr[8];
    pcb[current_process].r0_r12[5]  = stack_ptr[9];
    pcb[current_process].r0_r12[4]  = stack_ptr[10];
    pcb[current_process].r0_r12[3]  = stack_ptr[11];
    pcb[current_process].r0_r12[2]  = stack_ptr[12];
    pcb[current_process].r0_r12[1]  = stack_ptr[13];
    pcb[current_process].r0_r12[0]  = stack_ptr[14];

    // 2. Log de depuración (esto prueba el cambio físico de memoria)
    os_write("Context Switch: P");
    uart_putc(current_process + '0'); // Convierte PID a char
    os_write(" SP=");
    os_write_hex(current_sp);

    timer_irq_handler();
    uint32_t next_process = schedule();

    os_write(" -> P");
    uart_putc(next_process + '0');
    os_write(" SP=");
    os_write_hex(pcb[next_process].sp);
    os_write("\n");

    return pcb[next_process].sp;
}


// Round-Robin scheduler - selects next process
// Returns the PID of the next process to run
uint32_t schedule(void) {
    uint32_t next = current_process;

    // Lógica simple de alternancia (Round Robin)
    if (current_process == PID_P1) {
        // Solo cambiamos a P2 si fue creado
        if (pcb[PID_P2].state != BLOCKED) {
            next = PID_P2;
        }
    } else if (current_process == PID_P2) {
        if (pcb[PID_P1].state != BLOCKED) {
            next = PID_P1;
        }
    }

    // Actualizamos estados solo si hubo cambio real
    if (next != current_process) {
        pcb[current_process].state = READY;
        current_process = next;
        pcb[current_process].state = RUNNING;
    }

    return current_process;
}


// First context switch - from OS to first user process
// This is called once during OS initialization
void first_context_switch(void) {
    // Start with P1
    current_process = PID_P1;
    pcb[current_process].state = RUNNING;
    
    // The assembly code will load P1's context and jump to it
    start_process_asm(pcb[PID_P1].sp);
}

int main(void) {
    os_write("=== OS Starting ===\n");

    for(int i = 0; i < MAX_PROCESSES; i++) {
        pcb[i].state = BLOCKED;
    }
    
    // Disable watchdog first
    watchdog_disable();
    os_write("  - Watchdog disabled\n");
    
    // Initialize PCBs for all processes
    os_write("  - Initializing PCBs...\n");
    
    // OS PCB (PID 0) - not really used for scheduling
    pcb[PID_OS].pid = PID_OS;
    pcb[PID_OS].state = RUNNING;  // OS is always running
    os_write("    - OS PCB initialized\n");

    // P1 PCB (PID 1)
    create_process(PID_P1, PID_P1, P1_ENTRY, P1_STACK_TOP);
    os_write("    - P1 initialized\n");
    
    // P2 PCB (PID 2)
    create_process(PID_P2, PID_P2, P2_ENTRY, P2_STACK_TOP);
    os_write("    - P2 initialized\n");
    
    // Initialize timer for periodic interrupts
    os_write("Inicializar timer");
    timer_init(1000);
    
    os_write("  - Starting first process (P1)...\n");
    os_write("=== Context Switching Started ===\n");
    
    // Trigger first context switch to P1
    // This will never return until processes yield or block
    first_context_switch();
    return 0;
}
