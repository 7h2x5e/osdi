TOOLCHAIN_PREFIX = aarch64-linux-gnu-
CC = $(TOOLCHAIN_PREFIX)gcc
LD = $(TOOLCHAIN_PREFIX)ld
OBJCPY = $(TOOLCHAIN_PREFIX)objcopy

CFLAGS = -mcpu=cortex-a53+crc -Wall -c -ffreestanding -O0 -nostdinc -nostdlib -nostartfiles
CFLAGS += -g
CFLAGS += -I .

GIT_HOOKS := ./git/hooks/applied

include kernel/Makefile

.PHONY: all clean asm run debug directories

all: $(GIT_HOOKS) kernel8.img

$(GIT_HOOKS):
	@scripts/install-git-hooks
	@echo

asm: all
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial pty -drive if=sd,file=disk.img,format=raw -monitor stdio -d in_asm

run: all
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial pty -drive if=sd,file=disk.img,format=raw -monitor stdio
	
debug: all
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial pty -drive if=sd,file=disk.img,format=raw -monitor stdio -S -s

clean:
	rm -rf kernel8.*
	rm -rf kernel/*.o kernel/*.elf
	rm -rf lib/*.o
	rm -rf user/*.o user/*.elf user/*.bin user/*.img
