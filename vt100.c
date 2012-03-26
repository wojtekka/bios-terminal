#ifndef __VT100_C
#define __VT100_C

char vtDummy[10] = { 0 };

char vtScanCodes[2][58] = {
	{ 0,27,'1','2','3','4','5','6','7','8','9','0','-','=',8,9,
	  'q','w','e','r','t','y','u','i','o','p','[',']',13,0,'a','s',
	  'd','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v',
	  'b','n','m',',','.','/',0,0,0,' ' },

	{ 0,27,'!','@','#','$','%','^','&','*','(',')','_','+',8,9,
	  'Q','W','E','R','T','Y','U','I','O','P','{','}',13,0,'A','S',
	  'D','F','G','H','J','K','L',':','"','~',0,'|','Z','X','C','V',
	  'B','N','M','<','>','?',0,0,0,' ' } };

struct {
	char mode;
	int code;
	char *sequence;
} vtFunctionKeys[] = {
	{ 0, 0x3b00, "\033OP" },		/* F1 */
	{ 0, 0x3c00, "\033OQ" },
	{ 0, 0x3d00, "\033OR" },
	{ 0, 0x3e00, "\033OS" },
	{ 0, 0x3f00, "\033[15~" },	
	{ 0, 0x4000, "\033[17~" },
	{ 0, 0x4100, "\033[18~" },
	{ 0, 0x4200, "\033[19~" },
	{ 0, 0x4300, "\033[20~" },
	{ 0, 0x4400, "\033[21~" },		/* F10 */

	{ 0, 0x52e0, "\033[2~" },		/* Ins */
	{ 0, 0x53e0, "\033[3~" },		/* Del */
	{ 0, 0x47e0, "\033[1~" },		/* Home */
	{ 0, 0x4fe0, "\033[4~" },		/* End */
	{ 0, 0x49e0, "\033[5~" },		/* PgUp */
	{ 0, 0x51e0, "\033[6~" },		/* PgDn */

	{ 1, 0x48e0, "\033[A" },		/* ^ */
	{ 1, 0x50e0, "\033[B" },		/* v */
	{ 1, 0x4be0, "\033[D" },		/* < */
	{ 1, 0x4de0, "\033[C" },		/* > */

	{ 2, 0x48e0, "\033OA" },		/* ^ */
	{ 2, 0x50e0, "\033OB" },		/* v */
	{ 2, 0x4be0, "\033OD" },		/* < */
	{ 2, 0x4de0, "\033OC" },		/* > */

	{ 0, 0, 0 }
};

byte vtLineMap[32] = { 4, 176, '?', '?', '?', '?', 248, 241, '?', '?', 217, 191, 218, 192, 197, '?', '?', 196, '?', '?', 195, 180, 192, 194, 179, 243, 242, '?', '?', '?', '?', '?' };

byte vtG = 0;			// aktualna mapa znaków
byte vtG0 = 0, vtG1 = 0;	// wybrana mapa G0 i G1
byte vtMode = 0;		// tryb kursorów
byte vtLED = 0;			// w³±czone diody LED

/*
 * vtLEDUpdate()
 *
 * uaktualnia wska¼nik diod LED VT100.
 */
void vtLEDUpdate()
{
	screenPutCharInternal(67, 24, (vtLED & 1) ? '[' : ' ', 7*16);
	screenPutCharInternal(68, 24, (vtLED & 1) ? '1' : ' ', 7*16);
	screenPutCharInternal(69, 24, (vtLED & 1) ? ']' : ' ', 7*16);
	screenPutCharInternal(70, 24, (vtLED & 2) ? '[' : ' ', 7*16);
	screenPutCharInternal(71, 24, (vtLED & 2) ? '2' : ' ', 7*16);
	screenPutCharInternal(72, 24, (vtLED & 2) ? ']' : ' ', 7*16);
	screenPutCharInternal(73, 24, (vtLED & 4) ? '[' : ' ', 7*16);
	screenPutCharInternal(74, 24, (vtLED & 4) ? '3' : ' ', 7*16);
	screenPutCharInternal(75, 24, (vtLED & 4) ? ']' : ' ', 7*16);
	screenPutCharInternal(76, 24, (vtLED & 8) ? '[' : ' ', 7*16);
	screenPutCharInternal(77, 24, (vtLED & 8) ? '4' : ' ', 7*16);
	screenPutCharInternal(78, 24, (vtLED & 8) ? ']' : ' ', 7*16);
}

/*
 * vtGetChar()
 *
 * zwraca sekwencjê VT100 
 */
