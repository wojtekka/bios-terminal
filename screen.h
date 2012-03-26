#ifndef __SCREEN_H
#define __SCREEN_H

byte screenX = 0;	/* aktualna kolumna kursora (0..79) */

byte screenY = 0;	/* aktualny wiersz kursora (0..24) */

byte screenAttr = 7;	/* aktualne atrybyty tekstu. dolne 4 bity to kolor
			 * tekstu, górne 3 to kolor t³a, najstarszy to
			 * atrybut mrugania. */

/*
 * screenCursorUpdate()
 *
 * przesuwa kursor ekranowy do wspó³rzêdnych (screenX,screenY). funkcja
 * powinna byæ wywo³ywana po ka¿dej operacji zmieniaj±cej zawarto¶æ ekranu.
 */
void screenCursorUpdate()
{
#asm
	push ax
	push bx
	push dx

	mov ah,#0x02
	xor bx,bx
	mov dl,[_screenX]
	mov dh,[_screenY]

	int #0x10

	pop dx
	pop bx
	pop ax
#endasm
}

/*
 * screenPutCharInternal()
 *
 * umieszcza w buforze ekranu znak we wspó³rzêdnych (x,y) i atrybucie `attr'.
 */
void screenPutCharInternal(byte x, byte y, char ch, byte attr)
{
	word ptr = (y * 80 + x) << 1;

	memoryWriteByte(0xb800, ptr++, ch);
	memoryWriteByte(0xb800, ptr, attr);
}

/*
 * screenClearLine()
 *
 * czy¶ci podan± liniê ustawiaj±c atrybut linii na `screenAttr'.
 */
void screenClearLine(byte line)
{
	word ptr, i;

	ptr = line * 160;

	for (i = 0; i < 80; i++) {
		memoryWriteByte(0xb800, ptr++, ' ');
		memoryWriteByte(0xb800, ptr++, screenAttr);
	}
}

/*
 * screenClear()
 *
 * czy¶ci ca³y ekran atrybutem `screenAttr', ustawia kursor na pocz±tek
 * ekranu.
 */
void screenClear()
{
	word ptr, i;

	ptr = 0;
	
	for (i = 0; i < 2000; i++) {
		memoryWriteByte(0xb800, ptr++, ' ');
		memoryWriteByte(0xb800, ptr++, screenAttr);
	}
}

/*
 * screenScroll()
 *
 * przesuwa czê¶æ ekranu od linii `from' zawieraj±c± `lines' linii do linii
 * `to'. pozosta³ej czê¶ci _nie_czy¶ci_.
 */
void screenScroll(byte from, byte to, byte lines)
{
	word ptrFrom, ptrTo, count;
	byte tmp;

	ptrFrom = from * 160;
	ptrTo = to * 160;
	count = lines * 160;

	if (from > to) {
		while (count--) {
			tmp = memoryReadByte(0xb800, ptrFrom++);
			memoryWriteWord(0xb800, ptrTo++, tmp);
		}
	} else {
		ptrFrom += count;
		ptrTo += count;

		while (count--) {
			tmp = memoryReadByte(0xb800, --ptrFrom);
			memoryWriteWord(0xb800, --ptrTo, tmp);
		}
	}
}

void screenCursorLeft()
{
	if (screenX > 0)
		screenX--;
	else {
		if (screenY > 0) {
			screenY--;
			screenX = 79;
		}
	}

	screenCursorUpdate();
}

void screenCursorRight(int scroll)
{
	screenX++;

	if (screenX >= 80) {
		screenX = 0;


		if (!scroll) {
			if (screenY < 23)
				screenY++;
		} else {
			screenY++;

			if (screenY >= 24) {
				screenScroll(1, 0, 23);
				screenClearLine(23);
				screenY = 23;
			}
		}
	}

	screenCursorUpdate();
}

void screenCursorUp(int scroll)
{
	if (screenY > 0)
		screenY--;
	else {
		if (scroll) {
			screenScroll(0, 1, 23);
			screenClearLine(0);
		}
	}

	screenCursorUpdate();
}

void screenCursorDown(int scroll)
{
	if (screenY < 23)
		screenY++;
	else {
		if (scroll) {
			screenScroll(1, 0, 23);
			screenClearLine(23);
		}
	}

	screenCursorUpdate();
}

/*
 * screenPutChar()
 *
 * wy¶wietla znak na ekranie o atrybucie `screenAttr', zajmuj±c siê
 * przesuwaniem ekranu, kursora i ca³± reszt±. obs³uguje sekwencje
 * steruj±ce ASCII. _nie_obs³uguje_ sekwencji steruj±cych VT100.
 */
void screenPutChar(char ch)
{
	switch (ch) {
		case 7:		/* BEL, bell */
			break;
			
		case 8:		/* BS, backspace */
			screenCursorLeft();
			return;

		case 9:		/* HT, horizontal tabulation */
			break;
			
		case 10:	/* LF, line feed */
			screenCursorDown(1);
			return;
			
		case 13:	/* CR, carriage return */
			screenX = 0;
			screenCursorUpdate();	
			return;
	}

	if (ch < 32)
		return;

	screenPutCharInternal(screenX, screenY, ch, screenAttr);

	screenCursorRight(1);
}

/*
 * screenPutHex()
 *
 * wy¶wietla liczbê heksadecymaln± `x' na ekranie.
 */
void screenPutHex(byte x)
{
	byte y = (x >> 4) & 15;
	
	if (y < 10)
		screenPutChar(48 + y);
	else
		screenPutChar('A' - 10 + y);

	y = x & 15;

	if (y < 10)
		screenPutChar(48 + y);
	else
		screenPutChar('A' - 10 + y);
}

/*
 * screenPutString()
 *
 * wy¶wietla na ekranie ci±g znaków `s'. wszystkie informacje dla
 * funkcji screenPutChar() s± wa¿ne dla tej funkcji.
 */
void screenPutString(char *s)
{
	for (;;) {
		char ch = codeReadByte(s);
		s++;
		if (!ch)
			break;
		screenPutChar(ch);
	}
}

/*
 * screenInit()
 *
 * inicjuje ekran, ustawia odpowiedni tryb, kursor, czy¶ci pamiêæ i ustawia
 * domy¶lne atrybuty.
 */
void screenInit()
{
#asm
	mov ax,#0x0003
	int #0x10
#endasm

	screenAttr = 7;
	screenX = 0;
	screenY = 0;

	screenClear();
}

#endif /* __SCREEN_H */
