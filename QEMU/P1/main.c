#include "../lib/stdio.h"    
void delay(void) {
    for (volatile int i = 0; i < 5000000; i++);
}

int main(void) {
    int digit = 0;

    while (1) {
        // Forzamos un valor en R4 para demostrar el context switch
        asm volatile("ldr r4, =0x01010101");

        PRINT("----From P1: %d\n", digit);
        digit++;
        if (digit > 9) {
            digit = 0;
        }

        delay();
    }

    return 0;
}