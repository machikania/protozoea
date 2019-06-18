/*
   This file is provided under the LGPL license ver 2.1.
   Written by K.Tanaka & Katsumi
   http://www.ze.em-net.ne.jp/~kenken/index.html
   http://hp.vector.co.jp/authors/VA016157/
*/
// キー入力、カーソル表示関連機能 by K.Tanaka
// PS/2キーボード入力システム、カラーテキスト出力システム利用

#include "videoout.h"
#include "ps2keyboard.h"
#include "keyinput.h"
#include "plib.h"

volatile unsigned short vkey; //仮想キーコード
unsigned short keystatus,keystatus2,oldkey; //最新のボタン状態と前回のボタン状態

int lineinput(char *s,int n){
	return 0;
}

unsigned short readButton(void){
	unsigned short k;
	LCD_CS_HI;
	asm("nop");
	KEY_EN;
	asm("nop");
	TRISBSET=KEYMASK;
	asm("nop");
	k=~KEYPORT & KEYMASK;
	KEY_DS;
	asm("nop");
	TRISBCLR=KEYMASK;
	asm("nop");
	LCD_CS_LO;
	return k;
}
void keycheck(void){
//ボタン状態読み取り
//keystatus :現在押されているボタンに対応するビットを1にする
//keystatus2:前回押されていなくて、今回押されたボタンに対応するビットを1にする
	oldkey=keystatus;
	keystatus=readButton();
	keystatus2=keystatus & ~oldkey; //ボタンから手を離したかチェック
}
unsigned char ps2readkey(){
// ボタンで以下のPS/2キーボード用関数を擬似的に利用。戻り値は0
	
// 入力された1つのキーのキーコードをグローバル変数vkeyに格納（押されていなければ0を返す）
// 下位8ビット：キーコード
// 上位8ビット：シフト状態（押下：1）、上位から<0><CAPSLK><NUMLK><SCRLK><Win><ALT><CTRL><SHIFT>
// 英数・記号文字の場合、戻り値としてASCIIコード（それ以外は0を返す）
	keycheck();
	vkey=0;

	if(keystatus2==KEYFIRE) vkey=VK_RETURN;
	if(keystatus2==KEYSTART) vkey=VK_F1;
	if(keystatus2==KEYDOWN) vkey=VK_DOWN;
	if(keystatus2==KEYLEFT) vkey=VK_LEFT;
	if(keystatus2==KEYRIGHT) vkey=VK_RIGHT;
	if(keystatus2==KEYUP) vkey=VK_UP;

	return 0;
}

unsigned char inputchar(void){
// キーボードから1キー入力待ち
// 戻り値 通常文字の場合ASCIIコード、その他は0、グローバル変数vkeyに仮想キーコード
	unsigned char k;
	unsigned short d;
	d=drawcount;
	while(1){
		while(d==drawcount) asm("wait"); //60分の1秒ウェイト
		d=drawcount;
		k=ps2readkey();  //キーバッファから読み込み、k:通常文字入力の場合ASCIIコード
		if(vkey) return k;
	}
}
