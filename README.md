# Multiprogramming

## Building a Bare-Metal ARM Operating System

### General Description
This project implements a minimal multiprogramming (multitasking) system on the **BeagleBone Black** (AM335x, ARM Cortex-A8). The system runs without an underlying operating system and manages three distinct programs resident in memory:

1. **OS block** — Responsible for hardware initialization, interrupt handling, process scheduling, and context switching.
2. **User Process 1 (P1)** — Prints digits `0` through `9` in a loop, then repeats.
3. **User Process 2 (P2)** — Prints lowercase letters `a` through `z` in a loop, then repeats.

Because there is no MMU (Memory Management Unit), all programs are loaded at fixed memory addresses defined at link time. The OS uses the **AM335x DMTimer2** to generate periodic interrupts (for example, every second). On each timer interrupt, a **Round-Robin scheduler** performs a context switch between the two user processes.

Each process runs for one time slice and is then preempted. The result is interleaved output, demonstrating that multiple processes are sharing the CPU.

### Architecture
The project follows a layered approach:

- **Low-level / OS layer**: `os.c`, `root.s`
  - Hardware initialization
  - Vector table setup
  - Interrupt handling
  - Context switching
  - PCB management
- **Library layer**: `stdio.c`, `string.c`, `os.c` (I/O support)
  - `PRINT` support
  - Optional `READ`
  - String utilities
- **User layer**:
  - `P1/main.c`
  - `P2/main.c`

---

## Project Objectives
By the end of this project, the system should:

1. Load **three programs** (OS, P1, and P2) into fixed, non-overlapping memory regions.
2. Initialize AM335x hardware:
   - UART for serial I/O
   - DMTimer2 for periodic interrupts
   - INTC for interrupt routing
   - Watchdog disable sequence to avoid automatic reset
3. Implement a **Round-Robin scheduler** driven by timer interrupts.
4. Perform proper **context switching**, saving and restoring:
   - General-purpose registers
   - Stack pointer
   - Program counter
   - Link register
   - Status register
5. Give each process its own stack.
6. Produce interleaved serial output from P1 and P2.

The final system should be robust, predictable, and clearly separated into OS, library, and user code.

---

## Assignment Tasks

### 1. Hardware Initialization (OS Block)

#### `root.s`
- Set CPU to **System mode** or another appropriate privileged mode.
- Set the initial stack pointer for the OS.
- Clear the `.bss` section from `__bss_start__` to `__bss_end__`.
- Optionally use memory barriers (`dsb`, `isb`) after clearing `.bss`.
- Set up the ARM **vector table** so IRQ points to the IRQ handler.
- Jump to the C entry point using `bl main`.

#### `os.c`
- Disable **WDT1** as early as possible.
- Initialize or use **UART0** at base address `0x44E09000`.
- Configure **DMTimer2** at base address `0x48040000` for periodic interrupts.
- Configure **INTC** at base address `0x48200000`.
- Unmask **IRQ 68** (DMTimer2 overflow).
- Enable IRQs at the CPU level by clearing the I-bit in CPSR.

#### IRQ Handling
Implement an IRQ handler in assembly that:
- Saves `R0–R12` and `LR_irq`
- Calls `timer_irq_handler()` in C
- Restores `R0–R12` and `LR_irq`
- Returns from interrupt using `subs pc, lr, #4`

The C handler updates the current process PCB and restores the next process context.

---

### 2. Memory Layout and Linking
Design a fixed memory map so the OS and both user processes do not overlap.

| Region | Start | Size | Description |
|---|---:|---:|---|
| OS code/data | `0x82000000` | 64 KB | OS `.text`, `.data`, `.bss` |
| OS stack | `0x82010000` | 8 KB | OS stack (grows down) |
| P1 code/data | `0x82100000` | 64 KB | Process 1 `.text`, `.data`, `.bss` |
| P1 stack | `0x82110000` | 8 KB | Process 1 stack |
| P2 code/data | `0x82200000` | 64 KB | Process 2 `.text`, `.data`, `.bss` |
| P2 stack | `0x82210000` | 8 KB | Process 2 stack |

You may use different base addresses as long as they do not overlap and respect the AM335x memory map.

