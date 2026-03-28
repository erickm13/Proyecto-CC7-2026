#include "os.h"
#include "../lib/uart.h"
#include "../lib/timer.h"

// Variables Globales
PCB pcb[MAX_PROCESSES];
uint32_t current_process = 0;

// Simulador de desactivación de Watchdog (En VersatilePB no es necesario, lo dejamos vacío)
void watchdog_disable(void) {
    // QEMU versatilepb no tiene un watchdog activo por defecto.
}

// Inicialización de la pila virtual de un proceso
void create_process(uint32_t index_pcb, uint32_t pid, uint32_t entry_point, uint32_t stack_top) {
    uint32_t* sp = (uint32_t*)stack_top;

    // Reservar espacio para 14 registros (R0-R12, LR) + 1 SPSR = 15 palabras
    sp -= 15;

    // Inicializar todo en 0 para evitar basura en memoria
    for(int i = 0; i < 15; i++) {
        sp[i] = 0;
    }

    // Configurar el "Entry Point" en el lugar del LR guardado
    sp[14] = entry_point; 
    
    // Configurar SPSR inicial (SVC mode, IRQ habilitados -> 0x13)
    sp[0] = 0x13; 

    pcb[index_pcb].pid = pid;
    pcb[index_pcb].state = READY;
    pcb[index_pcb].sp = (uint32_t)sp;
}

// Planificador (Scheduler) Round-Robin Simple
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

// Handler llamado desde root.s cada que el Timer (SP804) llega a 0
uint32_t c_context_switch(uint32_t current_sp) {
    // 1. Guardar SP del proceso suspendido
    pcb[current_process].sp = current_sp;

    uint32_t* stack_ptr = (uint32_t*)current_sp;
    pcb[current_process].spsr = stack_ptr[0];
    pcb[current_process].lr = stack_ptr[1];   // This is the saved PC
    pcb[current_process].pc = stack_ptr[1];   // PC = LR saved from interrupt
    pcb[current_process].r0_r12[12] = stack_ptr[2]; //
    pcb[current_process].r0_r12[11] = stack_ptr[3]; //
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

    os_write_hex(pcb[current_process].r0_r12[0]);

    // 3. Log de depuración
    os_write("Context Switch: P");
    uart_putc(current_process + '0');
    os_write(" Guardando SP=");
    os_write_hex(current_sp);

    timer_irq_handler();

    // 4. Llamar al Planificador
    uint32_t next_sp = schedule();

    // 5. Log de entrada
    os_write(" -> P");
    uart_putc(current_process + '0');
    os_write(" Restaurando SP=");
    os_write_hex(pcb[next_sp].sp);
    os_write("\n");

    return pcb[next_sp].sp;
}

// Primera llamada para inicializar el salto hacia modo sistema
void first_context_switch(void) {
    if (pcb[PID_P1].state != BLOCKED) {
        current_process = PID_P1;
    } else {
        current_process = PID_P2;
    }
    pcb[current_process].state = RUNNING;

    start_process_asm(pcb[current_process].sp);
}

int main(void) {
    uart_init(); // Opcional, dependiendo si UART necesita ser inicializado en tu setup
    os_write("\n========================================\n");
    os_write("QEMU versatilepb Multiprogramming OS\n");
    os_write("========================================\n");

    // Inicializar todas las PCBs a estado de bloqueo
    for(int i = 0; i < MAX_PROCESSES; i++) {
        pcb[i].state = BLOCKED;
    }

    // Inicializar el Proceso Cero (SO)
    pcb[PID_OS].pid = PID_OS;
    pcb[PID_OS].state = RUNNING;

    // Inicializamos las Pila y estado de los Procesos de Usuario
    os_write("[OS] Configurando P1 y P2...\n");
    create_process(PID_P1, PID_P1, P1_ENTRY, P1_STACK_TOP);
    create_process(PID_P2, PID_P2, P2_ENTRY, P2_STACK_TOP);
    // Arrancar el timer en el hardware emulado de versatile (50 ms = scheduling rapido)
    os_write("[OS] Arrancando SP804 Timer (50ms)...\n");
    timer_init(500); 
    
    // Habilitamos las interrupciones Globalmente (Despejar bit I en el CPSR)
    enable_irq();

    // Arrancamos el Scheduling
    os_write("[OS] Inciando Context Switch!\n");
    first_context_switch();

    // Si todo funciona bien, la ejecución de la CPU ya está en los procesos 1 y 2, 
    // y esta línea de while jamás se ejecutará.
    while(1);
    
    return 0;
}
