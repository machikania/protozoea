/*
   This file is provided under the LGPL license ver 2.1.
   Written by K.Tanaka & Katsumi
   http://www.ze.em-net.ne.jp/~kenken/index.html
   http://hp.vector.co.jp/authors/VA016157/
*/

#include <xc.h>
#include "api.h"
#include "editor.h"
#include "compiler.h"
#include "main.h"

//配列RAM[]内にメモリ動的確保するためのポインタ
char *editormallocp;

//unsigned char cwdpath[PATHNAMEMAX]; //現在のディレクトリのパス名
unsigned char *cwdpath; //実体は配列RAM[]の中に確保する

unsigned char tempfile[13]; //編集中のファイル名、一時ファイル名

//unsigned char filenames[MAXFILENUM][13]; //ロード時のファイル名一覧バッファ
unsigned char (*filenames)[13]; //実体は配列RAM[]の中に確保する

unsigned char runfile[13]; //実行ファイル名

const unsigned char Message1[]="Hit Any Key\n";
const unsigned char Message2[]="File System Error\n";
const unsigned char Message3[]="Retry:[Enter] / Quit:[ESC]\n";
const unsigned char ROOTDIR[]="\\";

unsigned char * editormalloc(int size){
//配列RAM[]内にサイズsizeの領域を確保し、先頭アドレスを返す
//確保できない場合は、エラー表示し動作停止
	unsigned char *p;
	if(editormallocp+size>RAM+RAMSIZE){
		printstr("Cannot allocate memory");
		while(1) asm("wait");
	}
	p=editormallocp;
	editormallocp+=size;
	return p;
}

