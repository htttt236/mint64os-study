Csourcefiles = $(wildcard src/*.c)
Cobjectfiles = $(patsubst src/%.c, temp/%.o, $(Csourcefiles))

Asourcefiles = $(wildcard src/*.asm)
Aobjectfiles = $(patsubst src/%.asm, temp/%.o, $(Asourcefiles))

GCC = x86_64-elf-gcc -ffreestanding -m32 -O2
LD = x86_64-elf-ld -T linker.ld
OBJCOPY = x86_64-elf-objcopy -j .text -j .data -j .rodata -j .bss -S -O binary


all: kernel32.bin

kernel32.bin: kernel32.elf
	$(OBJCOPY) $< $@

kernel32.elf: $(Aobjectfiles) $(Cobjectfiles)
	$(LD) -o $@ $^

temp/%.o: src/%.c
	@mkdir -p temp
	$(GCC) -c $< -o $@

temp/%.o: src/%.asm
	@mkdir -p temp
	nasm -f elf32 -o $@ $<

clean:
	rm -f kernel32.bin kernel32.elf
	rm -rf temp/*.o