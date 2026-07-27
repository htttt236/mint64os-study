all: bootloader.bin

bootloader.bin: bootloader.asm disk.inc print.inc
	@KERNEL32_SEC=$$((`wc -c < ../01.kernel32/kernel32.bin` / 512)); \
	TOTAL_SEC=$$((`cat ../01.kernel32/kernel32.bin ../02.kernel64/kernel64.bin | wc -c` / 512));\
	nasm -f bin -D MACRO_TOTAL_SECTOR_COUNT=$$TOTAL_SEC -D MACRO_KERNEL32_SECTOR_COUNT=$$KERNEL32_SEC $< -o $@

clean:
	rm -f bootloader.bin