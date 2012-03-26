#ifndef __SERIAL_C
#define __SERIAL_C

byte serialRxBuf[8192] = { 0 };	/* bufor wej¶ciowy */
word serialRxHead = 0;		/* wska¼nik zapisu bufora */
word serialRxTail = 0;		/* wska¼nik odczytu bufora */

byte serialTxBuf[256] = { 0 };	/* bufor wyj¶ciowy */
byte serialTxHead = 0;
byte serialTxTail = 0;

word serialPort = 0;			/* adresu u¿ywanego portu */
byte serialInterruptLast = 0;		/* powód ostatniego przerwania */
byte serialIrqMask = 0;			/* maska dla portu 21h */

enum serialSpeed {
	BAUD_1200 = 0x60,
	BAUD_2400 = 0x30,
	BAUD_4800 = 0x18,
	BAUD_7200 = 0x10,
	BAUD_9600 = 0x0c,
	BAUD_19200 = 0x06,
	BAUD_38400 = 0x03,
	BAUD_57600 = 0x02,
	BAUD_115200 = 0x01
};

enum serialParity {
	PARITY_NONE = 0,
	PARITY_ODD = 1,
	PARITY_EVEN = 2
};

byte serialCom = 1;
byte serialSpeed = BAUD_9600;
byte serialBits = 8;
byte serialParity = PARITY_NONE;
byte serialStop = 1;

/*
 * serialInterrupt()
 *
 * funkcja obs³uguj±ca przerwanie portu szeregowego.
 */
void serialInterrupt()
{
#asm
	push ax
	push bx
	push dx
	push es
	push ds

	mov ax,cs
	mov ds,ax

	// zablokuj na chwilê przerwanie portu szeregowego
	in al,#0x21
	or al,[_serialIrqMask]
	out #0x21,al

	// i powiadom 8254 o tym, ¿e obs³u¿yli¶my przerwanie
	mov al,#0x20
	out #0x20,al

siLoop:
	// wczytaj powód przerwania
	mov dx,[_serialPort]
	add dx,#2
	in al,dx

	test al,#1
	jnz siQuit

	// zapamiêtaj go
	and al,#15
	mov [_serialInterruptLast],al

#ifdef DEBUG_SERIAL
	// wy¶wietl rodzaj przerwania
	mov bx,#0xb800
	mov es,bx
	mov ah,al
	add ah,#48
	seg es
	mov 79*2+24*160,ah
#endif

	// i skocz gdzie trzeba
	cmp al,#0
	je siMSR
	cmp al,#6
	je siLSR
	cmp al,#4
	je siDR
	cmp al,#2
	je siTHRE

	jmp siLoop

siMSR:
	// czytamy MSR i zapominamy
	mov dx,[_serialPort]
	add dx,#6
	in al,dx
	jmp siLoop

siLSR:
	// czytamy LSR i zapominamy
	mov dx,[_serialPort]
	add dx,#5
	in al,dx
	jmp siLoop

siDR:
	// odczytaj dan± z rejestru
	mov dx,[_serialPort]
	in al,dx

	// offset zapisu w buforze
	mov bx,[_serialRxHead]
	
	// zapisz do bufora
	mov _serialRxBuf[bx],al

	// serialRxHead++
	inc bx
	and bx,#0x1fff
	mov [_serialRxHead],bx

	// sprawd¼, czy mamy jeszcze ustawiony bit DR
	mov dx,[_serialPort]
	add dx,#5
	in al,dx
	test al,#1
	jnz siDR

	// je¶li THRE jest ustawione, obs³u¿ te¿ to przerwanie
	mov dx,[_serialPort]
	add dx,#5
	in al,dx
	test al,#0x40
	jz siLoop

siTHRE:
	// sprawd¼, czy mamy co¶ w buforze wyj¶ciowym
	xor bx,bx
	mov bl,[_serialTxTail]
	cmp bl,[_serialTxHead]
	je siTHRE2

	// wczytaj dan± z bufora i wy¶lij do portu
	mov al,_serialTxBuf[bx]
	mov dx,[_serialPort]
	out dx,al

	// serialTxTail++
	inc bx
	mov [_serialTxTail],bl

	jmp siLoop

siTHRE2:
	// je¶li w buforze wyj¶ciowym niczego nie ma, wy³±cz przerwanie THRE
	mov dx,[_serialPort]
	inc dx
	in al,dx
	and al,#0xfd
	out dx,al
	jmp siLoop

siQuit:
	// pozwól ponownie na przerwania portu szeregowego
	in al,#0x21
	mov ah,[_serialIrqMask]
	not ah
	and al,ah
	out #0x21,al

	pop ds
	pop es
	pop dx
	pop bx
	pop ax

	iret
#endasm
}

/*
 * serialDataReady()
 *
 * sprawdza, czy s± dostêpne dane w buforze wej¶ciowym.
 */
#define serialDataReady() (serialRxHead != serialRxTail)

