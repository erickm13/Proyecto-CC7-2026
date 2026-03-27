#ifndef UART_H
#define UART_H

#include <stdint.h>

// BeagleBone Black UART0 base address
#define UART0_BASE     0x44E09000
#define UART_THR       (UART0_BASE + 0x00)  // Transmit Holding Register
#define UART_LSR       (UART0_BASE + 0x14)  // Line Status Register
#define UART_LSR_THRE  0x20                  // Transmit Holding Register Empty
#define UART_LSR_RXFE  0x10                  // Receive FIFO Empty

void uart_putc(char c);
char uart_getc(void);
void uart_puts(const char *s);
void os_write(const char *s);
void os_read(char *buffer, int max_length);
void uart_putnum(unsigned int num);
void uart_itoa(int num, char *buffer);
void os_write_hex(uint32_t d);

static inline void PUT32(uint32_t addr, uint32_t value) {
    *(volatile uint32_t *)addr = value;
}

static inline uint32_t GET32(uint32_t addr) {
    return *(volatile uint32_t *)addr;
}

#endif // UART_H