char *vtGetChar()
{
	int ch = keyboardGetChar(), i, x, y, a;

#ifdef COM
	if (ch == 0x8500) {	/* F11 */
		if (screenX != 0)
			screenPutString("\r\n");
		#asm
			xor ax,ax
			int #0x21
		#endasm
	}
#endif

	if (ch == 0x8600) {	/* F12 */
		serialConfig();
		return;
	}

	for (i = 0; i < sizeof(vtDummy); i++)
		vtDummy[i] = 0;
	
#ifdef DEBUG_KEYBOARD
	x = screenX; y = screenY; a = screenAttr;
	screenCursorNoUpdate = 1;
	screenX = 75; screenY = 24; screenAttr = 7*16+1;
	screenPutHex(ch >> 8);
	screenPutHex(ch &255);
	screenCursorNoUpdate = 0;
	screenX = x; screenY = y; screenAttr = a;
#endif

	if (ch & 255) {
		if ((ch & 255) != 0xe0) {
			vtDummy[0] = ch;
			return vtDummy;
		}
	}

	if ((ch & 255) == 0) {
		if ((ch >> 8) < 58) {
			vtDummy[0] = 27;
			vtDummy[1] = vtScanCodes[0][ch >> 8];
			return vtDummy;
		}
	}

	for (i = 0; vtFunctionKeys[i].code; i++) {
		if (ch == vtFunctionKeys[i].code)
			if (!vtFunctionKeys[i].mode || (vtFunctionKeys[i].mode == vtMode + 1))
				return vtFunctionKeys[i].sequence;
	}

	return vtDummy;
}

int vtEsc = 0;			/* czy jeste¶my w sekwencji steruj±cych? */
int vtArg1 = 0, vtArg2 = 0;	/* argumenty */
int vtArgs = 0;			/* ilo¶æ argumentów */
char vtChar = 0;		/* pierwszy znak po ESC */
char vtChar2 = 0;		/* drugi znak po ESC */

/*
 * vtPutChar()
 *
 * wy¶wietla bajt na konsoli analizuj±c sekwencje VT100.
 */
