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
    b svc_handler
    
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

svc_handler:
    push {r0-r12, lr}
    mrs r0, spsr
    push {r0}

    mov r4, sp
    msr cpsr_c, #0xDF
    mov r1, sp
    msr cpsr_c, #0xD3
    mov sp, r4

    mov r0, sp
    bl c_svc_handler

    mov r4, r0
    bl get_current_user_sp
    msr cpsr_c, #0xDF
    mov sp, r0
    msr cpsr_c, #0xD3
    mov sp, r4

    pop {r0}
    msr spsr_cxsf, r0
    ldmfd sp!, {r0-r12, pc}^

prefetch_abort_handler:
    sub lr, lr, #4
    push {r0-r12, lr}
    mrs r0, spsr
    push {r0}

    mov r4, sp
    msr cpsr_c, #0xDF
    mov r1, sp
    msr cpsr_c, #0xD7
    mov sp, r4

    mov r0, sp
    mov r2, #1
    bl c_fault_handler

    mov r4, r0
    bl get_current_user_sp
    msr cpsr_c, #0xDF
    mov sp, r0
    msr cpsr_c, #0xD7
    mov sp, r4

    pop {r0}
    msr spsr_cxsf, r0
    ldmfd sp!, {r0-r12, pc}^

data_abort_handler:
    sub lr, lr, #8
    push {r0-r12, lr}
    mrs r0, spsr
    push {r0}

    mov r4, sp
    msr cpsr_c, #0xDF
    mov r1, sp
    msr cpsr_c, #0xD7
    mov sp, r4

    mov r0, sp
    mov r2, #2
    bl c_fault_handler

    mov r4, r0
    bl get_current_user_sp
    msr cpsr_c, #0xDF
    mov sp, r0
    msr cpsr_c, #0xD7
    mov sp, r4

    pop {r0}
    msr spsr_cxsf, r0
    ldmfd sp!, {r0-r12, pc}^

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
    
    // Set exception stacks before entering C.
    msr cpsr_c, #0xD2
    ldr sp, =0x8200BFFF

    msr cpsr_c, #0xD7
    ldr sp, =0x8200AFFF

    msr cpsr_c, #0xD3
    
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
    push {r0-r12, lr}       @ Guardar registros y PC de retorno
    mrs r0, spsr
    push {r0}               @ Guardar SPSR

    mov r4, sp
    msr cpsr_c, #0xDF
    mov r1, sp
    msr cpsr_c, #0xD2
    mov sp, r4

    mov r0, sp              @ Pasar SP actual a C
    bl c_context_switch

    mov r4, r0
    bl get_current_user_sp
    msr cpsr_c, #0xDF
    mov sp, r0
    msr cpsr_c, #0xD2
    mov sp, r4

    pop {r0}                @ Recuperar SPSR del nuevo proceso
    msr spsr_cxsf, r0       @ Preparar SPSR para el retorno
    ldmfd sp!, {r0-r12, pc}^

// ============================================================================
// First Context Switch - Jump from OS to first user process
// ============================================================================
.global start_process_asm
start_process_asm:
    mov r4, r0
    bl get_current_user_sp
    msr cpsr_c, #0xDF
    mov sp, r0
    msr cpsr_c, #0xD2  @ Usar la misma ruta de restore de excepciones
    mov sp, r4         @ Cargar el SP del proceso 1
    
    pop {r0}           @ Sacar SPSR
    msr spsr_cxsf, r0
    ldmfd sp!, {r0-r12, pc}^

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
