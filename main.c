/*
   This file is provided under the LGPL license ver 2.1.
   Written by K.Tanaka & Katsumi
   http://www.ze.em-net.ne.jp/~kenken/index.html
   http://hp.vector.co.jp/authors/VA016157/
*/

// main.c
// MachiKania BASIC System Ver Zoea
// KM-BASIC 統合開発実行環境 for PIC32MX170F256B / PIC32MX270F256B by K.Tanaka
// LCD出力バージョン（テスト） by K.Tanaka

// 利用システム
// libsdfsio.a ： SDカードアクセス用ライブラリ

// System clock :48MHz  FRC with PLL (8/2*24/2)
// Peripheral clock :48MHz

/*
	PIC32MX ペリフェラル使用状況
	
	割り込み
		NTSC,   Timer2, vector  8, priority 5
		MUSIC,  CS0,    vector  1, priority 2
		TIMER,  Timer1, vector  4, priority 3
		INT,    CS1,    vector  2, priority 1
	
	タイマー
		Timer1 BASIC用タイマー
		Timer2 NTSC
		Timer3 MUSIC/PWM
		Timer4 MUSIC
		Timer5 未使用
	
	DMA
		DMA0 未使用
		DMA1 未使用
		DMA2 MUSIC
		DMA3 未使用
	
	Output compair
		OC1 未使用
		OC2 未使用
		OC3 未使用
		OC4 MUSIC/PWM
		OC5 未使用
		
	SPI
		SPI1 未使用
		SPI2 マルチメディアカード
	
	I2C
		I2C1 未使用
		I2C2 未使用
	
	ポート使用
		A0  MMC
		A1  MMC
		A2  Audio
		A3  LCD/button切換え
		A4  MMC
		B0  LCD
		B1  LCD
		B2  LCD/button
		B3  LCD/button
		B4  LCD/button
		B5  LCD/button
		B6  未使用
		B7  LCD/button
		B8  LCD/button
		B9  PS2/button
		B10 LCD
		B11 LCD
		B12 未使用
		B13 LCD
		B14 LCD
		B15 MMC
*/

#include <xc.h>
#include "api.h"
#include "compiler.h"
#include "editor.h"
#include "main.h"

#define mBMXSetRAMKernProgOffset(offset)	(BMXDKPBA = (offset))
#define mBMXSetRAMUserDataOffset(offset)	(BMXDUDBA = (offset))
#define mBMXSetRAMUserProgOffset(offset)	(BMXDUPBA = (offset))

// INIファイル指定キーワード（8文字以内）
const char InitKeywords[][9]={
	"106KEY","101KEY","NUMLOCK","CAPSLOCK","SCRLLOCK"
};

void freadline(char *s,FSFILE *fp){
// ファイルから1行読み込み、配列sに返す
// 最大8文字まで。9文字以上の場合無効
// #または0x20以下のコードを見つけた場合、以降は無視
// s:9バイト以上の配列
// fp:ファイルポインタ
	int n;
	char c,*p;
	n=0;
	p=s;
	*p=0;
	while(n<=8){
		if(FSfread(p,1,1,fp)==0 || *p=='\n'){
			*p=0;
			return;
		}
		if(*p=='#'){
			*p=0;
			break;
		}
		if(*p<=' '){
			if(n>0){
				*p=0;
				break;
			}
			continue;
		}
		p++;
		n++;
	}
	if(n>8) *s=0; //9文字以上の文字列の場合は無効
	//以降の文字は無視
	while(FSfread(&c,1,1,fp) && c!='\n') ;
}
int searchinittext(char *s){
// InitKeywords配列の中から文字列sを探し、位置した場合何番目かを返す
// 見つからなかった場合-1を返す
	int i;
	char *p1;
	const char *p2;
	for(i=0;i<sizeof(InitKeywords)/sizeof(InitKeywords[0]);i++){
		p1=s;
		p2=InitKeywords[i];
		while(*p1==*p2){
			if(*p1==0) return i;
			p1++;
			p2++;
		}
	}
	return -1;
}
void readinifile(void){
	FSFILE *fp;
	char inittext[9];

	fp=FSfopen(INIFILE,"r");
	if(fp==NULL) return;
	printstr("Initialization File Found\n");
//	lockkey=0; //INIファイルが存在する場合、Lock関連キーはINIファイルに従う
	while(1){
		if(FSfeof(fp)) break;
		freadline(inittext,fp);
		switch(searchinittext(inittext)){
			case 0:
//				keytype=0;//日本語キーボード
				break;
			case 1:
//				keytype=1;//英語キーボード
				break;
			case 2:
//				lockkey|=2;//Num Lock
				break;
			case 3:
//				lockkey|=4;//CAPS Lock
				break;
			case 4:
//				lockkey|=1;//Scroll Lock
				break;
		}
	}
	FSfclose(fp);
}

