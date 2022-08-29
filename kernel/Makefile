LINKER_FILE = kernel/linker.ld

KERNEL_SRC = $(wildcard kernel/*.c)
KERNEL_ASM_SRC = $(wildcard kernel/*.S)
ULIB_SRC = $(wildcard lib/*.c)
USER_SRC = $(wildcard user/*.c)

KERNEL_OBJ = $(KERNEL_SRC:kernel/%.c=kernel/%.o)
KERNEL_ASM_OBJ = $(KERNEL_ASM_SRC:kernel/%.S=kernel/%.asm.o)
ULIB_OBJ = $(ULIB_SRC:lib/%.c=lib/%.o)
USER_OBJ = $(USER_SRC:user/%.c=user/%.o)

kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) $< -c -o $@

kernel/%.asm.o: kernel/%.S
	$(CC) $(CFLAGS) $< -c -o kernel/$*.asm.o

lib/%.o: lib/%.c
	$(CC) $(CFLAGS) $< -c -o $@

user/%.o: user/%.c
	$(CC) $(CFLAGS) $< -c -o $@

kernel/kernel8.elf: $(KERNEL_OBJ) $(KERNEL_ASM_OBJ) $(ULIB_OBJ) $(USER_OBJ)
	$(LD) -T $(LINKER_FILE) $^ -o $@

kernel8.img: kernel/kernel8.elf
	$(OBJCPY) -O binary $< $@
