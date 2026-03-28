.section .text
.syntax unified
.code 32
.globl _start
.globl PUT32
.globl GET32
.globl enable_irq
.globl start_process_asm

// ============================================================================
// 1. Tabla de Vectores
// ============================================================================
vector_table:
    ldr pc, reset_addr
    ldr pc, undef_addr
    ldr pc, swi_addr
    ldr pc, pabt_addr
    ldr pc, dabt_addr
    nop
    ldr pc, irq_addr
    ldr pc, fiq_addr

reset_addr: .word _start
undef_addr: .word undefined_handler
swi_addr:   .word swi_handler
pabt_addr:  .word prefetch_handler
dabt_addr:  .word data_handler
irq_addr:   .word irq_handler
fiq_addr:   .word fiq_handler

hang:
    b hang

// ============================================================================
// _start - Punto de entrada del OS
// ============================================================================
_start:
    // 1. Copiar la tabla de vectores a la dirección 0x00000000
    // Esto es crítico si QEMU no cargó el kernel exactamente en 0x0
    ldr r0, =0x00000000     @ Destino
    adr r1, vector_table    @ Origen (PC-relative)
    mov r2, #64             @ Copiamos 16 palabras
copy_vectors:
    ldr r3, [r1], #4
    str r3, [r0], #4
    subs r2, r2, #4
    bgt copy_vectors

    // 2. Configurar Pila para el modo IRQ (Interrupciones)
    msr cpsr_c, #0xD2           @ Modo IRQ, IRQ/FIQ disabled
    ldr sp, =0x0000BFFF         @ Stack Pointer de IRQ

    // 3. Configurar Pila para el modo SVC (Supervisor - OS)
    msr cpsr_c, #0xD3           @ Modo SVC, IRQ/FIQ disabled
    ldr sp, =0x00010000         @ Stack Pointer del OS (os.h)

    // 4. Limpiar la sección .bss
    ldr r0, =__bss_start__
    ldr r1, =__bss_end__
    mov r2, #0
clear_bss_loop:
    cmp r0, r1
    bge clear_bss_done
    str r2, [r0], #4
    b clear_bss_loop
clear_bss_done:

    // 5. Saltar a main en os.c
    bl main
    b hang

undefined_handler:
    b hang

swi_handler:
    b hang

prefetch_handler:
    b hang

data_handler:
    b hang

fiq_handler:
    b hang

// ============================================================================
// Context Switch: IRQ Handler
// ============================================================================
irq_handler:
    @ Ajustar LR para el retorno
    sub lr, lr, #4
    
    @ Guardar contexto: R0-R12 y LR_irq (que es nuestro PC de retorno)
    push {r0-r12, lr}
    
    @ Guardar SPSR
    mrs r0, spsr
    push {r0}
    

    @ Llamar al planificador
    mov r0, sp
    bl c_context_switch
    
    @ Restaurar SP del nuevo proceso
    mov sp, r0
    
    @ Restaurar SPSR
    pop {r0}
    msr spsr_cxsf, r0
    
    @ Restaurar registros y PC (salto atómico)
    ldmfd sp!, {r0-r12, pc}^

// ============================================================================
// Primer salto al Proceso 1 o 2 sino existe el 1
// ============================================================================
start_process_asm:
    @ r0 contiene el SP del proceso inicial
    @ Cambiamos a IRQ mode para usar su sp banked para el primer salto
    msr cpsr_c, #0xD2
    mov sp, r0
    
    @ Sacar SPSR inicial
    pop {r0}
    msr spsr_cxsf, r0
    
    @ Sacar contexto y saltar
    ldmfd sp!, {r0-r12, pc}^

PUT32:
    str r1, [r0]
    bx lr

GET32:
    ldr r0, [r0]
    bx lr

enable_irq:
    mrs r0, cpsr
    bic r0, r0, #0x80
    msr cpsr_c, r0
    bx lr
