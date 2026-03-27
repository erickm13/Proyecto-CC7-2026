# ==========================================
# Configuración de Directorios
# ==========================================
BUILD_DIR = build
BIN_DIR = bin

# ==========================================
# Configuración del Toolchain y Flags
# ==========================================
CC = arm-none-eabi-gcc
AS = arm-none-eabi-as
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy

# Flags para la arquitectura de la BeagleBone Black
CFLAGS = -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=hard -Wall -O2 -nostdlib -ffreestanding

# ==========================================
# Archivos Objeto (se guardarán en build/)
# ==========================================
LIB_OBJS = $(BUILD_DIR)/uart.o $(BUILD_DIR)/stdio.o $(BUILD_DIR)/string.o
OS_OBJS = $(BUILD_DIR)/root.o $(BUILD_DIR)/os.o $(BUILD_DIR)/timer.o $(LIB_OBJS)
P1_OBJS = $(BUILD_DIR)/start_p1.o $(BUILD_DIR)/main_p1.o $(LIB_OBJS)
P2_OBJS = $(BUILD_DIR)/start_p2.o $(BUILD_DIR)/main_p2.o $(LIB_OBJS)

# Target por defecto
.PHONY: all clean directories

all: directories $(BIN_DIR)/os.bin $(BIN_DIR)/p1.bin $(BIN_DIR)/p2.bin
	@echo "=== COMPILACIÓN EXITOSA ==="
	@echo "Tus binarios están en la carpeta $(BIN_DIR)/"

# Crear directorios si no existen
directories:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)

# ==========================================
# 1. Compilar Librería Compartida
# ==========================================
$(BUILD_DIR)/%.o: lib/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# ==========================================
# 2. Compilar Sistema Operativo (OS)
# ==========================================
$(BUILD_DIR)/root.o: OS/root.s
	$(AS) $< -o $@

$(BUILD_DIR)/%.o: OS/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/os.bin: $(OS_OBJS)
	@echo "Enlazando OS..."
	$(LD) -T OS/linker.ld $(OS_OBJS) -o $(BUILD_DIR)/os.elf
	$(OBJCOPY) -O binary $(BUILD_DIR)/os.elf $@

# ==========================================
# 3. Compilar Proceso 1 (P1)
# ==========================================
$(BUILD_DIR)/start_p1.o: P1/start.s
	$(AS) $< -o $@

$(BUILD_DIR)/main_p1.o: P1/main.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/p1.bin: $(P1_OBJS)
	@echo "Enlazando P1..."
	$(LD) -T P1/linker.ld $(P1_OBJS) -o $(BUILD_DIR)/p1.elf
	$(OBJCOPY) -O binary $(BUILD_DIR)/p1.elf $@

# ==========================================
# 4. Compilar Proceso 2 (P2)
# ==========================================
$(BUILD_DIR)/start_p2.o: P2/start.s
	$(AS) $< -o $@

$(BUILD_DIR)/main_p2.o: P2/main.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/p2.bin: $(P2_OBJS)
	@echo "Enlazando P2..."
	$(LD) -T P2/linker.ld $(P2_OBJS) -o $(BUILD_DIR)/p2.elf
	$(OBJCOPY) -O binary $(BUILD_DIR)/p2.elf $@

# ==========================================
# Limpieza Total
# ==========================================
clean:
	@echo "Limpiando archivos de compilación..."
	rm -rf $(BUILD_DIR) $(BIN_DIR)