void wait60thsec(unsigned short n){
	// 60分のn秒ウェイト（ビデオ画面の最下行信号出力終了まで待つ）
	n+=drawcount;
	while(drawcount!=n) asm(WAIT);
}
int filesystemretry(){
// SDファイルシステムの再初期化確認と実施
// SDファイルへの保存や読み込み時にファイルエラーが発生した場合に呼び出す
// 戻り値　0：初期化成功、-1：成功することなくEscapeで抜けた
	unsigned short vk;
	while(1){
		setcursorcolor(COLOR_NORMALTEXT);
		printstr((unsigned char *)Message3); //Retry / Quit
		while(1){
			inputchar(); //1文字入力待ち
			vk=vkey & 0xff;
			if(vk==VK_RETURN || vk==VK_SEPARATOR) break;
			if(vk==VK_ESCAPE || vk==VK_F1) return -1;
		}
		//ファイルシステム初期化
		if(FSInit()!=FALSE) return 0; //成功
		//エラーの場合
		setcursorcolor(COLOR_ERRORTEXT);
		printstr((unsigned char *)Message2);//File System Error
	}
}
void printfilename(unsigned char x,unsigned char y,int f,int num_dir){
// x,yの位置にファイル名またはディレクトリ名を表示

	if(f==-2){
		setcursor(x,y,COLOR_ERRORTEXT);
		printchar('<');
		printstr("New FILE");
		printchar('>');
	}
	else if(f==-1){
		setcursor(x,y,COLOR_ERRORTEXT);
		printchar('<');
		printstr("New Dir");
		printchar('>');
	}
	else if(f<num_dir){
		setcursor(x,y,COLOR_DIR);
		printchar('[');
		printstr(filenames[f]);
		printchar(']');
	}
	else{
		setcursor(x,y,COLOR_NORMALTEXT);
		printstr(filenames[f]);
	}
}
int select_dir_file(int filenum,int num_dir, unsigned char* msg){
// filenames[]配列に読み込まれたファイルまたはディレクトリを画面表示しキーボードで選択する
// filenum:ファイル＋ディレクトリ数
// num_dir:ディレクトリ数（filenames[]は先頭からnum_dir-1までがディレクトリ）
// msg:画面上部に表示するメッセージ
// 戻り値
//　filenames[]の選択されたファイルまたはディレクトリ番号
//　-1：新規ディレクトリ作成、tempfile[]にディレクトリ名
//　-2：新規ファイル作成、tempfile[]にファイル名
//　-3：ESCキーが押された
	int top,f;
	unsigned char *ps,*pd;
	int x,y;
	unsigned char vk;
	//ファイル一覧を画面に表示
	cls();
	setcursor(0,0,COLOR_NORMALTEXT);
	printstr(msg);
	printstr(": ");
	setcursorcolor(4);
//	printstr("Select&[FIRE] / [START]\n");
	printstr("Select & [FIRE]\n");
	for(f=-2;f<filenum;f++){
		x=(f&1)*15+1;
		y=(f+2)/2+1;
		if(y>=WIDTH_Y-1) break;
		printfilename(x,y,f,num_dir);
	}
	top=-2;//画面一番先頭のファイル番号
	f=-2;//現在選択中のファイル番号
	while(1){
		setcursor((f&1)*15,(f-top)/2+1,5);
		printchar(0x1c);// Right Arrow
		cursor--;
		while(1){
			inputchar();
			vk=vkey & 0xff;
			if(vk) break;
		}
		printchar(' ');
		setcursor(0,WIDTH_Y-1,COLOR_NORMALTEXT);
		for(x=0;x<twidth-1;x++) printchar(' '); //最下行のステータス表示を消去
		switch(vk){
			case VK_UP:
			case VK_NUMPAD8:
				//上矢印キー
				if(f>=0){
					f-=2;
					if(f<top){
						//画面最上部の場合、下にスクロールして最上部にファイル名2つ表示
						if(twidth==WIDTH_X1){
							setcursor(WIDTH_X1-1,WIDTH_Y-2,COLOR_NORMALTEXT);
							while(cursor>=TVRAM+WIDTH_X1*2){
								*cursor=*(cursor-WIDTH_X1);
								*(cursor+ATTROFFSET1)=*(cursor+ATTROFFSET1-WIDTH_X1);
								cursor--;
							}
							while(cursor>=TVRAM+WIDTH_X1) *cursor--=' ';
						}
						else{
							setcursor(WIDTH_X2-1,WIDTH_Y-2,COLOR_NORMALTEXT);
							while(cursor>=TVRAM+WIDTH_X2*2){
								*cursor=*(cursor-WIDTH_X2);
								*(cursor+ATTROFFSET2)=*(cursor+ATTROFFSET2-WIDTH_X2);
								cursor--;
							}
							while(cursor>=TVRAM+WIDTH_X2) *cursor--=' ';
						}
						top-=2;
						printfilename(1,1,top,num_dir);
						printfilename(16,1,top+1,num_dir);
					}
				}
				break;
			case VK_DOWN:
			case VK_NUMPAD2:
				//下矢印キー
				if(((f+2)&0xfffe)<filenum){
					f+=2;
					if(f>=filenum) f--;
					if(f-top>=(WIDTH_Y-2)*2){
						//画面最下部の場合、上にスクロールして最下部にファイル名1つor2つ表示
						setcursor(0,1,COLOR_NORMALTEXT);
						if(twidth==WIDTH_X1){
							while(cursor<TVRAM+WIDTH_X1*(WIDTH_Y-2)){
								*cursor=*(cursor+WIDTH_X1);
								*(cursor+ATTROFFSET1)=*(cursor+ATTROFFSET1+WIDTH_X1);
								cursor++;
							}
							while(cursor<TVRAM+WIDTH_X1*(WIDTH_Y-1)) *cursor++=' ';
						}
						else{
							while(cursor<TVRAM+WIDTH_X2*(WIDTH_Y-2)){
								*cursor=*(cursor+WIDTH_X2);
								*(cursor+ATTROFFSET2)=*(cursor+ATTROFFSET2+WIDTH_X2);
								cursor++;
							}
							while(cursor<TVRAM+WIDTH_X2*(WIDTH_Y-1)) *cursor++=' ';
						}
						top+=2;
						printfilename(1,WIDTH_Y-2,f&0xfffe,num_dir);
						if((f|1)<filenum){
							printfilename(16,WIDTH_Y-2,f|1,num_dir);
						}
					}
				}
				break;
			case VK_LEFT:
			case VK_NUMPAD4:
				//左矢印キー
				if(f&1) f--;
				break;
			case VK_RIGHT:
			case VK_NUMPAD6:
				//右矢印キー
				if((f&1)==0 && f+1<filenum) f++;
				break;
			case VK_RETURN: //Enterキー
			case VK_SEPARATOR: //テンキーのEnter
				if(f==-2){
					//新規ファイル
				}
				else if(f==-1){
					//新規ディレクトリ
				}
				else{
					//ファイル名またはディレクトリ名をtempfileにコピー
					ps=filenames[f];
					pd=tempfile;
					while(*ps) *pd++=*ps++;
					*pd=0;
					return f;
				}
//			case VK_ESCAPE:
//			case VK_F1: // STARTボタン
				//ESCキー
//				return -3;
		}
	}
}
int getfilelist(int *p_num_dir){
// カレントディレクトリでのディレクトリ、.BAS、.TXT、.INIファイル一覧を読み込む
// *p_num_dir:ディレクトリ数を返す
// filenames[]:ファイル名およびディレクトリ名一覧
// 戻り値　ファイル＋ディレクトリ数

	unsigned char *ps,*pd;
	int filenum;
	SearchRec sr;
	filenum=0;
	//ディレクトリのサーチ
	if(FindFirst("*",ATTR_DIRECTORY,&sr)==0){
		do{
			//filenames[]にディレクトリ名の一覧を読み込み
			ps=sr.filename;
			pd=filenames[filenum];
			while(*ps!=0) *pd++=*ps++;
			*pd=0;
			filenum++;
		}
		while(!FindNext(&sr) && filenum<MAXFILENUM);
	}
	*p_num_dir=filenum;
	if(filenum>=MAXFILENUM) return filenum;
	//拡張子 BASファイルのサーチ
	if(FindFirst("*.BAS",ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_ARCHIVE,&sr)==0){
		do{
			//filenames[]にファイル名の一覧を読み込み
			ps=sr.filename;
			pd=filenames[filenum];
			while(*ps!=0) *pd++=*ps++;
			*pd=0;
			filenum++;
		}
		while(!FindNext(&sr) && filenum<MAXFILENUM);
	}
	if(filenum>=MAXFILENUM) return filenum;
	//拡張子 TXTファイルのサーチ
	if(FindFirst("*.TXT",ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_ARCHIVE,&sr)==0){
		do{
			//filenames[]にファイル名の一覧を読み込み
			ps=sr.filename;
			pd=filenames[filenum];
			while(*ps!=0) *pd++=*ps++;
			*pd=0;
			filenum++;
		}
		while(!FindNext(&sr) && filenum<MAXFILENUM);
	}
	if(filenum>=MAXFILENUM) return filenum;
	return filenum;
}
void fileselect(void){
// SDカードからファイルを選択
// runfile[]にファイル名を記憶
// 対象ファイル拡張子 BASおよびTXT
	int filenum,f;
	unsigned char *ps,*pd;
	int num_dir;//ディレクトリ数

	//ファイルの一覧をSDカードから読み出し
	cls();
	while(1){
		filenum=getfilelist(&num_dir); //ディレクトリ、ファイル名一覧を読み込み
		if(filenum==0){
			setcursorcolor(COLOR_ERRORTEXT);
			printstr(".BAS or .TXT File Not Found\n");
			printstr((unsigned char *)Message1);// Hit Any Key
			inputchar(); //1文字入力待ち
			continue;
		}
		//ファイルの選択
		f=select_dir_file(filenum,num_dir,"Load");
		if(f<num_dir){
			//ディレクトリ変更して、再度ファイル一覧画面へ
			FSchdir(tempfile);
			continue;
		}
		ps=filenames[f];
		pd=runfile;
		while(*ps) *pd++=*ps++;
		*pd=0;
		return;
	}
}
void run(int test){
//KM-BASICコンパイル＆実行
// test 0:コンパイルと実行、0以外:コンパイルのみで終了
	int er2;

	cls();
	setcursor(0,0,COLOR_NORMALTEXT);
//	set_width(0);//30文字モードに設定

	// Enable Break key
	g_disable_break=0;
	//KM-BASIC実行
	er2=runbasic(runfile,test);

	stopPCG();//システムフォントに戻す
	setcursorcolor(COLOR_NORMALTEXT);
	printchar('\n');
	printstr((unsigned char *)Message1);// Hit Any Key
	do ps2readkey(); //キーバッファが空になるまで読み出し
	while(vkey!=0);
	inputchar(); //1文字入力待ち
	init_textgraph(); //パレット初期化のため画面初期化
	//画面モードを戻す
//	if(widthmode==WIDTH_X1) set_width(0);
//	else set_width(1);

}
void texteditor(void){
	editormallocp=RAM;
	cwdpath=editormalloc(PATHNAMEMAX);
	filenames=(unsigned char (*)[])editormalloc(MAXFILENUM*13);

	cwdpath[0]='\\'; //カレントディレクトリをルートに設定
	cwdpath[1]=0;
	while(1){
		fileselect();
		run(0);
	}
}
