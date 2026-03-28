# ==========================================
# Configuración de Directorios
# ==========================================
BUILD_DIR = build
BIN_DIR = bin
BEAGLE_BUILD_DIR = Beagle/build
BEAGLE_BIN_DIR = Beagle/bin

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
# Archivos Objeto QEMU (se guardarán en build/)
# ==========================================
LIB_OBJS = $(BUILD_DIR)/uart.o $(BUILD_DIR)/stdio.o $(BUILD_DIR)/string.o
OS_OBJS = $(BUILD_DIR)/root.o $(BUILD_DIR)/os.o $(BUILD_DIR)/timer.o $(LIB_OBJS)
P1_OBJS = $(BUILD_DIR)/start_p1.o $(BUILD_DIR)/main_p1.o $(LIB_OBJS)
P2_OBJS = $(BUILD_DIR)/start_p2.o $(BUILD_DIR)/main_p2.o $(LIB_OBJS)

# ==========================================
# Archivos Objeto Beagle (se guardarán en Beagle/build/)
# ==========================================
BEAGLE_LIB_OBJS = $(BEAGLE_BUILD_DIR)/uart.o $(BEAGLE_BUILD_DIR)/stdio.o $(BEAGLE_BUILD_DIR)/string.o
BEAGLE_OS_OBJS = $(BEAGLE_BUILD_DIR)/root.o $(BEAGLE_BUILD_DIR)/os.o $(BEAGLE_BUILD_DIR)/timer.o $(BEAGLE_LIB_OBJS)
BEAGLE_P1_OBJS = $(BEAGLE_BUILD_DIR)/start_p1.o $(BEAGLE_BUILD_DIR)/main_p1.o $(BEAGLE_LIB_OBJS)
BEAGLE_P2_OBJS = $(BEAGLE_BUILD_DIR)/start_p2.o $(BEAGLE_BUILD_DIR)/main_p2.o $(BEAGLE_LIB_OBJS)

# Target por defecto
.PHONY: all clean directories beagle

all: directories $(BIN_DIR)/os.bin $(BIN_DIR)/p1.bin $(BIN_DIR)/p2.bin
	@echo "=== COMPILACIÓN EXITOSA (QEMU) ==="
	@echo "Tus binarios están en la carpeta $(BIN_DIR)/"

# Target para BeagleBone Black
beagle: beagle-directories $(BEAGLE_BIN_DIR)/os.bin $(BEAGLE_BIN_DIR)/p1.bin $(BEAGLE_BIN_DIR)/p2.bin
	@echo "=== COMPILACIÓN EXITOSA (BEAGLE) ==="
	@echo "Tus binarios están en la carpeta $(BEAGLE_BIN_DIR)/"
	@echo "=== DIRECCIONES PARA COOLTERM ==="
	@echo "1. $(BEAGLE_BIN_DIR)/os.bin"
	@echo "2. $(BEAGLE_BIN_DIR)/p1.bin"
	@echo "3. $(BEAGLE_BIN_DIR)/p2.bin"
	@echo "Usa 'loady' en CoolTerm para subir cada archivo"

# Crear directorios si no existen
directories:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)

beagle-directories:
	@mkdir -p $(BEAGLE_BUILD_DIR)
	@mkdir -p $(BEAGLE_BIN_DIR)

# ==========================================
# 1. Compilar Librería Compartida (QEMU)
# ==========================================
$(BUILD_DIR)/%.o: lib/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# ==========================================
# 1. Compilar Librería Compartida (Beagle)
# ==========================================
$(BEAGLE_BUILD_DIR)/%.o: Beagle/lib/%.c
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
# 2. Compilar Sistema Operativo (Beagle)
# ==========================================
$(BEAGLE_BUILD_DIR)/root.o: Beagle/OS/root.s
	$(AS) $< -o $@

$(BEAGLE_BUILD_DIR)/%.o: Beagle/OS/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BEAGLE_BIN_DIR)/os.bin: $(BEAGLE_OS_OBJS)
	@echo "Enlazando OS (Beagle)..."
	$(LD) -T Beagle/OS/linker.ld $(BEAGLE_OS_OBJS) -o $(BEAGLE_BUILD_DIR)/os.elf
	$(OBJCOPY) -O binary $(BEAGLE_BUILD_DIR)/os.elf $@

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
# 3. Compilar Proceso 1 (Beagle)
# ==========================================
$(BEAGLE_BUILD_DIR)/start_p1.o: Beagle/P1/start.s
	$(AS) $< -o $@

$(BEAGLE_BUILD_DIR)/main_p1.o: Beagle/P1/main.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BEAGLE_BIN_DIR)/p1.bin: $(BEAGLE_P1_OBJS)
	@echo "Enlazando P1 (Beagle)..."
	$(LD) -T Beagle/P1/linker.ld $(BEAGLE_P1_OBJS) -o $(BEAGLE_BUILD_DIR)/p1.elf
	$(OBJCOPY) -O binary $(BEAGLE_BUILD_DIR)/p1.elf $@

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
# 4. Compilar Proceso 2 (Beagle)
# ==========================================
$(BEAGLE_BUILD_DIR)/start_p2.o: Beagle/P2/start.s
	$(AS) $< -o $@

$(BEAGLE_BUILD_DIR)/main_p2.o: Beagle/P2/main.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BEAGLE_BIN_DIR)/p2.bin: $(BEAGLE_P2_OBJS)
	@echo "Enlazando P2 (Beagle)..."
	$(LD) -T Beagle/P2/linker.ld $(BEAGLE_P2_OBJS) -o $(BEAGLE_BUILD_DIR)/p2.elf
	$(OBJCOPY) -O binary $(BEAGLE_BUILD_DIR)/p2.elf $@

# ==========================================
# Limpieza Total
# ==========================================
clean:
	@echo "Limpiando archivos de compilación..."
	rm -rf $(BUILD_DIR) $(BIN_DIR) $(BEAGLE_BUILD_DIR) $(BEAGLE_BIN_DIR)

qemu:
	./QEMU/build_and_run.sh