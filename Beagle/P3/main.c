#include "../lib/stdio.h"
#include "../lib/user_syscalls.h"

void delay(void) {
    for (volatile int i = 0; i < 10000000; i++);
}

int main(void) {

    while (1) {
        PRINT("----From P3 ------\n");
        PRINT("Prueba Tercer Proceso.\n");

        sys_yield();
        delay();
    }

    return 0;
}
