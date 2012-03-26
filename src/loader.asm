#asm

.data
	.space 12000	// przesuñ dane o 16kB bajtów w binarce

.text

#ifdef COM
	.org 0x100

	mov ax,cs
	mov ds,ax

	jmp _main
	
#else
	db 0x55		// sygnatura BIOSu
	db 0xaa
	db 32768/512	// BIOS ma 32kB

	jmp _init

	db 0x00		// miejsce na sumê kontroln±, offset 0x0006

// skopiuj ca³y segment d000: (ROM) do 8000: (RAM), poniewa¿ kompilator
// i tak wszystko wrzuci³ do jednego segmentu.

_init:

	cld
	xor si,si
	xor di,di
	mov cx,#0x8000
	mov es,cx
	mov ax,cs
	mov ds,ax
	rep
	movsw
	
// inicjalizacja stosu (0000:fffe) i segmentu danych

	mov ax,#0xfffe
	mov sp,ax
	xor ax,ax
	mov ss,ax

	mov ax,#0x8000
	mov ds,ax

	jmp 0x8000:_main

#endif

#endasm
