/*
   This file is provided under the LGPL license ver 2.1.
   Written by Katsumi.
   http://hp.vector.co.jp/authors/VA016157/
   kmorimatsu@users.sourceforge.jp
*/

#define CPU_CLOCK_HZ 48000000
#define PERSISTENT_RAM_SIZE (1024*53)

int readbuttons();
void scroll(int x, int y);
void usegraphic(int mode);
void videowidth(int width);
int lib_system(int a0, int a1 ,int v0, int a3, int g_gcolor, int g_prev_x, int g_prev_y);

// 30 or 40 characters per line for Zoea
#define printcomma() printstr("          "+rem10_32((unsigned int)(cursor-TVRAM)))

// Check break key or buttons when executing BASIC code.
// In PS/2 mode, detect ctrl-break.
// In button mode, detect pushing four buttons are pushed simultaneously.
#define check_break() \
	if (g_disable_break==0) {\
		if (inPS2MODE()) {\
			if (ps2keystatus[0x03]) err_break();\
		} else {\
			if ((PORTB&0x4c80)==0) err_break();\
		}\
	}

// Protozoa specific lists of statements and functions (none defined for Zoea)
#define ADDITIONAL_STATEMENTS
#define ADDITIONAL_INT_FUNCTIONS
#define ADDITIONAL_STR_FUNCTIONS
#define ADDITIONAL_RESERVED_VAR_NAMES
#define ADDITIONAL_INTERRUPT_FUNCTIONS
