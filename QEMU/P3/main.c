#include "../lib/stdio.h"
#include "../lib/user_syscalls.h"

void delay(void) {
    for (volatile int i = 0; i < 100000; i++);
}

int main(void) {

    while (1) {
        // Forzamos un valor en R4 para demostrar el context switch
        asm volatile("ldr r4, =0x02020202");

        PRINT("----From P3 ------\n");
        PRINT("Prueba Tercer Proceso.\n");

        //const char msg[] = "P2 direct sys_write\n";
        //sys_write(1, msg, sizeof(msg) - 1);

        //sys_yield();
        delay();
    }

    return 0;
}
