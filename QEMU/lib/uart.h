#ifndef UART_H
#define UART_H

#include <stdint.h>

/*
 * =============================================================================
 * QEMU versatilepb UART Definitions (PL011)
 * =============================================================================
 * 
 * The QEMU versatilepb board uses the ARM PrimeCell PL011 UART.
 * This is different from the AM335x UART on BeagleBone Black.
 * 
 * UART0 Base Address: 0x10009000 (for QEMU versatilepb)
 * 
 * NOTE: Some QEMU versions may use 0x101f1000. If output doesn't work,
 * try changing UART0_BASE to 0x101f1000.
 * 
 * REGISTER MAP (PL011):
 * - DR (0x00):   Data Register - read to receive, write to transmit
 * - FR (0x18):   Flag Register - status flags (TX full, RX empty, etc.)
 * - LCRH (0x1C): Line Control Register - data length, parity, FIFO enable
 * - IBRD (0x24): Integer Baud Rate Divisor
 * - FBRD (0x28): Fractional Baud Rate Divisor
 * - CR (0x30):   Control Register - enable TX/RX, UART enable
 * - ICR (0x44):  Interrupt Clear Register
 * 
 * CHANGES FROM BEAGLEBONE VERSION:
 * - Different base address (0x10009000 vs 0x44E09000)
 * - Different register offsets
 * - Different flag register bits
 * =============================================================================
 */

/* UART0 Base Address - QEMU versatilepb */
/* 0x101f1000 is the standard PL011 address for versatilepb */
#define UART0_BASE     0x101f1000

/* Register Offsets (from UART0_BASE) */
#define UART_DR      0x00   // Data Register - read/write
#define UART_FR      0x18   // Flag Register - read only
#define UART_LCRH    0x1C   // Line Control Register
#define UART_IBRD    0x24   // Integer Baud Rate Divisor
#define UART_FBRD    0x28   // Fractional Baud Rate Divisor
#define UART_CR      0x30   // Control Register
#define UART_ICR     0x44   // Interrupt Clear Register

/* Flag Register (FR) bits */
#define UART_FR_RXFE   0x10  // Receive FIFO Empty (bit 4)
#define UART_FR_TXFF   0x20  // Transmit FIFO Full (bit 5)
#define UART_FR_RXFF   0x40  // Receive FIFO Full (bit 6)
#define UART_FR_TXFE   0x80  // Transmit FIFO Empty (bit 7)
#define UART_FR_BUSY   0x08  // UART Busy (bit 3)

/* Control Register (CR) bits */
#define UART_CR_UARTEN 0x01  // UART Enable (bit 0)
#define UART_CR_TXE    0x0100 // Transmit Enable (bit 8)
#define UART_CR_RXE    0x0200 // Receive Enable (bit 9)

/* Line Control Register (LCRH) bits */
#define UART_LCRH_FEN  0x10  // FIFO Enable (bit 4)
#define UART_LCRH_WLEN_8 0x60 // 8-bit word length (bits 6-5)

/* Function Prototypes */

// Basic UART functions
void uart_init(void);
void uart_putc(char c);
char uart_getc(void);

// String output functions
void uart_puts(const char *s);
void os_write(const char *s);

// Input functions
void os_read(char *buffer, int max_length);

// Number formatting functions
void uart_putnum(unsigned int num);
void os_write_hex(uint32_t d);
void uart_itoa(int num, char *buffer);
int uart_atoi(const char *s);

/*
 * Low-level memory access functions
 * These are defined in OS/root.s for the OS, but we need them for UART too.
 * We provide inline versions here for user processes.
 */
static inline void PUT32(uint32_t addr, uint32_t value) {
    *(volatile uint32_t *)addr = value;
}

static inline uint32_t GET32(uint32_t addr) {
    return *(volatile uint32_t *)addr;
}

#endif // UART_H
