#undef DEBUG_KEYBOARD
#undef DEBUG_SERIAL
#undef DEBUG_TRANSFER

#include "loader.asm"
#include "types.h"
#include "io.c"
#include "screen.c"
#include "keyboard.c"
#include "serial.c"
#include "vt100.c"

int main()
{
	screenInit();

	screenPutString("BIOS Terminal v1.0 build " BUILD "\r\n(C) Copyright 2004 Wojtek Kaniewski <wojtekka@toxygen.net>\r\nPress F12 to configure serial port\r\n\r\n");

	serialSetup();

	for (;;) {

#ifdef DEBUG_SERIAL
		byte x, y, a;

		x = screenX; y = screenY; a = screenAttr;
		screenX = 30; screenY = 24; screenAttr = 7*16;
		screenCursorNoUpdate = 1;
		screenPutString(" s:");
		screenPutHex((char)((char) serialRxHead - (char) serialRxTail));
		screenPutString(" i:");
		screenPutHex(portRead(0x21));
		screenPutString(" t:");
		screenPutHex(serialRxTail >> 8);
		screenPutHex(serialRxTail &255);
		screenPutString("/h:");
		screenPutHex(serialRxHead >> 8);
		screenPutHex(serialRxHead &255);
		screenPutString(",");
		screenPutHex(portRead(serialPort+1));
		screenPutString(",");
		screenPutHex(serialInterruptLast); // +2
		screenPutString(",");
		screenPutHex(portRead(serialPort+3));
		screenPutString(",");
		screenPutHex(portRead(serialPort+4));
		screenPutString(",");
		screenPutHex(portRead(serialPort+5));
		screenPutString(",");
		screenPutHex(portRead(serialPort+6));
		screenPutString(" k:");
		screenCursorNoUpdate = 0;
		screenX = x; screenY = y; screenAttr = a;

		screenCursorUpdate();
#endif // DEBUG_SERIAL

		if (keyboardPressed()) {
			char *seq = vtGetChar();

			while (*seq) {
				serialPutChar(*seq);
				seq++;
			}
		}

		while (serialDataReady()) {
			char ch = serialGetChar();

			vtPutChar(ch);
		}
	}
}
