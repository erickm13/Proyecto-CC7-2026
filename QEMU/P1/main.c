#include "../lib/stdio.h"
#include "../lib/user_syscalls.h"
void delay(void) {
    for (volatile int i = 0; i < 5000000; i++);
}

int main(void) {
    int digit = 0;

    while (1) {
        asm volatile("ldr r4, =0x01010101");

        PRINT("----From P1: %d\n", digit);
        /*
        count++;
        if (count == 5) {
            PRINT("----P1 exiting now----\n");
            sys_exit(0);
        }
        */
        //const char msg[] = "P1 direct sys_write\n";
        //sys_write(1, msg, sizeof(msg) - 1);

        digit++;
        if (digit > 9) {
            digit = 0;
        }

        //sys_yield();
        delay();
    }

    return 0;
}
