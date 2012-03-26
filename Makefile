all:	terminal.bin terminal.com

terminal.s:	terminal.c
	x=`cat .build`; x=$$((x+1)); echo $$x > .build
	bcc -ansi -0 -S -DDATE="\"`date +%D`\"" -DBUILD="\"`cat .build`\"" terminal.c -o terminal.s 

terminal2.s:	terminal.c
	bcc -ansi -0 -S -DCOM -DDATE="\"`date +%D`\"" -DBUILD="\"`cat .build`\"" terminal.c -o terminal2.s

terminal.bin:	terminal.s checksum
	as86 -j terminal.s -b terminal.bin -l terminal.lst
	dd if=/dev/zero of=terminal.bin bs=1 count=1 seek=32767
	./checksum -o 0x0006 terminal.bin

terminal.com:	terminal2.s
	as86 -j terminal2.s -b terminal.tmp
	rm -f terminal2.s
	dd if=terminal.tmp of=terminal.com bs=256 skip=1
	rm -f terminal.tmp

checksum:	checksum.c
	gcc -Wall $< -o $@

clean:
	rm -f *.bin *.com *.s *.o *.lst *~ core checksum

poke:	terminal.bin
	seprog port /dev/ttyS0 chip at29c010 write terminal.bin

.PHONY:	poke clean
	
