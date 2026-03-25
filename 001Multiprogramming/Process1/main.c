#include "../lib/stdio.h"

int main(void) {
    int i = 0;

    while (1) {
        PRINT("----From P1: %d\n", i);

        i++;
        if (i > 9) {
            i = 0;
        }
    }

    return 0;
}