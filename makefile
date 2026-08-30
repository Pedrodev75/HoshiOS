CC      = gcc
LD      = ld
AS      = nasm
OBJCOPY = objcopy

CFLAGS = -m32 -ffreestanding -fno-pic -fno-pie -fno-stack-protector -nostdlib -nostartfiles -Wall -Wextra -Os -MMD -MP -c
LDFLAGS = -m elf_i386 -T linker.ld
CPPFLAGS = -I. -Iinclude -Iarch/x86 -Idrivers/video -Idrivers/input -Idrivers/timer

BUILD = build

BOOTLOADER = boot/bootloader.asm
KENTRY     = boot/kernel_entry.asm
KERNEL = kernel/kernel.c

CFILES := $(patsubst ./%,%,$(shell find . -type f -name '*.c' \
	! -path './$(BUILD)/*' ! -path './$(KERNEL)'))

ASMFILES := $(patsubst ./%,%,$(shell find . -type f -name '*.asm' \
	! -path './$(BUILD)/*' \
	! -path './$(BOOTLOADER)' \
	! -path './$(KENTRY)'))

OBJS_C   := $(CFILES:%.c=$(BUILD)/%.o)
OBJS_ASM := $(ASMFILES:%.asm=$(BUILD)/%.o)

DEPS := $(BUILD)/kernel.d $(OBJS_C:.o=.d)

# Deve ser igual a KERNEL_SECTORS * 512 em boot/bootloader.asm.
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
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

$(BUILD)/%.o: %.c | $(BUILD)
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

$(BUILD)/%.o: %.asm | $(BUILD)
	mkdir -p $(dir $@)
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
	qemu-system-i386 -m 1024M -drive format=raw,file=build/kernel.img -serial stdio
clean:
	rm -rf $(BUILD)

.PHONY: all compile run clean

-include $(DEPS)
