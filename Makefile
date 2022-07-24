TOOLCHAIN_PREFIX = aarch64-linux-gnu-
CC = $(TOOLCHAIN_PREFIX)gcc
LD = $(TOOLCHAIN_PREFIX)ld
OBJCPY = $(TOOLCHAIN_PREFIX)objcopy

GIT_HOOKS := ./git/hooks/applied
SRC_DIR = src
OUT_DIR = build
ASM_OUT_DIR = $(OUT_DIR)/asm

LINKER_FILE = $(SRC_DIR)/linker.ld
ASM_SRCS = $(wildcard $(SRC_DIR)/*.S)
ASM_OBJS = $(ASM_SRCS:$(SRC_DIR)/%.S=$(ASM_OUT_DIR)/%.o)
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OUT_DIR)/%.o)

CFLAGS = -Wall -I include -c -ffreestanding -O0 -nostdinc -nostdlib -nostartfiles -g
.PHONY: all clean asm run debug directories

all: $(GIT_HOOKS) directories kernel8.img

$(GIT_HOOKS):
	@scripts/install-git-hooks
	@echo

$(ASM_OUT_DIR)/%.o: $(SRC_DIR)/%.S
	$(CC) $(CFLAGS) $< -o $@

$(OUT_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $< -o $@

kernel8.img: $(OBJS) $(ASM_OBJS)
	$(LD) $(ASM_OBJS) $(OBJS) -T $(LINKER_FILE) -o kernel8.elf
	$(OBJCPY) -O binary kernel8.elf kernel8.img

asm: all
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial pty -monitor stdio -d in_asm

run: all
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial pty -monitor stdio
	
debug: all
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial pty -monitor stdio -S -s

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

$(ASM_OUT_DIR):
	mkdir -p $(ASM_OUT_DIR)

directories: $(OUT_DIR) $(ASM_OUT_DIR)

clean:
	rm -rf $(OUT_DIR)/* $(ASM_OUT_DIR) kernel8.*