#### Linker Scripts
- Provide a linker script for the OS.
- Provide separate linker scripts for P1 and P2.
- Reserve stack regions explicitly.

#### Build Model
- Build the **OS** as one image.
- Build **P1** and **P2** separately as standalone images.
- No dynamic loading is required.

---

### 3. Process Control Blocks (PCB)
Each PCB should contain at least:

- Process ID
- Stack Pointer (`SP`)
- Program Counter (`PC`)
- Link Register (`LR`)
- Saved Program Status Register (`SPSR`) *(optional but recommended)*
- General-purpose registers `R0–R12`
- State field such as `RUNNING` or `READY` *(optional but recommended)*

Maintain an array of PCBs for:
- OS
- P1
- P2

The OS initializes each PCB with:
- Entry point
- Initial stack pointer
- Initial register values

---

### 4. Stack Setup for Each Process
Each process must have its own stack in a separate region.

Typical setup:
- Reserve a contiguous stack region (for example, 8 KB).
- Build an initial saved context at the top of that stack.
- Store `R0–R12` and `LR`.
- Set `LR` to the process entry point.
- Set PCB `SP` to the base of that saved frame.
- Set PCB `PC` to the process entry point.

Ensure stacks do not overlap with code, data, or other stacks.

---

### 5. Context Switching
On each timer interrupt:

1. Acknowledge the timer interrupt.
2. Signal **End of Interrupt** to the INTC.
3. Save the current process context.
4. Choose the next process using **Round-Robin scheduling**.
5. Restore the next process context.
6. Return from the IRQ handler.

For the **first run**:
- The OS initializes hardware, PCBs, and stacks.
- Then it performs an initial context switch to P1 or P2.
- Since there is no previous saved state yet, the PCB’s preloaded values are used.

---

### 6. User Process Implementation

#### Process 1 (`P1/main.c`)
- Uses `PRINT`
- Prints digits `0` through `9`
- Repeats forever
- May include a short delay loop

Example behavior:
```c
PRINT("----From P1: %d\n", n);
```

#### Process 2 (`P2/main.c`)
- Uses `PRINT`
- Prints letters `a` through `z`
- Repeats forever
- May include a short delay loop

Example behavior:
```c
PRINT("----From P2: %c\n", c);
```

Both processes run indefinitely and are preempted only by the timer-driven scheduler.

---

### 7. Build and Load

#### Build
- Compile and link the OS into one binary, such as `os.bin` or `os.elf`
- Compile and link P1 and P2 into separate binaries

#### Load with U-Boot
Example sequence:

```bash
loady 0x82000000   # Load OS
loady 0x82100000   # Load P1
loady 0x82200000   # Load P2
go 0x82000000      # Start OS
```

Document the exact load addresses and startup command in this README or in a `Loads.txt` file.

---

## Expected Output and Verification
The serial console should show interleaved output from both processes.

Example:

```text
----From P1: 0
----From P1: 1
----From P2: a
----From P2: b
----From P1: 3
----From P2: d
```

The exact order depends on timer settings and process execution time, but both processes must continue producing output over time.

### Verification Checklist
- P1 output appears correctly
- P2 output appears correctly
- Board does not reset after about 60 seconds
- Execution continues without hangs
- Context switching works correctly

---

## Implementation Hints
- Disable the watchdog very early using the required AM335x sequence.
- Clear `.bss` before calling `main`.
- Use memory barriers after critical initialization.
- Compute the interrupted `PC` correctly from `LR_irq`.
- Save and restore `SP` and `LR` from the correct CPU mode.
- Make the initial stack frame match the restore path layout.
- Use a known-good **DMTimer2** configuration.
- Avoid excessive debug output inside the timer handler.
- Disable interrupts during critical context-switch sections.

---

## Optional / Extra Points
Possible extensions include:

- Adding process states such as `RUNNING` and `READY`

---

## Summary
This project demonstrates how to build a minimal **bare-metal multitasking operating system** on ARM hardware. It combines:

- Hardware initialization
- Interrupt-driven scheduling
- Context switching
- Static memory layout
- Independent user processes

The result is a simple but complete multiprogramming system that proves multiple user programs can safely share the CPU on a bare-metal ARM platform.