/*
 * serialGetChar()
 *
 * wczytuje z bufora portu szeregowego.
 */
byte serialGetChar()
{
	char data;
	
	while (!serialDataReady());

	data = serialRxBuf[serialRxTail];
	serialRxTail = (serialRxTail + 1) & 0x1fff;
	
	return data;
}

/*
 * serialPutChar()
 *
 * zapisuje bufora portu szeregowego.
 */
byte serialPutChar(byte data)
{
#ifdef DEBUG_TRANSFER
	byte a = screenAttr;
	screenAttr = 8;
	screenPutString("[s");
	screenPutHex(data);
	screenPutChar(']');
	screenAttr = a;
#endif

	/* zapisz do bufora */
	serialTxBuf[serialTxHead++] = data;

	// je¶li nie nadaje w tej chwili, w³±cz przerwania
	if (portRead(serialPort+5) & 0x20)
		portWrite(serialPort+1, portRead(serialPort+1) | 2);
}

/*
 * serialPutString()
 *
 * wysy³a ci±g znaków `s'.
 */
void serialPutString(char *s)
{
	while (*s) {
		serialPutChar(*s);
		s++;
	}
}

/*
 * serialPutDec()
 *
 * wysy³a liczbê dziesiêtn± `x' na port szeregowy.
 */
void serialPutDec(byte x)
{
	byte h = 0, t = 0;

	while (x >= 100) {
		h++;
		x -= 100;
	}

	while (x >= 10) {
		t++;
		x -= 10;
	}

	if (h > 0)
		serialPutChar(h + '0');

	if (h > 0 || t > 0)
		serialPutChar(t + '0');

	serialPutChar(x + '0');
}

/*
 * serialSetup()
 *
 * ustawia parametry portu szeregowego, czy¶ci bufory itd.
 */
void serialSetup()
{
	byte irq, tmp;

	/* wyzeruj bufory */
	serialTxHead = 0;
	serialTxTail = 0;
	serialRxHead = 0;
	serialRxTail = 0;

	/* je¶li wcze¶niej by³ wybrany port szeregowy, wy³±cz zg³aszanie
	 * przerwañ. nie bêd± nam potrzebne */
	if (serialPort != 0)
		portWrite(serialPort+1, 0);
	
	/* ustal parametry danego portu szeregowego */
	switch (serialCom) {
		case 1:
			serialPort = 0x3f8;
			irq = 4;
			break;
		case 2:
			serialPort = 0x2f8;
			irq = 3;
			break;
		case 3:
			serialPort = 0x3e8;
			irq = 4;
			break;
		case 4:
			serialPort = 0x2e8;
			irq = 3;
			break;
	}

	/* ustaw parametry */
	portWrite(serialPort+3, 0x80);
	portWrite(serialPort, serialSpeed & 255);
	portWrite(serialPort+1, serialSpeed >> 8);

	tmp = 0;

	switch (serialBits) {
		case 6:
			tmp |= 1;
			break;
		case 7:
			tmp |= 2;
			break;
		case 8:
			tmp |= 3;
			break;
	}

	if (serialStop == 2)
		tmp |= 4;
	
	switch (serialParity) {
		case PARITY_ODD:
			tmp |= 8;
			break;
		case PARITY_EVEN:
			tmp |= 24;
			break;
	}
	
	portWrite(serialPort+3, tmp);

	/* zainstaluj funkcjê obs³ugi przerwañ */
	interruptSet(8 + irq, serialInterrupt);

	/* wy³±cz FIFO, wyczy¶æ */
	portWrite(serialPort+2, 0);

	/* w³±cz zg³aszanie przerwañ danych przychodz±cych i b³êdu transmisji */
	portWrite(serialPort+1, 5);

	/* opró¿nij bufor wej¶ciowy, linie modemu i linie stanu */
	while ((portRead(serialPort+5) & 1))
		portRead(serialPort);
	portRead(serialPort+6);

	/* w³±cz przerwania */
	serialIrqMask = (1 << irq);
	portWrite(0x21, (portRead(0x21) | 0x18) & ~serialIrqMask);

	/* w³acz DTR, RTS, i OUT2 */
	portWrite(serialPort+4, 0x0f);

	/* wci¶niêty ScrollLock w³±cza lokaln± pêtlê */
	if (memoryReadByte(0x40, 0x17) & 0x20)
		portWrite(serialPort+4, portRead(serialPort+4) | 0x10);

	// uaktualnij parametry w linii stanu
	{
		int x, y, a;
		a = screenAttr; x = screenX; y = screenY;
		screenCursorNoUpdate = 1;
		screenX = 1; screenY = 24; screenAttr = 7*16;
		screenPutString("COM");
		screenPutChar(48+serialCom);
		screenPutChar(' ');

		switch (serialSpeed) {
			case BAUD_1200: screenPutString("1200"); break;
			case BAUD_2400: screenPutString("2400"); break;
			case BAUD_4800: screenPutString("4800"); break;
			case BAUD_7200: screenPutString("7200"); break;
			case BAUD_9600: screenPutString("9600"); break;
			case BAUD_19200: screenPutString("19200"); break;
			case BAUD_38400: screenPutString("38400"); break;
			case BAUD_57600: screenPutString("57600"); break;
			case BAUD_115200: screenPutString("115200"); break;
		}

		screenPutChar(' ');
		screenPutChar(48+serialBits);

		switch (serialParity) {
			case PARITY_NONE: screenPutChar('n'); break;
			case PARITY_ODD: screenPutChar('o'); break;
			case PARITY_EVEN: screenPutChar('e'); break;
		}

		screenPutChar(48+serialStop);
		screenPutString("  ");

		screenX = x; screenY = y; screenAttr = a;
		screenCursorNoUpdate = 0;
	}
}

