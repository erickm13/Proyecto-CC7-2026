#include "uart.h"

// Note: UART0_BASE + offset is handled in the calls below
#define UART_REG(off) (UART0_BASE + (off))

void uart_init(void) {
    // QEMU usually doesn't require baud rate config, but for completeness:
    // 1. Disable UART
    PUT32(UART_REG(UART_CR), 0x0);
    
    // 2. Set Line Control: 8-bit, FIFO enabled
    PUT32(UART_REG(UART_LCRH), UART_LCRH_WLEN_8 | UART_LCRH_FEN);
    
    // 3. Enable UART, Transmit, and Receive
    PUT32(UART_REG(UART_CR), UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE);
}

void uart_putc(char c) {
    // Wait while Transmit FIFO is Full (TXFF)
    while (GET32(UART_REG(UART_FR)) & UART_FR_TXFF);
    PUT32(UART_REG(UART_DR), c);
}

char uart_getc(void) {
    // Wait while Receive FIFO is Empty (RXFE)
    while (GET32(UART_REG(UART_FR)) & UART_FR_RXFE);
    return (char)(GET32(UART_REG(UART_DR)) & 0xFF);
}

void uart_puts(const char *s) {
    while (*s) {
        uart_putc(*s++);
    }
}

void os_write(const char *s) {
    uart_puts(s);
}

void os_read(char *buffer, int max_length) {
    int i = 0;
    char c;
    while (i < max_length - 1) {
        c = uart_getc();
        // Echo back and handle newline
        if (c == '\r' || c == '\n') {
            uart_putc('\n');
            break;
        }
        uart_putc(c);
        buffer[i++] = c;
    }
    buffer[i] = '\0';
}

void uart_putnum(unsigned int num) {
    char buf[11];
    int i = 0;
    if (num == 0) {
        uart_putc('0');
        return;
    }
    while (num > 0) {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    }
    while (i > 0) {
        uart_putc(buf[--i]);
    }
}

void os_write_hex(uint32_t d) {
    char hex_chars[] = "0123456789ABCDEF";
    os_write("0x");
    for (int i = 7; i >= 0; i--) {
        uint32_t nibble = (d >> (i * 4)) & 0xF;
        uart_putc(hex_chars[nibble]);
    }
}

void uart_itoa(int num, char *buffer) {
    int i = 0;
    int is_negative = 0;
    if (num == 0) { buffer[i++] = '0'; buffer[i] = '\0'; return; }
    if (num < 0) { is_negative = 1; num = -num; }
    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    if (is_negative) buffer[i++] = '-';
    buffer[i] = '\0';
    for (int j = 0; j < i / 2; j++) {
        char temp = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = temp;
    }
}

int uart_atoi(const char *s) {
    int num = 0, sign = 1, i = 0;
    if (s[0] == '-') { sign = -1; i++; }
    for (; s[i] >= '0' && s[i] <= '9'; i++) 
        num = num * 10 + (s[i] - '0');
    return sign * num;
}
