/*
   This file is provided under the LGPL license ver 2.1.
   Written by Katsumi.
   http://hp.vector.co.jp/authors/VA016157/
   kmorimatsu@users.sourceforge.jp
*/

#include <xc.h>
#include "main.h"
#include "compiler.h"
#include "api.h"

/* 
	dymmy imprementations:
	ps2keystatus
	ps2init
*/
volatile unsigned char ps2keystatus[256];
void g_set_palette(unsigned char n,unsigned char b,unsigned char r,unsigned char g){
	set_palette(n,b,r,g);
}
void ps2init(void){}


void pre_run(){
	// Currently, do nothing.
}

void post_run(){
	// Cool down
}


int readbuttons(){
	int keys;
	LCD_CS_HI;
	asm("nop");
	KEY_EN;
	asm("nop");
	TRISBSET=KEYMASK;
	asm("nop");
	keys=KEYPORT;
	KEY_DS;
	asm("nop");
	TRISBCLR=KEYMASK;
	asm("nop");
	LCD_CS_LO;
	return keys;	
}

void scroll30(int x,int y){
	int i,j;
	int vector=y*WIDTH_X1+x;
	if (vector<0) {
		// Copy data from upper address to lower address
		for(i=0-vector;i<WIDTH_X1*WIDTH_Y;i++){
			TVRAM[i+vector]=TVRAM[i];
			TVRAM[WIDTH_X1*WIDTH_Y+i+vector]=TVRAM[WIDTH_X1*WIDTH_Y+i];
		}
	} else if (0<vector) {
		// Copy data from lower address to upper address
		for(i=WIDTH_X1*WIDTH_Y-vector-1;0<=i;i--){
			TVRAM[i+vector]=TVRAM[i];
			TVRAM[WIDTH_X1*WIDTH_Y+i+vector]=TVRAM[WIDTH_X1*WIDTH_Y+i];
		}
	} else {
		return;
	}
	if (x<0) {
		// Fill blanc at right
		for(i=x;i<0;i++){
			for(j=WIDTH_X1+i;j<WIDTH_X1*WIDTH_Y;j+=WIDTH_X1){
				TVRAM[j]=0x00;
				TVRAM[WIDTH_X1*WIDTH_Y+j]=cursorcolor;
			}
		}
	} else if (0<x) {
		// Fill blanc at left
		for(i=0;i<x;i++){
			for(j=i;j<WIDTH_X1*WIDTH_Y;j+=WIDTH_X1){
				TVRAM[j]=0x00;
				TVRAM[WIDTH_X1*WIDTH_Y+j]=cursorcolor;
			}
		}
	}
	if (y<0) {
		// Fill blanc at bottom
		for(i=WIDTH_X1*(WIDTH_Y+y);i<WIDTH_X1*WIDTH_Y;i++){
				TVRAM[i]=0x00;
				TVRAM[WIDTH_X1*WIDTH_Y+i]=cursorcolor;
		}
	} else if (0<y) {
		// Fill blanc at top
		for(i=0;i<WIDTH_X1*y;i++){
				TVRAM[i]=0x00;
				TVRAM[WIDTH_X1*WIDTH_Y+i]=cursorcolor;
		}
	}
}

void scroll40(int x,int y){
	int i,j;
	int vector=y*WIDTH_X2+x;
	if (vector<0) {
		// Copy data from upper address to lower address
		for(i=0-vector;i<WIDTH_X2*WIDTH_Y;i++){
			TVRAM[i+vector]=TVRAM[i];
			TVRAM[WIDTH_X2*WIDTH_Y+i+vector]=TVRAM[WIDTH_X2*WIDTH_Y+i];
		}
	} else if (0<vector) {
		// Copy data from lower address to upper address
		for(i=WIDTH_X2*WIDTH_Y-vector-1;0<=i;i--){
			TVRAM[i+vector]=TVRAM[i];
			TVRAM[WIDTH_X2*WIDTH_Y+i+vector]=TVRAM[WIDTH_X2*WIDTH_Y+i];
		}
	} else {
		return;
	}
	if (x<0) {
		// Fill blanc at right
		for(i=x;i<0;i++){
			for(j=WIDTH_X2+i;j<WIDTH_X2*WIDTH_Y;j+=WIDTH_X2){
				TVRAM[j]=0x00;
				TVRAM[WIDTH_X2*WIDTH_Y+j]=cursorcolor;
			}
		}
	} else if (0<x) {
		// Fill blanc at left
		for(i=0;i<x;i++){
			for(j=i;j<WIDTH_X2*WIDTH_Y;j+=WIDTH_X2){
				TVRAM[j]=0x00;
				TVRAM[WIDTH_X2*WIDTH_Y+j]=cursorcolor;
			}
		}
	}
	if (y<0) {
		// Fill blanc at bottom
		for(i=WIDTH_X2*(WIDTH_Y+y);i<WIDTH_X2*WIDTH_Y;i++){
				TVRAM[i]=0x00;
				TVRAM[WIDTH_X2*WIDTH_Y+i]=cursorcolor;
		}
	} else if (0<y) {
		// Fill blanc at top
		for(i=0;i<WIDTH_X2*y;i++){
				TVRAM[i]=0x00;
				TVRAM[WIDTH_X2*WIDTH_Y+i]=cursorcolor;
		}
	}
}

