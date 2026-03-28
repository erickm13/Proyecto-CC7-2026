#include "../lib/stdio.h"

void delay(void) {
    for (volatile int i = 0; i < 10000000; i++);
}

int main(void) {
    int digit = 0;

    while (1) {
        PRINT("----From P1: %d\n", digit);
        digit++;
        if (digit > 9) {
            digit = 0;
        }

        // Small delay for readability
        delay();
    }

    return 0;
}