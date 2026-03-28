#!/bin/bash

# Salir inmediatamente si algún comando falla
set -e

echo "========================================"
echo " Limpiando el entorno..."
echo "========================================"
rm -rf obj/ build/
mkdir -p obj/OS obj/P1 obj/P2 obj/lib
mkdir -p build

echo "========================================"
echo " 1. Compilando el Sistema Operativo (OS)"
echo "========================================"
# Ensamblador y SO
arm-none-eabi-as -o obj/OS/root.o OS/root.s
arm-none-eabi-gcc -c -Wall -Wno-array-bounds -O2 -nostdlib -nostartfiles -ffreestanding -o obj/OS/os.o OS/os.c

# Periféricos (Ahora en lib/)
arm-none-eabi-gcc -c -Wall -O2 -nostdlib -nostartfiles -ffreestanding -o obj/OS/timer.o lib/timer.c
arm-none-eabi-gcc -c -Wall -O2 -nostdlib -nostartfiles -ffreestanding -o obj/OS/uart.o lib/uart.c

# Linkear OS
arm-none-eabi-ld -T OS/linker.ld -o obj/OS/os.elf obj/OS/root.o obj/OS/os.o obj/OS/timer.o obj/OS/uart.o
arm-none-eabi-objcopy -O binary obj/OS/os.elf build/os.bin

echo "========================================"
echo " 2. Compilando Proceso 1 (P1)"
echo "========================================"
arm-none-eabi-as -o obj/P1/start.o P1/start.s
arm-none-eabi-gcc -c -Wall -O2 -nostdlib -nostartfiles -ffreestanding -o obj/P1/main.o P1/main.c
arm-none-eabi-gcc -c -Wall -O2 -nostdlib -nostartfiles -ffreestanding -o obj/lib/stdio.o lib/stdio.c
arm-none-eabi-gcc -c -Wall -O2 -nostdlib -nostartfiles -ffreestanding -o obj/lib/uart.o lib/uart.c
arm-none-eabi-ld -T P1/linker.ld -o obj/P1/p1.elf obj/P1/start.o obj/P1/main.o obj/lib/stdio.o obj/lib/uart.o
arm-none-eabi-objcopy -O binary obj/P1/p1.elf build/p1.bin

echo "========================================"
echo " 3. Compilando Proceso 2 (P2)"
echo "========================================"
arm-none-eabi-as -o obj/P2/start.o P2/start.s
arm-none-eabi-gcc -c -Wall -O2 -nostdlib -nostartfiles -ffreestanding -o obj/P2/main.o P2/main.c
# stdio.o and uart.o already compiled in P1 step
arm-none-eabi-ld -T P2/linker.ld -o obj/P2/p2.elf obj/P2/start.o obj/P2/main.o obj/lib/stdio.o obj/lib/uart.o
arm-none-eabi-objcopy -O binary obj/P2/p2.elf build/p2.bin

echo "========================================"
echo " Ejecutando en QEMU..."
echo "========================================"
qemu-system-arm -M versatilepb -m 128M -audio none -nographic \
    -kernel obj/OS/os.elf \
    -device loader,file=build/p1.bin,addr=0x00200000 \
    -device loader,file=build/p2.bin,addr=0x00300000
