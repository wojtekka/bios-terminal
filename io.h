#ifndef __IO_H
#define __IO_H

void memset(void *ptr, byte value, word size)
{
	byte *ptr2 = ptr;
	
	while (size--) {
		*ptr2 = value;
		ptr2++;
	}
}

byte codeReadByte(word offset)
{
#asm
	push bp
	mov bp,sp
	push bx
	mov bx,4[bp]
	seg cs
	mov al,byte ptr [bx]
	pop bx
	pop bp
#endasm
}

void portWrite(word port, byte value)
{
#asm
	push bp
	mov bp,sp
	
	push dx
	push ax
	
	mov dx,4[bp]
	mov al,6[bp]
	out dx,al

	pop ax
	pop dx

	pop bp
#endasm
}

byte portRead(word port)
{
#asm
	push bp
	mov bp,sp
	
	push dx
	
	mov dx,4[bp]
	in al,dx

	pop dx

	pop bp
#endasm
}

void interruptSet(byte number, word offset)
{
	memoryWriteWord(0x0000, number * 4, offset);
	memoryWriteWord(0x0000, number * 4 + 2, 0xf000);
}

void interruptInit()
{
	/* inicjalizacja pierwszego 8259, bez trybu kaskadowego */
	portWrite(0x20, 0x10);
	portWrite(0x21, 0x08);
	portWrite(0x21, 0x04);
	portWrite(0x21, 0x01);

	/* maskowanie przerwañ, odblokowanie przerwania klawiatury */
	portWrite(0x21, 0xff - 0x02);
}

void interruptEnd()
{
	portWrite(0x20, 0x20);
}

#endif /* __IO_H */
