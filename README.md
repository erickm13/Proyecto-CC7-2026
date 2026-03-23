# Proyecto-CC7-2026

A bare-metal operating system kernel for the **BeagleBone Black** (ARM Cortex-A8), developed as a course project (CC7-2026). The kernel boots from reset, configures hardware peripherals, handles timer interrupts, and outputs debug messages over UART — all without any operating system or standard library support.

---

## Table of Contents

- [Repository Structure](#repository-structure)
- [Key Technologies](#key-technologies)
- [Code Organization](#code-organization)
  - [root.s — ARM Assembly Startup](#roots--arm-assembly-startup)
  - [os.c — OS Implementation](#osc--os-implementation)
  - [os.h — Header / Interface](#osh--header--interface)
  - [linker.ld — Memory Layout](#linkerld--memory-layout)
  - [makefile — Build System](#makefile--build-system)
- [Hardware & Memory Map](#hardware--memory-map)
- [Building](#building)
- [Running / Deployment](#running--deployment)
- [Expected Output](#expected-output)

---

## Repository Structure

```
Proyecto-CC7-2026/
├── .github/
│   └── CODEOWNERS              # Repository ownership (erickm13)
├── 001Multiprogramming/
│   └── OS/                     # Bare-metal kernel source
│       ├── os.c                # Main OS logic (watchdog, UART, timer, main)
│       ├── os.h                # Function declarations / public interface
│       ├── root.s              # ARM assembly: reset handler, vector table, IRQ
│       ├── linker.ld           # Linker script: memory regions and section layout
│       ├── makefile            # GNU Make build rules
│       ├── os.elf              # (generated) ELF executable with debug symbols
│       ├── os.bin              # (generated) Raw binary for flashing
│       ├── os.lst              # (generated) Disassembly listing
│       ├── os.o                # (generated) Compiled C object
│       └── root.o              # (generated) Assembled ARM object
└── README.md                   # This file
```

---

## Key Technologies

| Category | Technology |
|---|---|
| **Languages** | C (bare-metal, no stdlib) + ARM32 Assembly |
| **Target Platform** | BeagleBone Black — TI AM335x SoC (ARM Cortex-A8) |
| **Toolchain** | `arm-none-eabi` (GNU cross-compiler for bare-metal ARM) |
| **Build System** | GNU Make |
| **Linker** | GNU LD with a custom linker script (`linker.ld`) |
| **Output formats** | ELF (debug), raw binary (flash image), LST (disassembly) |

---

## Code Organization

### `root.s` — ARM Assembly Startup

This is the very first code that runs after reset. It is responsible for the lowest-level CPU setup before any C code can execute.

| Section | Lines | Responsibility |
|---|---|---|
| `_start` / `reset_handler` | 6–38 | Sets stack pointer (`sp = _stack_top`), installs exception vector table via VBAR, calls `main()` |
| `vector_table` | 13–21 | 32-byte-aligned table with branches to all 8 ARM exception handlers |
| `irq_handler` | 60–72 | Saves `r0–r12` + `lr`, calls `timer_irq_handler()` in C, restores registers, returns with `subs pc, lr, #4` |
| `PUT32` / `GET32` | 81–89 | Inline memory-mapped I/O helpers — write/read a 32-bit value at a given address |
| `enable_irq` | 98–109 | Reads CPSR, clears the I-bit (bit 7) to globally enable IRQ interrupts, writes CPSR back |
| Stack allocation | 113–119 | Reserves 8 KB of `.bss` space; `_stack_top` is exported for `reset_handler` |

### `os.c` — OS Implementation

Contains all C-level hardware drivers and the program entry point.

**Watchdog (`disable_watchdog`)**  
Writes the two-step magic sequence (`0xAAAA` then `0x5555`) to WDT1's Start/Stop register and polls the write-posting status bits between writes. This prevents the hardware watchdog from resetting the board during development.

**UART output (`uart_putc`, `os_write`)**  
Polls the UART0 Line Status Register until the Transmit Holding Register is empty, then writes one byte. `os_write` iterates over a string and converts `\n` to `\r\n` for serial terminal compatibility.

**Timer (`timer_init`)**  
Configures DMTIMER2 to fire an overflow interrupt every ~2 seconds:
1. Enables the Timer2 peripheral clock via Clock Manager.
2. Unmasks IRQ 68 (bit 4 of `INTC_MIR_CLEAR2`).
3. Sets interrupt priority/type register for IRQ 68.
4. Stops the timer, clears any pending interrupt flags.
5. Loads the reload value `0xFE91CA00` (≈ 2 s at 24 MHz) into `TLDR` and `TCRR`.
6. Enables overflow interrupts (`TIER = 0x2`).
7. Starts the timer in auto-reload mode (`TCLR = 0x3`).

**Timer interrupt handler (`timer_irq_handler`)**  
Called from the assembly IRQ handler every 2 seconds:
1. Clears the overflow flag in `TISR`.
2. Acknowledges the interrupt to the controller (`INTC_CONTROL = 0x1`).
3. Prints `"Tick\n"` over UART.

**`main()`**  
Orchestrates initialization in order:
1. Prints `"Starting...\n"`.
2. Calls `disable_watchdog()`.
3. Calls `timer_init()`.
4. Calls `enable_irq()` (assembly function in `root.s`).
5. Prints `"Enabling interrupts...\n"` then enters an infinite `while(1)` loop, waiting for timer interrupts.

### `os.h` — Header / Interface

Declares every function used across the C and assembly boundary so the compiler can type-check calls:

```c
void os_write(const char *s);
void uart_putc(char c);
void disable_watchdog(void);
void timer_init(void);
void timer_irq_handler(void);
void enable_irq(void);                          // implemented in root.s
void PUT32(unsigned int addr, unsigned int value); // implemented in root.s
unsigned int GET32(unsigned int addr);          // implemented in root.s
```

### `linker.ld` — Memory Layout

Defines a single `ram` region starting at `0x82000000` (256 KB) — the address where U-Boot loads the image on the BeagleBone Black.

| Section | Contents |
|---|---|
| `.text` | All code (`*(.text*)`) + read-only data (`*(.rodata*)`) |
| `.data` | Initialized global/static variables |
| `.bss` | Uninitialized variables; also holds the 8 KB stack |

`_stack_top` is set to `ORIGIN(ram) + LENGTH(ram)` = `0x82040000`, so the stack grows downward from the top of RAM.

### `makefile` — Build System

```
make          # Build os.bin and os.lst
make clean    # Remove all generated files (*.o, *.elf, *.bin, *.lst)
```

Key flags passed to `arm-none-eabi-gcc`:

| Flag | Purpose |
|---|---|
| `-mcpu=cortex-a8 -marm` | Target ARM Cortex-A8 in 32-bit ARM mode |
| `-ffreestanding` | No hosted standard-library assumptions |
| `-nostdlib -nostartfiles` | Do not link any default libraries or CRT startup files |
| `-O0 -g` | No optimization, full debug symbols |
| `-Wall -Wextra` | Enable extra compiler warnings |

---

## Hardware & Memory Map

| Peripheral | Base Address | Registers used |
|---|---|---|
| UART0 | `0x44E09000` | `THR` (transmit), `LSR` (status) |
| DMTIMER2 | `0x48040000` | `TCLR`, `TCRR`, `TISR`, `TIER`, `TLDR` |
| Interrupt Controller (INTCPS) | `0x48200000` | `MIR_CLEAR2`, `CONTROL`, `ILR68` |
| Clock Manager (CM_PER) | `0x44E00000` | `TIMER2_CLKCTRL` |
| Watchdog Timer 1 (WDT1) | `0x44E35000` | `WSPR`, `WWPS` |
| RAM (code + stack) | `0x82000000` | 256 KB region loaded by U-Boot |

---

## Building

### Prerequisites

Install the ARM bare-metal cross-compiler:

```bash
# Debian / Ubuntu
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi

# macOS (Homebrew)
brew install --cask gcc-arm-embedded
```

### Compile

```bash
cd 001Multiprogramming/OS
make
```

This produces:
- `os.elf` — ELF executable (includes DWARF debug info)
- `os.bin` — Raw binary suitable for flashing
- `os.lst` — Full disassembly (useful for debugging)

To clean generated files:

```bash
make clean
```

---

## Running / Deployment

1. **Load via U-Boot** (most common for BeagleBone Black):
   ```
   # On the target's U-Boot prompt:
   fatload mmc 0:1 0x82000000 os.bin
   go 0x82000000
   ```

2. **JTAG / OpenOCD**: Load `os.elf` directly into RAM at `0x82000000` and set the program counter to `_start`.

3. **Connect a serial terminal** to UART0 (pins P9-24 TX / P9-26 RX on BeagleBone Black header) at **115200 8N1** to see debug output.

---

## Expected Output

Once the binary is running you should see the following on the serial console, with `Tick` appearing every ~2 seconds thereafter:

```
Starting...
Watchdog disabled
Timer initialized
Enabling interrupts...
Tick
Tick
Tick
...
```
