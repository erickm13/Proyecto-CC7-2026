echo "Compiling Process1/main.c..."
arm-none-eabi-gcc -c \
    -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=hard \
    -I lib -Wall -O2 -nostdlib -nostartfiles -ffreestanding \
    -o trash/process1.o Process1/main.c

echo "Compiling lib/stdio.c..."
arm-none-eabi-gcc -c \
    -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=hard \
    -I lib -Wall -O2 -nostdlib -nostartfiles -ffreestanding \
    -o trash/stdio.o lib/stdio.c

echo "Linking Process1..."
arm-none-eabi-gcc -nostartfiles -T Process1/linker.ld \
    -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=hard \
    -o trash/process1.elf \
    trash/process1.o trash/stdio.o

echo "Generating Process1 binary..."
arm-none-eabi-objcopy -O binary trash/process1.elf bin/process1.bin