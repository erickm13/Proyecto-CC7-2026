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

    // 2. Extraer R4 de la pila (en el indice 5 del frame)
    uint32_t* frame = (uint32_t*)current_sp;
    uint32_t r4_val = frame[5];
    /*
    sp[0] = SPSR
    sp[1] = R0
    sp[2] = R1
    sp[3] = R2
    sp[4] = R3
    sp[5] = R4
    sp[6] = R5
    sp[7] = R6
    sp[8] = R7
    sp[9] = R8
    sp[10] = R9
    sp[11] = R10
    sp[12] = R11
    sp[13] = R12 
    sp[14] = LR 
    sp[15] = PC    
    */

    // 3. Log de depuración
    os_write("Context Switch: P");
    uart_putc(current_process + '0');
    os_write(" [R4=");
    os_write_hex(r4_val);
    os_write("] SP=");
    os_write_hex(current_sp);

    timer_irq_handler();

    // 4. Llamar al Planificador
    uint32_t next_sp = schedule();

    // 5. Log de entrada
    os_write(" -> P");
    uart_putc(current_process + '0');
    os_write(" SP=");
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
    // Arrancar el timer en el hardware emulado de versatile (1000 ms = 1s)
    os_write("[OS] Arrancando SP804 Timer (1s)...\n");
    timer_init(2000); 
    
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
