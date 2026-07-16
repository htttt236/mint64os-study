all: bootloader.bin

bootloader.bin: bootloader.asm disk.inc print.inc
	@SECTORS=$$((`wc -c < ../01.kernel32/kernel32.bin` / 512));\
	nasm -f bin -D KERNEL_SECTOR_COUNT=$$SECTORS $< -o $@

clean:
	rm -f bootloader.bin