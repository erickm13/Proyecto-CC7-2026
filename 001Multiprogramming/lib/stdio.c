#include "stdio.h"
#include <stdarg.h>

#define UART0_BASE     0x44E09000
#define UART_THR       (UART0_BASE + 0x00)
#define UART_LSR       (UART0_BASE + 0x14)
#define UART_LSR_THRE  0x20

static inline void PUT32(unsigned int addr, unsigned int value) {
    *(volatile unsigned int *)addr = value;
}

static inline unsigned int GET32(unsigned int addr) {
    return *(volatile unsigned int *)addr;
}

void uart_putc(char c) {
    while ((GET32(UART_LSR) & UART_LSR_THRE) == 0) {
    }
    PUT32(UART_THR, c);
}

void uart_puts(const char *s) {
    while (*s) {
        uart_putc(*s++);
    }
}

void uart_itoa(int num, char *buffer) {
    int i = 0;
    int is_negative = 0;

    if (num == 0) {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return;
    }

    if (num < 0) {
        is_negative = 1;
        num = -num;
    }

    while (num > 0) {
        buffer[i++] = (num % 10) + '0';
        num /= 10;
    }

    if (is_negative) {
        buffer[i++] = '-';
    }

    buffer[i] = '\0';

    for (int j = 0, k = i - 1; j < k; j++, k--) {
        char temp = buffer[j];
        buffer[j] = buffer[k];
        buffer[k] = temp;
    }
}

void PRINT(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    for (int i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] != '%') {
            uart_putc(fmt[i]);
            continue;
        }

        i++;

        if (fmt[i] == 'd') {
            int num = va_arg(args, int);
            char buffer[16];
            uart_itoa(num, buffer);
            uart_puts(buffer);
        } else {
            uart_putc('%');
            uart_putc(fmt[i]);
        }
    }

    va_end(args);
}

void READ(const char *fmt, ...) {
    (void)fmt;
}