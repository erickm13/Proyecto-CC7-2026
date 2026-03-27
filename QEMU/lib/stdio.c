#include <stdarg.h>
#include "uart.h"

void PRINT(const char *format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[64];

    for (const char *p = format; *p != '\0'; p++) {
        if (*p == '%') {
            p++;
            switch (*p) {
                case 's': uart_puts(va_arg(args, char*)); break;
                case 'd':
                    uart_itoa(va_arg(args, int), buffer);
                    uart_puts(buffer);
                    break;
                case 'c': uart_putc((char)va_arg(args, int)); break;
            }
        } else {
            uart_putc(*p);
        }
    }
    va_end(args);
}