void printhex8(unsigned char d){
	printchar("0123456789ABCDEF"[d>>4]);
	printchar("0123456789ABCDEF"[d&0x0f]);	
}

void printhex16(unsigned short d){
	printhex8(d>>8);
	printhex8(d&0x00ff);
}

void printhex32(unsigned int d){
	printhex16(d>>16);
	printhex16(d&0x0000ffff);
}

int main(void){
	char *appname,*s;

	if(DEVCFG1 & 0x8000){
		// Set Clock switching enabled and reset
		NVMWriteWord(&DEVCFG1,DEVCFG1 & 0xffff7fff);
		SoftReset();
	}

	/* ポートの初期設定 */
	TRISA = 0x0000; // PORTA全て出力
	ANSELA = 0x0000; // 全てデジタル
	TRISB = 0x0000; // PORTB全て出力（ボタン読み込み時のみ入力に切り替える）
	ANSELB = 0x0000; // 全てデジタル
	KEY_DS;
	LCD_CS_LO;

	// 周辺機能ピン割り当て
	SDI2R=2; //RPA4:SDI2
	RPA1R=4; //RPA1:SDO2

	// Make RAM executable. See also "char RAM[RAMSIZE]" in globalvars.c
	mBMXSetRAMKernProgOffset(PIC32MX_RAMSIZE-RAMSIZE);
	mBMXSetRAMUserDataOffset(PIC32MX_RAMSIZE);
	mBMXSetRAMUserProgOffset(PIC32MX_RAMSIZE);

	INTEnableSystemMultiVectoredInt();
	IPC2bits.T2IP = 4;			// 割り込みレベル4
	IFS0bits.T2IF = 0;
	IEC0bits.T2IE = 1;			// タイマ2割り込み有効化
	PR2=3125;				// 60分の1秒
	T2CON = 0x8070;				// プリスケーラ1:256、タイマ2開始

	init_textgraph();// パレット設定、LCDクリア
	setcursor(0,0,COLOR_NORMALTEXT);
	keystatus=0;

	// Show blue screen if exception before soft reset.
	blue_screen();

	printstr("MachiKania BASIC System\n");
	printstr(" Ver "SYSVER1" "SYSVER2" by KENKEN\n");
	printstr("BASIC Compiler "BASVER"\n");
	printstr(" by Katsumi\n\n");
	//SDカードファイルシステム初期化
	setcursorcolor(COLOR_NORMALTEXT);
	printstr("Init File System...");
	// Initialize the File System
	if(FSInit()==FALSE){ //ファイルシステム初期化
		//エラーの場合停止
		setcursorcolor(COLOR_ERRORTEXT);
		printstr("\nFile System Error\n");
		printstr("Insert Correct Card\n");
		printstr("And Reset\n");
		while(1) asm("wait");
	}
	printstr("OK\n");
	readinifile(); //INIファイル読み込み

	wait60thsec(60); //1秒待ち

	// 実行中HEXファイル名がHEXFILEと一致した場合はエディタ起動
	appname=(char*)FILENAME_FLASH_ADDRESS;
	s=HEXFILE;
	while(*s++==*appname++) if(*s==0) texteditor(); //テキストエディター呼び出し

	// 実行中HEXファイル名の「.HEX」を「.BAS」に置き換えてBASファイルを実行
	appname=(char*)FILENAME_FLASH_ADDRESS;
	s=tempfile;
	while(*appname!='.') *s++=*appname++;
	appname=".BAS";
	while(*appname!=0) *s++=*appname++;
	*s=0;
	// buttonmode(); //ボタン有効化
	g_disable_break=1; // Breakキー無効化
	runbasic(tempfile,0);
	while(1) asm(WAIT);
}
