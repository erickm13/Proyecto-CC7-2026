.section .text
.syntax unified
.code 32
.globl _start

_start:
    b reset_handler


// Exception Vector Table
// Must be aligned to 32 bytes (0x20)
.align 5
vector_table:
    b reset_handler      @ 0x00: Reset
    b undefined_handler  @ 0x04: Undefined Instruction
    b swi_handler        @ 0x08: Software Interrupt (SWI)
    b prefetch_handler   @ 0x0C: Prefetch Abort
    b data_handler       @ 0x10: Data Abort
    b .                  @ 0x14: Reserved
    b irq_handler        @ 0x18: IRQ (Interrupt Request)
    b fiq_handler        @ 0x1C: FIQ (Fast Interrupt Request)


reset_handler:

    // Disable IRQ and FIQ during initialization
    cpsid if

    // Set IRQ mode stack
    cps #0x12
    ldr sp, =_irq_stack_top

    // Set Supervisor mode stack
    cps #0x13
    ldr sp, =_stack_top

    // Set up exception vector table base address (VBAR - Vector Base Address Register)
    ldr r0, =vector_table
    mcr p15, 0, r0, c12, c0, 0
    dsb
    isb

    // Call main function
    bl main

hang:
    b hang


undefined_handler:
    b hang

swi_handler:
    b hang

prefetch_handler:
    b hang

data_handler:
    b hang


// IRQ handler with PCB-based context switch
irq_handler:

    // Save original r0 and lr_irq on IRQ stack
    sub sp, sp, #8
    str r0, [sp]
    str lr, [sp, #4]

    // r3 = IRQ stack pointer
    mov r3, sp

    // r2 = lr_irq
    ldr r2, [r3, #4]

    // r0 = original r0
    ldr r0, [r3]

    // Switch to System mode so we can use task stack
    cps #0x1F

    // Reserve 56 bytes on current task stack
    sub sp, sp, #56

    // Save r0-r12 into current task context frame
    stmia sp, {r0-r12}

    // Save synthetic lr at offset 52
    str r2, [sp, #52]

    // r0 = saved current task sp
    mov r0, sp

    // Back to IRQ mode
    cps #0x12

    // Restore IRQ stack pointer
    mov sp, r3

    // Clear timer + ask scheduler for next task sp
    push {r4, lr}
    bl timer_irq_handler
    bl schedule_next_sp
    pop {r4, lr}

    // r4 = next task sp
    mov r4, r0

    // Switch to System mode and restore next task context
    cps #0x1F
    mov sp, r4

    // Restore r0-r12
    ldmia sp, {r0-r12}

    // Load synthetic lr
    ldr r1, [sp, #52]

    // Pop context frame
    add sp, sp, #56

    // Back to IRQ mode
    cps #0x12

    // Put next task return address in lr_irq
    mov lr, r1

    // Drop temporary IRQ scratch
    add sp, sp, #8

    // Return from IRQ into selected task
    subs pc, lr, #4


fiq_handler:
    b hang


// Low-level memory access functions
.globl PUT32
PUT32:
    str r1, [r0]
    bx lr


.globl GET32
GET32:
    ldr r0, [r0]
    bx lr


// TODO: Implement enable_irq function
// This function should:
// 1. Read the current CPSR
// 2. Clear the I-bit (bit 7) to enable IRQ interrupts
// 3. Write back to CPSR
.globl enable_irq
enable_irq:

    // 1. Read CPSR
    mrs r0, cpsr

    // 2. Clear interrupt disable bit
    bic r0, r0, #0x80

    // 3. Write CPSR back
    msr cpsr_c, r0

    bx lr


// Start first task from an initialized PCB stack
// r0 = task sp
.globl start_first_task
start_first_task:

    // Switch to System mode
    cps #0x1F

    // Load task stack
    mov sp, r0

    // Restore r0-r12
    ldmia sp, {r0-r12}

    // Load synthetic lr
    ldr r1, [sp, #52]

    // Pop context frame
    add sp, sp, #56

    // Jump to task entry
    sub r1, r1, #4
    bx r1


// Stack space allocation
.section .bss
.align 4

_irq_stack_bottom:
    .skip 0x1000  @ 4KB IRQ stack

_irq_stack_top:

_stack_bottom:
    .skip 0x2000  @ 8KB Supervisor stack

_stack_top: