#include "../lib/stdio.h"

void delay(void) {
    for (volatile int i = 0; i < 5000000; i++);
}

int main(void) {
    char letter = 'a';

    while (1) {
        // Forzamos un valor en R4 para demostrar el context switch
        asm volatile("ldr r4, =0x02020202");

        PRINT("----From P2: %c\n", letter);
        letter++;
        if (letter > 'z') {
            letter = 'a';
        }

        delay();
    }

    return 0;
}