/*
 * serialConfig()
 * 
 * wy¶wietla okno konfiguracji parametrów transmisji.
 */

byte count[5] = { 4, 9, 3, 3, 2 };
byte x1[5] = { 15, 25, 37, 48, 59 };
byte x2[5] = { 20, 31, 42, 55, 64 };
byte x3[5] = { 15, 25, 36, 49, 58 };
byte x4[5] = { 20, 32, 43, 54, 65 };
byte speeds[9] = { BAUD_1200, BAUD_2400, BAUD_4800, BAUD_7200, BAUD_9600, BAUD_19200, BAUD_38400, BAUD_57600, BAUD_115200 };
byte y[5] = { 0, 0, 0, 0, 0 };

void serialConfig()
{
	byte i, j, k, x = 0;

	y[0] = serialCom - 1;
	switch (serialSpeed) {
		case BAUD_1200: y[1] = 0; break;
		case BAUD_2400: y[1] = 1; break;
		case BAUD_4800: y[1] = 2; break;
		case BAUD_7200: y[1] = 3; break;
		case BAUD_9600: y[1] = 4; break;
		case BAUD_19200: y[1] = 5; break;
		case BAUD_38400: y[1] = 6; break;
		case BAUD_57600: y[1] = 7; break;
		case BAUD_115200: y[1] = 8; break;
		default: y[1] = 0; break;
	}
	y[2] = serialBits - 6;
	y[3] = serialParity;
	y[4] = serialStop - 1;

	screenSave();

	screenAttr = 7*16;

	i = 3;
	screenPutStringXY(10, i++, "                                                            ");
	screenPutStringXY(10, i++, "                  Serial Port Configuration                 ");
	screenPutStringXY(10, i++, "                                                            ");
	screenPutStringXY(10, i++, " ---- Port ---- Speed ----- Bits ----- Parity --- Stop ---- ");
	screenPutStringXY(10, i++, "                                                            ");
	screenPutStringXY(10, i++, "      COM1      1200       6 bits       None     1 bit      ");
	screenPutStringXY(10, i++, "      COM2      2400       7 bits       Odd      2 bits     ");
	screenPutStringXY(10, i++, "      COM3      4800       8 bits       Even                ");
	screenPutStringXY(10, i++, "      COM4      7200                                        ");
	screenPutStringXY(10, i++, "                9600                                        ");
	screenPutStringXY(10, i++, "                19200                                       ");
	screenPutStringXY(10, i++, "                38400                                       ");
	screenPutStringXY(10, i++, "                57600                                       ");
	screenPutStringXY(10, i++, "                115200                                      ");
	screenPutStringXY(10, i++, "                                                            ");
	screenPutStringXY(10, i++, "                Enter - Accept   Esc - Cancel               ");
	screenPutStringXY(10, i++, "                                                            ");

	for (;;) {
		for (i = 15; i < 66; i++)
			memoryWriteByte(0xb800, (480+i)*2+1, (i >= x1[x] && i <= x2[x]) ? 7 : 7*16);

		for (j = 8; j < 8+9; j++)
			for (i = 0; i < 5; i++)
				for (k = x3[i]; k <= x4[i]; k++)
					memoryWriteByte(0xb800, (j*80+k)*2+1, (j - 8 == y[i]) ? 7 : 7*16);

		switch (keyboardGetChar()) {
			case 0x4be0:	/* < */
				if (x > 0)
					x--;
				break;
			case 0x4de0:	/* > */
				if (x < 4)
					x++;
				break;
			case 0x48e0:	/* ^ */
				if (y[x] > 0)
					y[x]--;
				break;
			case 0x50e0:	/* v */
				if (y[x] < count[x] - 1)
					y[x]++;
				break;
			case 0x011b:	/* Esc */
				goto escape;
			case 0x1c0d:	/* Enter */
				goto accept;
		}
	}

accept:
	screenRestore();

	serialCom = y[0] + 1;
	serialSpeed = speeds[y[1]];
	serialBits = y[2] + 6;
	serialParity = y[3];
	serialStop = y[4] + 1;
	serialSetup();
	
	return;
	
escape:
	screenRestore();
}

#endif /* __SERIAL_C */
