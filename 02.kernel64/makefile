GCC = x86_64-elf-gcc -ffreestanding -m64 -O2 -mno-red-zone -mcmodel=large \
	-fno-pic -fno-pie -fno-builtin -fno-tree-vectorize
LD = x86_64-elf-ld -T linker.ld
OBJCOPY = x86_64-elf-objcopy -j .text -j .data -j .rodata -j .bss -S -O binary

ENTRY_OBJ = temp/entrypoint.o

C_SOURCES = $(wildcard src/*.c)
C_OBJECTS = $(patsubst src/%.c, temp/%.o, $(C_SOURCES))

ASM_SOURCES = $(wildcard src/*.asm)
ASM_OBJECTS = $(filter-out $(ENTRY_OBJ), $(patsubst src/%.asm, temp/%.o, $(ASM_SOURCES)))


all: kernel64.bin

kernel64.bin: temp/kernel64.elf
	$(OBJCOPY) $< $@
	truncate -s %512 $@

temp/kernel64.elf: $(ENTRY_OBJ) $(ASM_OBJECTS) $(C_OBJECTS)
	$(LD) -o $@ $^

temp/%.o: src/%.c
	@mkdir -p temp
	$(GCC) -MMD -c $< -o $@

temp/%.o: src/%.asm
	@mkdir -p temp
	nasm -MD $(@:.o=.d) -f elf64 -I src/ -o $@ $<

clean:
	rm -f kernel64.bin
	rm -rf temp/*.o temp/*.d temp/*.elf

-include $(C_OBJECTS:.o=.d) $(ASM_OBJECTS:.o=.d) $(ENTRY_OBJ:.o=.d)