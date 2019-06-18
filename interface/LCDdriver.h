#define VERTICAL 0
#define HORIZONTAL 1
//#define LCD_ALIGNMENT VERTICAL
#define LCD_ALIGNMENT HORIZONTAL

#if LCD_ALIGNMENT == VERTICAL
	#define X_RES 240 // �������𑜓x
	#define Y_RES 320 // �c�����𑜓x
#else
	#define X_RES 320 // �������𑜓x
	#define Y_RES 240 // �c�����𑜓x
#endif

#define LCD_DAT_MASK 0x01bf
#define LCD_DATCLR LATBCLR
#define LCD_DATSET LATBSET
#define LCD_RS_LO LATBCLR=0x200
#define LCD_RS_HI LATBSET=0x200
#define LCD_CS_LO LATBCLR=0x800
#define LCD_CS_HI LATBSET=0x800
#define LCD_WR_LO LATBCLR=0x400
#define LCD_WR_HI LATBSET=0x400
#define LCD_RD_LO LATBCLR=0x2000
#define LCD_RD_HI LATBSET=0x2000
#define LCD_RESET_LO LATBCLR=0x4000
#define LCD_RESET_HI LATBSET=0x4000

void LCD_WriteIndex(unsigned char index);
void LCD_WriteData(unsigned short data);
unsigned short LCD_ReadData(void);
void LCD_Init(void);
void LCD_SetCursor(unsigned short x, unsigned short y);
void LCD_Clear(unsigned short color);
void drawPixel(unsigned short x, unsigned short y, unsigned short color);
void LCD_set_Vertical(void);
void LCD_set_Horizontal(void);
