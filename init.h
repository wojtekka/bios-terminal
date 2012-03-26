#ifndef __INIT_H
#define __INIT_H

#asm

.org 0xf000

// inicjalizacja sprz�tu

initialize:
	cli

// wyzeruj i zainicjuj kontrolery DMA

	xor ax,ax
	out 0x0d,al
	out 0xda,al

	mov al,#0xc0
	out 0xd6,al
	mov al,#0x00
	out 0xd4,al
	
// skopiuj ca�y segment 0xf000 (ROM) do 0x8000 (RAM), poniewa� kompilator
// i tak wszystko wrzuci� do jednego segmentu.

	cld
	mov cx,#0x8000
	mov es,cx
	mov ax,#0xf000
	mov ds,ax
	xor ax,ax
	rep
	movsw
	
// inicjalizacja stosu i segmentu danych

	mov ax,#0xfffe
	mov sp,ax

	xor ax,ax
	mov ss,ax

	mov ax,#0x8000
	mov ds,ax

// wype�nij wektory przerwa� g�upiutkim handlerem, kt�ry nic nie robi,
// na wypadek, gdyby biosy sprz�tu chcia�y wywo�a� co�, czego nie powinny.

	xor ax,ax
	mov es,ax
	xor di,di

	mov cx,#256
clear_interrupts:
	mov ax,#default_interrupt
	stosw
	mov ax,cs
	stosw
	loop clear_interrupts

// zainicjuj timer: ustaw przerwanie 0x08 na handler sprz�towy

	seg es
	mov 32,#default_hardware_interrupt
	seg es
	mov ax,cs
	mov 34,ax

	mov al,#0x34
	out 0x43,al
	xor ax,ax
	out 0x40,al
	out 0x40,al
	
// zainicjuj kontroler przerwa� 8259

	call 0xf000:_interruptInit

// szukamy innych bios�w, mi�dzy innymi biosu karty graficznej. biosy
// znajduj� si� mi�dzy adresami 0xc0000..0xdffff. co 2kB mamy sygnatury
// o nast�puj�cej postaci:
// 
// bajt 0: 0x55
// bajt 1: 0xaa
// bajt 2: d�ugo�� biosu w 512-bajtowych blokach
// bajt 3: offset pocz�tku biosu
	
	mov ax,#0xc000
	mov ds,ax
	xor si,si
bios_loop:
	cmp [si],#0xaa55
	jne bios_next
	
bios_call:
	push ds 			// segment

	mov ax,si
	add ax,#0x0003
	push ax				// offset
	
	mov bp,sp
	
	db 0xff				// call ss:[bp+0]
	db 0x5e
	db 0

	cli
	add sp,#4			// zdejmij ze stosu

bios_next:
	add si,#0x0800
	cmp si,#0xf800
	jb bios_loop

	mov ax,#0x8000
	mov ds,ax

	xor ax,ax		// wyczy�� segment danych
	mov cx,#0x8000
	mov di,#0x0000
	rep
	stosw

	xor ax,ax
	
	sti			// w��cz ju� przerwania

	call 0xf000:_main	// skocz do main()

stop:	jmp stop 		// je�li wr�ci, zatrzymaj program

// domy�lna obs�uga przerwania, kt�ra w zasadzie nic nie robi 

default_interrupt:
	iret

// domy�lna obs�uga przerwania sprz�towego, kt�ra zeruje PIC

default_hardware_interrupt:
	push ax
	mov al,#0x20
	out 0x20,al
	pop ax
	iret

// pierwszy adres, pod kt�ry skacze procesor po resecie

.org 0xfff0
jmp 0xf000:initialize

// data kompilacji w postaci MM/DD/YY, sygnatura modelu komputera 

.org 0xfff5
.ascii DATE

.org 0xfffe
db 0xfc		// komputer klasy AT 
db 0xc7		// suma kontrolna

#endasm

#endif /* __INIT_H */
