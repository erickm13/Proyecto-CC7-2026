.section .text
.syntax unified
.code 32
.globl _start
.globl PUT32
.globl GET32
.globl enable_irq

// ============================================================================
// Exception Vector Table
// Must be aligned to 32 bytes (0x20)
// ============================================================================
.align 5
vector_table:
    // Reset Vector (0x00)
    b _start
    
    // Undefined Instruction Vector (0x04)
    b undefined_handler
    
    // Software Interrupt Vector (0x08)
    b swi_handler
    
    // Prefetch Abort Vector (0x0C)
    b prefetch_abort_handler
    
    // Data Abort Vector (0x10)
    b data_abort_handler
    
    // Reserved Vector (0x14)
    nop
    
    // IRQ Vector (0x18) - This is what we need for timer interrupts
    b irq_handler
    
    // FIQ Vector (0x1C)
    b fiq_handler

// ============================================================================
// Exception Handlers (stub implementations)
// ============================================================================
undefined_handler:
    b hang

swi_handler:
    b hang

prefetch_abort_handler:
    b hang

data_abort_handler:
    b hang

fiq_handler:
    b hang

hang:
    b hang

// ============================================================================
// _start - OS Entry Point
// ============================================================================
_start:
    // Disable interrupts during initialization
    cpsid i
    cpsid f
    
    // Set CPU to SVC mode (Supervisor mode) for initialization
    mrs r0, cpsr
    bic r0, r0, #0x1F
    orr r0, r0, #0x13
    msr cpsr_c, r0
    
    // Set up SVC mode stack pointer
    ldr sp, =_stack_top

    @ --- SOBREESCRIBIR EL VBAR (Vector Base Address Register) ---
    @ Obligamos al Cortex-A8 a usar nuestra tabla de vectores, ignorando la de U-Boot
    ldr r0, =vector_table
    mcr p15, 0, r0, c12, c0, 0
    @ ------------------------------------------------------------
    
    // Clear .bss section (Initialize uninitialized C variables to 0)
    ldr r0, =__bss_start__
    ldr r1, =__bss_end__
    mov r2, #0              @ <--- ¡CORRECCIÓN CRÍTICA: r2 debe ser 0!
    cmp r0, r1
    bge clear_bss_done
clear_bss_loop:
    str r2, [r0], #4        @ Escribe 0 (r2) en la memoria y avanza 4 bytes
    cmp r0, r1
    blo clear_bss_loop
clear_bss_done:
    
    // Memory barrier to ensure BSS is cleared before C code runs
    dsb
    isb
    
    // Jump to OS C main
    bl main
    
    // Should never reach here (main should end in first_context_switch)
    b hang
// ============================================================================
// IRQ Handler - Context Switch Entry Point
// ============================================================================
irq_handler:
    sub lr, lr, #4          @ Ajustar LR_irq
    cps #0x13               @ Cambiar a SVC mode
    push {r0-r12}           @ Guardar registros
    push {lr}               @ Guardar el PC de retorno
    mrs r12, spsr
    push {r12}              @ Guardar SPSR

    mov r0, sp              @ Pasar SP actual a C
    bl c_context_switch
    mov sp, r0              @ Cargar nuevo SP

    pop {r12}               @ Recuperar SPSR del nuevo proceso
    msr spsr_cxsf, r12      @ Preparar SPSR para el retorno

    pop {lr}                @ Recuperar PC de retorno hacia LR
    pop {r0-r12}            @ Recuperar registros generales

    subs pc, lr, #0         @ ¡IMPORTANTE! Usa #0 porque el ajuste -4 ya se hizo arriba

// ============================================================================
// First Context Switch - Jump from OS to first user process
// ============================================================================
.global start_process_asm
start_process_asm:
    mov sp, r0         @ Cargar el SP del proceso 1
    
    pop {r12}          @ Sacar SPSR
    msr spsr_cxsf, r12 
    
    pop {lr}           @ Sacar Entry Point
    pop {r0-r12}       @ Sacar registros
    
    @ El truco final: usamos subs pc para saltar al proceso y cambiar de modo 
    @ atómicamente, como si fuera el fin de una IRQ.
    subs pc, lr, #0

// ============================================================================
// Low-level memory access functions
// ============================================================================
.globl PUT32
PUT32:
    str r1, [r0]
    bx lr

.globl GET32
GET32:
    ldr r0, [r0]
    bx lr
    
.globl enable_irq
enable_irq:
    mrs r0, cpsr
    bic r0, r0, #0x80
    msr cpsr_c, r0
    bx lr

// ============================================================================
// Stack space allocation (8KB for OS)
// ============================================================================
.section .bss
.align 4
_stack_bottom:
    .skip 0x2000  @ 8KB stack space
_stack_top:
