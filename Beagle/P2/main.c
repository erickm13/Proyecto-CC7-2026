#include "../lib/stdio.h"

void delay(void) {
    for (volatile int i = 0; i < 10000000; i++);
}

int main(void) {
    char letter = 'a';

    while (1) {
        PRINT("----From P2: %c\n", letter);
        letter++;
        if (letter > 'z') {
            letter = 'a';
        }

        // Small delay for readability
        delay();
    }

    return 0;
}