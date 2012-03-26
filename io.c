#ifndef __IO_C
#define __IO_C

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

void memoryWriteByte(word segment, word offset, byte x)
{
#asm
	push bp
	mov bp,sp
	
	push ax
	push di
	push es
	
	mov ax,4[bp]
	mov es,ax
	mov ax,6[bp]
	mov di,ax
	mov al,8[bp]
	seg es
	mov [di],al

	pop es
	pop di
	pop ax
	pop bp
#endasm
}

void memoryWriteWord(word segment, word offset, word x)
{
#asm
	push bp
	mov bp,sp
	
	push ax
	push di
	push es
	
	mov ax,4[bp]
	mov es,ax
	mov ax,6[bp]
	mov di,ax
	mov ax,8[bp]
	seg es
	mov [di],ax

	pop es
	pop di
	pop ax
	pop bp
#endasm
}

word memoryReadByte(word segment, word offset)
{
#asm
	push bp
	mov bp,sp
	
	push di
	push es
	
	mov ax,4[bp]
	mov es,ax
	mov ax,6[bp]
	mov di,ax
	seg es
	mov al,[di]

	pop es
	pop di
	pop bp
#endasm
}

word memoryReadWord(word segment, word offset)
{
#asm
	push bp
	mov bp,sp
	
	push di
	push es
	
	mov ax,4[bp]
	mov es,ax
	mov ax,6[bp]
	mov di,ax
	seg es
	mov ax,[di]

	pop es
	pop di
	pop bp
#endasm
}

/*
 * interruptSet()
 *
 * ustawia funkcjê obs³ugi przerwania.
 */
void interruptSet(byte number, word offset)
{
	word segCS = 0;

#asm
	push ax
	mov ax,cs
	mov -6[bp],ax
	pop ax
#endasm

	memoryWriteWord(0x0000, number * 4, offset);
	memoryWriteWord(0x0000, number * 4 + 2, segCS);
}

#endif /* __IO_C */
