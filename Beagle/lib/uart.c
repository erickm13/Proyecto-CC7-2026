// UART support for user processes (P1, P2) - without floating point
#include "uart.h"

void uart_putc(char c) {
    while ((GET32(UART_LSR) & UART_LSR_THRE) == 0);
    PUT32(UART_THR, c);
}

char uart_getc(void) {
    while ((GET32(UART_LSR) & UART_LSR_RXFE) != 0);
    return (char)(GET32(UART_THR) & 0xFF);
}

void uart_puts(const char *s) {
    while (*s) {
        uart_putc(*s++);
    }
}

void os_write(const char *s) {
    while (*s) {
        uart_putc(*s++);
    }
}

void os_read(char *buffer, int max_length) {
    int i = 0;
    char c;
    while (i < max_length - 1) {
        c = uart_getc();
        if (c == '\n' || c == '\r') {
            uart_putc('\n');
            break;
        }
        uart_putc(c);
        buffer[i++] = c;
    }
    buffer[i] = '\0';
}

void uart_putnum(unsigned int num) {
    char buf[10];
    int i = 0;
    if (num == 0) {
        uart_putc('0');
        return;
    }
    do {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    } while (num > 0 && i < 10);
    while (i > 0) {
        uart_putc(buf[--i]);
    }
}

// Integer to string conversion
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
    // Reverse string
    for (int j = 0; j < i / 2; j++) {
        char temp = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = temp;
    }
}

int uart_atoi(const char *s) {
    int num = 0, sign = 1, i = 0;
    if (s[0] == '-') { sign = -1; i++; }
    for (; s[i] >= '0' && s[i] <= '9'; i++) num = num * 10 + (s[i] - '0');
    return sign * num;
}

void os_write_hex(uint32_t d) {
    char hex_chars[] = "0123456789ABCDEF";
    os_write("0x"); // Prefijo estándar para hexadecimal
    
    // Un uint32_t tiene 8 dígitos hexadecimales (4 bits cada uno)
    // Empezamos desde el más significativo (el de la izquierda)
    for (int i = 7; i >= 0; i--) {
        // Desplazamos el número para poner los 4 bits que queremos al final
        // y aplicamos una máscara 0xF (1111 en binario)
        uint32_t nibble = (d >> (i * 4)) & 0xF;
        
        // Enviamos el carácter correspondiente
        uart_putc(hex_chars[nibble]);
    }
}

