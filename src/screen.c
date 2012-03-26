#ifndef __SCREEN_C
#define __SCREEN_C

byte screenX = 0;	/* aktualna kolumna kursora (0..79) */

byte screenY = 0;	/* aktualny wiersz kursora (0..24) */

byte screenAttr = 7;	/* aktualne atrybyty tekstu. dolne 4 bity to kolor
			 * tekstu, górne 3 to kolor t³a, najstarszy to
			 * atrybut mrugania. */

byte screenCursorNoUpdate = 0;		/* je¶li 1, nie przesuwamy kursora
					 * fizycznego po ekranie.*/

byte screenScrollTop = 0;		/* górny wiersz regionu przewijania */
byte screenScrollBottom = 23;		/* dolny wiersz regionu przewijania */

byte screenBuffer[4000] = { 0 };	/* kopia ekranu */
byte screenBufferX = 0;
byte screenBufferY = 0;
byte screenBufferAttr = 0;

/*
 * screenCursorUpdate()
 *
 * przesuwa kursor ekranowy do wspó³rzêdnych (screenX,screenY). funkcja
 * powinna byæ wywo³ywana po ka¿dej operacji zmieniaj±cej zawarto¶æ ekranu.
 */
void screenCursorUpdate()
{
#asm
	mov al,[_screenCursorNoUpdate]
	test al,al
	jz _sCU1
	ret

_sCU1:
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
 * screenSave()
 *
 * zachowuje ekran do bufora.
 */
void screenSave()
{
#asm
	push cx
	push ax
	push es
	push ds
	push si
	push di
	
	mov cx,#80*25
	mov ax,ds
	mov es,ax
	mov ax,#0xb800
	mov ds,ax
	mov di,#_screenBuffer
	xor si,si
	rep
	movsw

	pop di
	pop si
	pop ds
	pop es
	pop ax
	pop cx
#endasm

	screenBufferX = screenX;
	screenBufferY = screenY;
	screenBufferAttr = screenAttr;
}

/*
 * screenRestore()
 *
 * przywraca ekran z bufora.
 */
void screenRestore()
{
#asm
	push cx
	push ax
	push es
	push ds
	push si
	push di
	
	mov cx,#80*25
	mov ax,#0xb800
	mov es,ax
	mov si,#_screenBuffer
	xor di,di
	rep
	movsw

	pop di
	pop si
	pop ds
	pop es
	pop ax
	pop cx
#endasm

//	for (i = 0; i < 4000; i++)
//		memoryWriteByte(0xb800, i, screenBuffer[i]);

	screenX = screenBufferX;
	screenY = screenBufferY;
	screenAttr = screenBufferAttr;

	screenCursorUpdate();
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
	word ptr, data;
	byte i;

	ptr = line * 160;
	data = ' ' | (screenAttr << 8);

	for (i = 0; i < 80; i++, ptr += 2)
		memoryWriteWord(0xb800, ptr, data);
}

/*
 * screenClear()
 *
 * czy¶ci ca³y ekran atrybutem `screenAttr', ustawia kursor na pocz±tek
 * ekranu.
 */
void screenClear()
{
	word ptr = 0, i, j = ' ' | (screenAttr << 8);
	
	for (i = 0; i < 1920; i++, ptr += 2)
		memoryWriteWord(0xb800, ptr, j);
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
	count = lines * 80;

	if (from < to) {
		ptrFrom += count * 2 - 2;
		ptrTo += count * 2 - 2;
		
		asm("std");
	} else {
		asm("cld");
	}

#asm
	push ds
	push es

	mov ax,#0xb800
	mov ds,ax
	mov es,ax

	mov si,-6[bp]
	mov di,-8[bp]
	mov cx,-10[bp]

	rep
	movsw

	pop es
	pop ds
#endasm

#if 0
	if (from > to) {
		while (count--) {
			tmp = memoryReadWord(0xb800, ptrFrom++);
			memoryWriteWord(0xb800, ptrTo++, tmp);
		}
	} else {
		ptrFrom += count;
		ptrTo += count;

		while (count--) {
			tmp = memoryReadWord(0xb800, --ptrFrom);
			memoryWriteWord(0xb800, --ptrTo, tmp);
		}
	}
#endif
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
			if (screenY < screenScrollBottom)
				screenY++;
		} else {
			screenY++;

			if (screenY >= screenScrollBottom + 1) {
				screenScroll(screenScrollTop + 1, screenScrollTop, (screenScrollBottom - screenScrollTop));
				screenClearLine(screenScrollBottom);
				screenY = screenScrollBottom;
			}
		}
	}

	screenCursorUpdate();
}

void screenCursorUp(int scroll)
{
	if (screenY > screenScrollTop)
		screenY--;
	else {
		if (scroll) {
			screenScroll(screenScrollTop, screenScrollTop + 1, (screenScrollBottom - screenScrollTop));
			screenClearLine(screenScrollTop);
		}
	}

	screenCursorUpdate();
}

void screenCursorDown(int scroll)
{
	if (screenY < screenScrollBottom)
		screenY++;
	else {
		if (scroll) {
			screenScroll(screenScrollTop + 1, screenScrollTop, (screenScrollBottom - screenScrollTop));
			screenClearLine(screenScrollBottom);
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

word screenPutCharOffset = 0;
word screenPutCharData = 0;

void screenPutChar(char ch)
{
	if (ch == 7)		/* BEL, bell */
		return;

	if (ch == 8) {		/* BS, backspace */
		if (screenX > 0)
			screenCursorLeft();
		return;
	}

	if (ch == 9)		/* HT, horizontal tabulation */
		return;
				
	if (ch == 10 || ch == 11 || ch == 12) {	/* LF, VT, FF */
		screenCursorDown(1);
		return;
	}
				
	if (ch == 13) {		/* CR, carriage return */
		screenX = 0;
		screenCursorUpdate();
		return;
	}

	if (ch < 32)
		return;

	if (ch == 127)
		return;

	screenPutCharOffset = (screenY * 80 + screenX) << 1;
	screenPutCharData = ch | (screenAttr << 8);

#asm
	push ax
	push bx
	push es

	mov ax,#0xb800
	mov es,ax

	mov bx,[_screenPutCharOffset]
	mov ax,[_screenPutCharData]

	seg es
	mov [bx],ax

	pop es
	pop bx
	pop ax
#endasm
	
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
		char ch = *s;
		s++;
		if (!ch)
			break;
		screenPutChar(ch);
	}
}

#define screenPutStringXY(x,y,s) do { screenX = x; screenY = y; screenPutString(s); } while(0)

/*
 * screenInit()
 *
 * inicjuje ekran, ustawia odpowiedni tryb, kursor, czy¶ci pamiêæ i ustawia
 * domy¶lne atrybuty.
 */
void screenInit()
{
	int i;

#asm
	mov ax,#0x0003
	int #0x10

#if 0
	push bp
	
	mov ax,ds
	mov es,ax
	mov ax,#_screenCharset
	mov bp,ax

	mov ax,#0x1100
	mov cx,#0x0100
	mov dx,#0x0000
	mov bx,#0x1000
	
	int #0x10

	pop bp
#endif
#endasm

	screenAttr = 7;
	screenX = 0;
	screenY = 0;

	screenClear();

	for (i = 0; i < 80; i++)
		screenPutCharInternal(i, 24, ' ', 7*16);
}

#endif /* __SCREEN_C */
