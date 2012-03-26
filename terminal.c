#asm
.text
.org 0x0000
db 0x55
db 0xaa

.org 0x1000		/* kod niech zaczyna siê dalej, poniewa¿ kompilator
			 * wszystkie dane wrzuca od adresu 0. */
#endasm

#include "types.h"
#include "io.h"
#include "screen.h"
#include "keyboard.h"

void keyboardInit();

void main()
{
	int x, y;
	char ch = 0;

	screenInit();
	
	screenPutString("BIOS Terminal v0.1\r\n");
	screenPutString("(C) Copyright 2004 Wojtek Kaniewski <wojtekka@toxygen.net>\r\n\r\n");

	keyboardInit();

	screenPutString("\r\nKeyboard initialized\r\n");

#if 1
	for (;;) {
		char ch = keyboardGetChar();
		
		screenPutHex(ch);
	}
#endif

#if 0
	screenY = 20;

	for (x = 0; x < 80; x++)
		screenPutCharInternal(x, 24, ' ', 0x70);

	for (;;) {
		word i;

		if (ch < 32) {
			screenPutChar('?');
			ch++;
		} else
			screenPutChar(ch++);

		for(i = 0; i < 1000;i++);
	}
#endif

}

#include "init.h"

