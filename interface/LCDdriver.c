/*
 * 液晶モジュールドライバ for PIC32MX1xx/2xx
 * システムクロック48MHz、周辺クロック48MHz
 *
 * 液晶モジュールの初期化はブートローダ側で行うため不要
 *
 * 8bit mode接続
 * D8  RB0
 * D9  RB1
 * D10 RB2
 * D11 RB3
 * D12 RB4
 * D13 RB5
 * D14 RB7
 * D15 RB8
 * RS  RB9
 * WR  RB10
 * CS  RB11
 * RD  RB13
 * RESET RB14
 */

#include <plib.h>
#include "LCDdriver.h"

void inline LCD_set_dat(unsigned char c){
	LCD_DATCLR=LCD_DAT_MASK;
	LCD_DATSET=((c<<1)&0x180)+(c&0x3f);
}
unsigned char LCD_get_dat(void){
	unsigned short d;
	d=PORTB & LCD_DAT_MASK;
	return ((d&0x180)>>1)+(d&0x3f);
}
void LCD_WriteIndex(unsigned char index){
// Write Index
	LCD_RS_LO;
	LCD_set_dat(0);
	LCD_WR_LO;
	asm("nop");
	LCD_WR_HI;
	LCD_set_dat(index);
	LCD_WR_LO;
	asm("nop");
	LCD_WR_HI;
}

void LCD_WriteData(unsigned short data)
{
// Write Data
	LCD_RS_HI;
	LCD_set_dat(data>>8);
	LCD_WR_LO;
	asm("nop");
	LCD_WR_HI;
	LCD_set_dat((unsigned char)data);
	LCD_WR_LO;
	asm("nop");
	LCD_WR_HI;
}
unsigned short LCD_ReadData(void)
{
// Read Data
	unsigned short d;
	TRISBSET=LCD_DAT_MASK;
	LCD_RS_HI;
	LCD_RD_LO;
	asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
	asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
	asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
	d=LCD_get_dat();
	LCD_RD_HI;
	asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
	asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
	LCD_RD_LO;
	asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
	asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
	asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
	d=(d<<8)+LCD_get_dat();
	LCD_RD_HI;
	asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");
	TRISBCLR=LCD_DAT_MASK;
	return d;
}
void LCD_WriteReg(unsigned char index, unsigned short data)
{
// Write Index
	LCD_WriteIndex(index);

// Write Data
	LCD_WriteData(data);
}
unsigned short LCD_ReadReg(unsigned char index)
{
// Write Index
	LCD_WriteIndex(index);

// Read Data
	return LCD_ReadData();
}

void LCD_SetCursor(unsigned short x, unsigned short y)
{
#if LCD_ALIGNMENT == VERTICAL
	LCD_WriteReg(0x20, x);
	LCD_WriteReg(0x21, y);
#elif LCD_ALIGNMENT == HORIZONTAL
	LCD_WriteReg(0x20, y);
	LCD_WriteReg(0x21, x);
#endif
}

void LCD_continuous_output(unsigned short x,unsigned short y,unsigned short color,int n)
{
	//High speed continuous output
	int i;
	unsigned char d1,d2;
	LCD_SetCursor(x,y);
	LCD_WriteIndex(0x22);
	LCD_RS_HI;
	d1=color>>8;
	d2=(unsigned char)color;
	for (i=0; i < n ; i++){
		LCD_set_dat(d1);
		LCD_WR_LO;
		asm("nop");
		LCD_WR_HI;
		LCD_set_dat(d2);
		LCD_WR_LO;
		asm("nop");
		LCD_WR_HI;
	}
}
void LCD_Clear(unsigned short color)
{
	LCD_continuous_output(0,0,color,X_RES*Y_RES);
}

void drawPixel(unsigned short x, unsigned short y, unsigned short color)
{
	LCD_SetCursor(x,y);
	LCD_WriteIndex(0x22);
	LCD_WriteData(color);
}

unsigned short getColor(unsigned short x, unsigned short y)
{
	unsigned short d;
	LCD_SetCursor(x,y);
	LCD_WriteIndex(0x22);
	LCD_ReadData(); //dummy read
	d=LCD_ReadData();
	return (d>>11)+(d&0x7e0)+((d&0x1f)<<11); //swap R and B
}

void LCD_set_Vertical(void){
	if(LCD_ReadReg(0)==0x0129){
		LCD_WriteReg(0x01, 0x0127);
		LCD_WriteReg(0x03, 0x1030);
	}
	else{
		LCD_WriteReg(0x01, 0x0100);
		LCD_WriteReg(0x03, 0x1030);
	}
}

void LCD_set_Horizontal(void){
	if(LCD_ReadReg(0)==0x0129){
		LCD_WriteReg(0x01, 0x0027);
		LCD_WriteReg(0x03, 0x1028);
	}
	else{
		LCD_WriteReg(0x01, 0x0000);
		LCD_WriteReg(0x03, 0x1028);
	}
}
