KERNEL_LINKER_FILE = kernel/linker.ld
USER_LINKER_FILE = user/linker.ld

LIB_SRC = $(wildcard lib/*.c)
USER_SRC = $(wildcard user/*.c)
USER_ASM_SRC = $(wildcard user/*.S)
KERNEL_SRC = $(wildcard kernel/*.c)
KERNEL_ASM_SRC = $(wildcard kernel/*.S)

LIB_OBJ = $(LIB_SRC:lib/%.c=lib/%.o)
USER_OBJS = $(USER_SRC:user/%.c=user/%.o)
USER_OBJS += $(USER_ASM_SRC:user/%.S=user/%.asm.o)
KERNEL_OBJS = $(KERNEL_SRC:kernel/%.c=kernel/%.o)
KERNEL_OBJS += $(KERNEL_ASM_SRC:kernel/%.S=kernel/%.asm.o)

USER_BIN = user/user.bin

# library
lib/%.o: lib/%.c
	$(CC) $(CFLAGS) $< -c -o $@

# user
user/%.o: user/%.c
	$(CC) $(CFLAGS) -fno-zero-initialized-in-bss $< -c -o $@

user/%.asm.o: user/%.S
	$(CC) $(CFLAGS) -fno-zero-initialized-in-bss $< -c -o user/$*.asm.o

user/user.elf: $(LIB_OBJ) $(USER_OBJS)
	$(LD) -T $(USER_LINKER_FILE) $^ -o $@

user/user.img: user/user.elf
	$(OBJCPY) -O binary $< $@

# creates an linkable binary object
user/user.bin: user/user.img 
	$(LD) -r -b binary $< -o $@

# kernel
kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) $< -c -o $@

kernel/%.asm.o: kernel/%.S
	$(CC) $(CFLAGS) $< -c -o kernel/$*.asm.o

kernel/kernel8.elf: $(KERNEL_OBJS) $(LIB_OBJ) $(USER_BIN)
	$(LD) -T $(KERNEL_LINKER_FILE) $^ -o $@

kernel8.img: kernel/kernel8.elf
	$(OBJCPY) -O binary $< $@
