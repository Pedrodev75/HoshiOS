CC      = gcc
LD      = ld
AS      = nasm
OBJCOPY = objcopy

CFLAGS  = -m32 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -nostdlib -nostartfiles -c
LDFLAGS = -m elf_i386 -T linker.ld

BUILD = build

BOOTLOADER = core/bootloader.asm
KENTRY     = core/kernel_entry.asm
KERNEL     = kernel.c

CFILES   := $(wildcard include/*.c)
ASMFILES := $(wildcard include/*.asm)

OBJS_C   := $(patsubst include/%.c,$(BUILD)/%.o,$(CFILES))
OBJS_ASM := $(patsubst include/%.asm,$(BUILD)/%.o,$(ASMFILES))

# Deve ser igual a KERNEL_SECTORS * 512 em core/bootloader.asm.
MAX_KERNEL_SIZE = 24576

all: compile

compile: $(BUILD)/kernel.img

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/bootloader.bin: $(BOOTLOADER) | $(BUILD)
	$(AS) -f bin $< -o $@

$(BUILD)/kernel_entry.o: $(KENTRY) | $(BUILD)
	$(AS) -f elf32 $< -o $@

$(BUILD)/kernel.o: $(KERNEL) | $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/%.o: include/%.c | $(BUILD)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/%.o: include/%.asm | $(BUILD)
	$(AS) -f elf32 $< -o $@

$(BUILD)/kernel.elf: $(BUILD)/kernel_entry.o $(BUILD)/kernel.o $(OBJS_C) $(OBJS_ASM)
	$(LD) $(LDFLAGS) $^ -o $@

$(BUILD)/kernel.bin: $(BUILD)/kernel.elf
	$(OBJCOPY) -O binary $< $@
	@size=$$(stat -c%s $@); \
	if [ $$size -gt $(MAX_KERNEL_SIZE) ]; then \
		echo "ERRO: kernel.bin tem $$size bytes; limite: $(MAX_KERNEL_SIZE)."; \
		exit 1; \
	fi

$(BUILD)/kernel.img: $(BUILD)/bootloader.bin $(BUILD)/kernel.bin
	cat $^ > $@
	# Mantem os 49 primeiros setores de boot/kernel e reserva espaco de dados.
	truncate -s 1048576 $@

run: $(BUILD)/kernel.img
	qemu-system-i386 -drive format=raw,file=build/kernel.img

clean:
	rm -rf $(BUILD)
