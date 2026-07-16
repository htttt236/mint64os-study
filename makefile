all: run

run: disk.img
	qemu-system-x86_64 -hda $<

disk.img: 00.bootloader/bootloader.bin 01.kernel32/kernel32.bin
	cat $^ > $@

00.bootloader/bootloader.bin: 01.kernel32/kernel32.bin
	make -C 00.bootloader

01.kernel32/kernel32.bin:
	make -C 01.kernel32


clean:
	rm -f disk.img
	make -C 00.bootloader clean
	make -C 01.kernel32 clean