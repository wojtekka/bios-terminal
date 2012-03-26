#ifndef __KEYBOARD_H
#define __KEYBOARD_H

#include "scancodes.h"

#define keyboardReadInput() portRead(0x60)
#define keyboardReadStatus() portRead(0x64)
#define keyboardWriteOutput(val) portWrite(0x60, val)
#define keyboardWriteCommand(val) portWrite(0x64, val)

byte keyboardPressed[128] = { 0 };
byte keyboardBuffer[256] = { 0 };
byte keyboardPtrWrite = 0;
byte keyboardPtrRead = 0;

byte scancode = 0, ascii = 0;

void keyboardPutChar(byte ch)
{
	if (keyboardPtrWrite != keyboardPtrRead - 1) {
		keyboardBuffer[keyboardPtrWrite] = ch;
		keyboardPtrWrite++;
	}
}

void keyboardInterrupt()
{
#asm
	cli

	pusha
#endasm
	
	ascii = 0;
	scancode = keyboardReadInput();

	if (scancode & 0x80)
		keyboardPressed[scancode & 0x7f] = 0;
	else {
		keyboardPressed[scancode] = 1;

		if (scancode == 43)
			ascii = 13;
		if (scancode == 2)
			ascii = '1';
		if (scancode == 17)
			ascii = 'q';
	}

	if (ascii)
		keyboardPutChar(ascii);

	interruptEnd();

#asm
	pop bp			; kompilator wstawia push bp na pocz±tku

	popa
	
	sti

	iret
#endasm
}

byte keyboardGetChar()
{
	while (keyboardPtrWrite == keyboardPtrRead)
		;

	return keyboardBuffer[keyboardPtrRead++];
}

byte keyboardKeyPressed()
{
	return (keyboardPtrRead != keyboardPtrWrite);
}

void keyboardError(char *msg)
{
	screenPutString("\r\nKeyboard error: ");
	screenPutString(msg);
	screenPutString("\r\n");

	for (;;);
}

/*
byte keyboardHandleEvent()
{
	byte status;
	word timeout = 65535;

	while (timeout-- && ((status = keyboardReadStatus()) & 0x01)) {
		byte scancode;
		
		scancode = keyboardReadInput();

		if ((status & 0x1f)) {
			keyboardHandleScancode();
		}

		status = keyboardReadStatus();
	}

	if (!timeout)
		keyboardError("timeout");
	
	return status;
}
*/

void keyboardWait()
{
	word timeout;

	timeout = 65535;

	while (timeout--) {
		byte status = keyboardReadStatus();

		if (!(status & 0x02))
			return;
	}
}

void keyboardCommand(byte val)
{
	screenPutString("keyboardCommand(0x");
	screenPutHex(val);
	screenPutString("):\r\n");

	keyboardWait();
	keyboardWriteCommand(val);
}

void keyboardWrite(byte val)
{
	screenPutString("keyboardWrite(0x");
	screenPutHex(val);
	screenPutString("):\r\n");

	keyboardWait();
	keyboardWriteOutput(val);
}

byte keyboardRead()
{
	word timeout = 65535;

	screenPutString("keyboardRead(): ");

	while (timeout--) {
		byte status, data;

		status = keyboardReadStatus();

		if (status & 0xe0)
			keyboardError("transmition error");

		if (!(status & 0x01))
			continue;

		data = keyboardReadInput();

		screenPutHex(data);
		screenPutString("\r\n");
		
		return data;
	}

	keyboardError("timeout");
}

void keyboardInit()
{
	word timeout;

	/* wyczy¶æ bufory */

	memset(keyboardBuffer, 0, sizeof(keyboardBuffer));
	memset(keyboardPressed, 0, sizeof(keyboardPressed));

	timeout = 65535;
	
	while (timeout--) {
		byte status = keyboardReadStatus();

		if ((status & 0x01))
			keyboardReadInput();

		if (!(status & 0x02))
			break;
	}
	
	if (!timeout)
		keyboardError("timeout #1");

	/* selftest kontrolera */

	keyboardCommand(0xaa);

	if (keyboardRead() != 0x55)
		keyboardError("controller selftest failed");

	/* test interfejsu kontrolera */

	keyboardCommand(0xab);

	if (keyboardRead() != 0x00)
		keyboardError("interface test failed");

	/* w³±cz zegar klawiatury */

	keyboardWriteCommand(0xae);
	keyboardWriteCommand(0xa8);

	/* zresetuj klawiaturê */

	keyboardWrite(0xff);

	if (keyboardRead() != 0xfa)
		keyboardError("no ack #2");

	if (keyboardRead() != 0xaa)
		keyboardError("reset failed");

	/* wy³±cz klawiaturê i ustaw domy¶lne warto¶ci */

	keyboardWrite(0xf5);

	if (keyboardRead() != 0xfa)
		keyboardError("no ack #3");

	/* w³±cz przerwania */
	
	keyboardCommand(0x60);
	keyboardWrite(0x61);

	/* w³±cz klawiaturê */

	keyboardWrite(0xf4);

	if (keyboardRead() != 0xfa)
		keyboardError("no ack #4");

	/* ustaw obs³ugê przerwania */

	interruptSet(0x09, keyboardInterrupt);
}

#endif /* __KEYBOARD_H */