void vtPutChar(byte ch)
{
	int i;

	if (ch == 15) {		/* Ctrl-O, select G0 */
		vtG = 0;
		return;
	}

	if (ch == 14) {		/* Ctrl-N, select G1 */
		vtG = 1;
		return;
	}
	
	if (ch == 27) {
		vtEsc = 1;
		vtArgs = 0;
		vtArg1 = 0;
		vtArg2 = 0;
		vtChar = 0;
		vtChar2 = 0;

		return;
	}

	if (vtEsc) {
		if (ch == 24 || ch == 26) {	/* CAN, SUB */
			vtEsc = 0;
			return;
		}

		if (vtChar == '(') {
			vtEsc = 0;
			switch (ch) {
				case 'A': vtG0 = 0; break;
				case 'B': vtG0 = 1; break;
				case '0': vtG0 = 2; break;
			}
			return;
		}

		if (vtChar == ')') {
			vtEsc = 0;
			switch (ch) {
				case 'A': vtG1 = 0; break;
				case 'B': vtG1 = 1; break;
				case '0': vtG1 = 2; break;
			}
			return;
		}

		if (vtChar && !vtChar2)
			vtChar2 = ch;

		if (!vtChar)
			vtChar = ch;

		if (ch >= '0') if (ch <= '9') if (vtArgs < 2) {
			if (!vtArgs)
				vtArg1 = vtArg1 * 10 + (ch - '0');
			else
				vtArg2 = vtArg2 * 10 + (ch - '0');
		}

		if (ch == ';')
			vtArgs++;

		if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
			vtArgs++;
			vtEsc = 0;
			
			switch (ch) {
				case 'r':
					screenScrollTop = vtArg1 - 1;
					screenScrollBottom = vtArg2 - 1;

					if (screenScrollTop > 23)
						screenScrollTop = 23;

					if (screenScrollBottom > 23)
						screenScrollBottom = 23;
					
					if (screenScrollTop == 0 || screenScrollBottom == 0) {
						screenScrollTop = 0;
						screenScrollBottom = 23;
					}

					screenX = 0;
					screenY = 0;
					screenCursorUpdate();

					break;

				case 'A':
					if (!vtArg1)
						vtArg1++;
					for (i = 0; i < vtArg1; i++)
						screenCursorUp(0);
					break;
					
				case 'B':
					if (!vtArg1)
						vtArg1++;
					for (i = 0; i < vtArg1; i++)
						screenCursorDown(0);
					break;
					
				case 'C':
					if (!vtArg1)
						vtArg1++;
					for (i = 0; i < vtArg1; i++)
						screenCursorRight(0);
					break;
					
				case 'D':
					if (vtChar == 'D') {
						screenCursorDown(1);
						break;
					}

					if (!vtArg1)
						vtArg1++;
					for (i = 0; i < vtArg1; i++)
						screenCursorLeft(0);
					break;

				case 'M':
					if (vtChar == 'M')
						screenCursorUp(1);
					else {
						screenScroll(screenScrollTop + vtArg1, screenScrollTop, (screenScrollBottom - screenScrollTop - vtArg1 + 1));
						screenClearLine(screenScrollBottom);
					}

					break;

				case 'L':
					for (i = 0; i < vtArg1; i++)
						screenCursorUp(1);
					break;

				case 'E':
					screenPutChar(13);
					screenPutChar(10);
					break;

				case 'P':
					for (i = screenX + vtArg1; i < 80; i++)
						memoryWriteWord(0xb800, screenY * 160 + (i - vtArg1) * 2, memoryReadWord(0xb800, screenY * 160 + i * 2));
					for (i = 80 - vtArg1; i < 80; i++)
						screenPutCharInternal(i, screenY, ' ', screenAttr);

					break;
					
				case 'K':
					switch (vtArg1) {
						case 0:
							for (i = screenX; i < 80; i++)
								screenPutCharInternal(i, screenY, ' ', screenAttr);
							break;
						case 1:
							for (i = 0; i <= screenX; i++)
								screenPutCharInternal(i, screenY, ' ', screenAttr);
							break;
						case 2:
							for (i = 0; i < 80; i++)
								screenPutCharInternal(i, screenY, ' ', screenAttr);
							break;
					}

					break;
					
				case 'J':
					switch (vtArg1) {
						case 0:
							for (i = screenY * 80 + screenX; i < 80 * 24; i++)
								memoryWriteWord(0xb800, i * 2, ' ' | (screenAttr << 8));
							break;
						case 1:
							for (i = 0; i <= screenY * 80 + screenX; i++)
								memoryWriteWord(0xb800, i * 2, ' ' | (screenAttr << 8));
							break;
						case 2:
							for (i = 0; i < 80 * 25; i++)
								memoryWriteWord(0xb800, i * 2, ' ' | (screenAttr << 8));
							break;
					}

					break;
					
				case 'm':
					switch (vtArg1) {
						case 0:
							screenAttr = 7;
							break;
						case 1:
							screenAttr = (screenAttr & 128) | 15;
							break;
						case 4:
							screenAttr = (screenAttr & 128) | 1;
							break;
						case 5:
							screenAttr = screenAttr | 128;
							break;
						case 7:
							screenAttr = (screenAttr & 128) | 7*16;
							break;
						case 27:
							screenAttr = (screenAttr & 128) | 7;
					}
					break;

				case 'H':
				case 'f':
					if (vtArg1 > 0)
						vtArg1--;
					if (vtArg2 > 0)
						vtArg2--;
					
					screenX = vtArg2;
					screenY = vtArg1;
					screenCursorUpdate();

					break;

				case 'c':
					if (vtChar == '[' && vtChar2 != '?')
						serialPutString("\033[?1;0c");

					if (vtChar == 'c') {
						screenX = 0;
						screenY = 0;
						screenAttr = 7;
						screenScrollTop = 0;
						screenScrollBottom = 23;
						screenCursorUpdate();
						screenClear();
						vtMode = 0;
						vtG = 0;
						vtG0 = 0;
						vtG1 = 0;
						vtLED = 0;
						vtLEDUpdate();
					}
			
					break;

				case 'Z':
					serialPutString("\033[?1;0c");
					break;

				case 'n':
					if (vtArg1 == 5)
						serialPutString("\033[0n");

					if (vtArg1 == 6) {
						serialPutString("\033[");
						serialPutDec(screenY + 1);
						serialPutChar(';');
						serialPutDec(screenX + 1);
						serialPutChar('R');
					}

					break;

				case 'h':
					if (vtChar2 == '?')
						if (vtArg1 == 1)
							vtMode = 1;
					break;

				case 'l':
					if (vtChar2 == '?')
						if (vtArg1 == 1)
							vtMode = 0;
					break;

				case 'q':
					if (vtArg1 == 0)
						vtLED = 0;
					if (vtArg1 > 0 && vtArg1 < 5)
						vtLED |= (1 << (vtArg1 - 1));

					vtLEDUpdate();

					break;
			}
		}

		return;
	}

	if ((vtG == 0 && vtG0 == 2) || (vtG == 1 && vtG1 == 2)) {
		if (ch >= 96 && ch < 128)
			ch = vtLineMap[ch - 96];
	}

	if (ch >= 32)
		if (ch < 128)
			screenPutChar(ch);

	if (ch >= 160)
		if (ch < 256)
			screenPutChar(ch);

	if (ch == 13 || ch == 10 || ch == 8)
		screenPutChar(ch);
}

#endif /* __VT100_C */
