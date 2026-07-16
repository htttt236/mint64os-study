GCC = x86_64-elf-gcc -ffreestanding -m32 -O2
LD = x86_64-elf-ld -T linker.ld
OBJCOPY = x86_64-elf-objcopy -j .text -j .data -j .rodata -j .bss -S -O binary

all: kernel32.bin

kernel32.bin: temp/kernel32.elf
	$(OBJCOPY) $< $@
	truncate -s %512 $@

temp/kernel32.elf: temp/entrypoint.o temp/main.o
	$(LD) -o $@ $^

temp/main.o: src/main.c
	@mkdir -p temp
	$(GCC) -MMD -c $< -o $@

temp/entrypoint.o: src/entrypoint.asm src/32bit_print.inc src/gdt.inc
	@mkdir -p temp
	nasm -f elf32 -I src/ -o $@ $<

clean:
	rm -f kernel32.bin
	rm -rf temp/*.o temp/*.d temp/*.elf

-include temp/main.d 