void scroll(int x, int y){
	if (twidth==40) scroll40(x,y);
	else scroll30(x,y);
}

void allocate_graphic_area(){
	if (!g_graphic_area) {
		// Use this pointer like unsigned short GVRAM[G_H_WORD*G_Y_RES] __attribute__ ((aligned (4)));
//		g_graphic_area=(unsigned short*)alloc_memory(G_H_WORD*G_Y_RES/2,ALLOC_GRAPHIC_BLOCK);
		g_graphic_area=(void *)1;
		// Start graphic and clear screen
//		init_graphic(g_graphic_area);
		init_graphic();
		// Move current point to (0,0)
//		g_prev_x=g_prev_y=0;
	}
}

void usegraphic(int mode){
	// Modes; 0: stop GRAPHIC, 1: use GRAPHIC, 2: reset GRAPHIC and use it
	switch(mode){
		case 0:
			if (g_use_graphic){
				// Stop GRAPHIC if used
				set_graphmode(0);
				g_use_graphic=0;
				// Set timer4 for tempo
//				PR4=59473;       // 3632*262/16-1
			} else {
				// Prepare GRAPHIC area if not used and not allcated.
				allocate_graphic_area();
			}
			break;
		case 2:
			// Reset GRAPHIC and use it
			g_graphic_area=0;
			// Continue to case 1:
		case 1:
		case 3:
		default:
			// Use GRAPHIC
			allocate_graphic_area();
			// Start showing GRAPHIC with mode 1, but not with mode 3
			if (mode !=3 && !g_use_graphic){
				set_graphmode(1);
				g_use_graphic=1;
				// Set timer4 for tempo
//				PR4=55756;       // ~=3405*262/16-1(55755.875)
			}
			break;
	}
}

void videowidth(int width){
	// Do nothing
}

int lib_system(int a0, int a1 ,int v0, int a3, int g_gcolor, int g_prev_x, int g_prev_y){
	switch(a0){
		// Version info etc
		case 0: return (int)SYSVER1;
		case 1: return (int)SYSVER2;
		case 2: return (int)BASVER;
		case 3: return (int)FILENAME_FLASH_ADDRESS;
		case 4: return (int)CPU_CLOCK_HZ;
		// Display info
		case 20: return twidth;
		case 21: return WIDTH_Y;
		case 22: return G_X_RES;
		case 23: return G_Y_RES;
		case 24: return cursorcolor;
		case 25: return g_gcolor;
		case 26: return ((int)(cursor-TVRAM))%twidth;
		case 27: return ((int)(cursor-TVRAM))/twidth;
		case 28: return g_prev_x;
		case 29: return g_prev_y;
		// Keyboard info
		case 40: return 0;//(int)inPS2MODE();
		case 41: return 0;//(int)vkey;
		case 42: return 0;//(int)lockkey;
		case 43: return 0;//(int)keytype;
		// Pointers to gloval variables
		case 100: return (int)&g_var_mem[0];
		case 101: return (int)&g_rnd_seed;
		case 102: return (int)&TVRAM[0];
		case 103: return (int)&FontData[0];
		case 104: return (int)g_var_mem[ALLOC_PCG_BLOCK];
		case 105: return (int)g_var_mem[ALLOC_GRAPHIC_BLOCK];
		// Change system settings
		case 200:
			// ON/OFF monitor
			if (v0) {
//				start_composite();
			} else {
//				stop_composite();
			}
			break;
		default:
			break;
	}
	return 0;
}
