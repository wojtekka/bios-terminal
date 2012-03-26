#ifndef __KEYBOARD_C
#define __KEYBOARD_C

/*
 * keyboardGetChar()
 *
 * wczytuje z klawiatury znak. wszystkie znaki rozszerzone maj± warto¶ci
 * wiêksze ni¿ 255.
 */
word keyboardGetChar()
{
#asm
	mov ah,#0x10
	int #0x16
#endasm
}

/*
 * keyboardPressed()
 *
 * zwraca co¶, je¶li wci¶niêto klawisz.
 */
byte keyboardPressed()
{
#asm
	mov ah,#0x11
	int #0x16
	mov al,#0
	jz _keyboardPressed_1
	mov al,#1
_keyboardPressed_1:
#endasm
}

#endif /* __KEYBOARD_C */
