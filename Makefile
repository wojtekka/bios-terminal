default:	clean bios.bin

terminal.bin:	terminal.s
	as86 -j -O terminal.s -b terminal.bin -l terminal.lst

terminal.s:	terminal.c
	bcc -ansi -C-c -0 -S -DDATE="\"`date +%D`\"" terminal.c -o terminal.s 
	mv terminal.s terminal.s-
	sed -e 's/^\.bss/.data/' terminal.s- > terminal.s
	rm -f terminal.s-

original.tmp:	terminal.bin
	dd if=/dev/zero of=zero64k.bin bs=65536 count=1 
	cat zero64k.bin terminal.bin > original.tmp
	rm -f zero64k.bin

bios.bin:	original.tmp
	lha -ao5 bios.bin original.tmp
	rm -f original.tmp
	dd if=images/bootblock.bin of=bios.bin bs=4096 seek=26
	printf '\x00\x00\x00\x50' | dd of=bios.bin bs=1 seek=15 conv=notrunc

.PHONY:	clean

clean:
	rm -f *.bin *.s *.o *.lst *~ core

