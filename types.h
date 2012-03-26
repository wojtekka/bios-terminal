#ifndef __TYPES_H
#define __TYPES_H 1

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;
typedef byte spinlock;

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

#endif /* __TYPES_H */

