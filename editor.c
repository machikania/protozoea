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

//�z��RAM[]���Ƀ��������I�m�ۂ��邽�߂̃|�C���^
char *editormallocp;

//unsigned char cwdpath[PATHNAMEMAX]; //���݂̃f�B���N�g���̃p�X��
unsigned char *cwdpath; //���͔̂z��RAM[]�̒��Ɋm�ۂ���

unsigned char tempfile[13]; //�ҏW���̃t�@�C�����A�ꎞ�t�@�C����

//unsigned char filenames[MAXFILENUM][13]; //���[�h���̃t�@�C�����ꗗ�o�b�t�@
unsigned char (*filenames)[13]; //���͔̂z��RAM[]�̒��Ɋm�ۂ���

unsigned char runfile[13]; //���s�t�@�C����

const unsigned char Message1[]="Hit Any Key\n";
const unsigned char Message2[]="File System Error\n";
const unsigned char Message3[]="Retry:[Enter] / Quit:[ESC]\n";
const unsigned char ROOTDIR[]="\\";

unsigned char * editormalloc(int size){
//�z��RAM[]���ɃT�C�Ysize�̗̈���m�ۂ��A�擪�A�h���X��Ԃ�
//�m�ۂł��Ȃ��ꍇ�́A�G���[�\���������~
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
	// 60����n�b�E�F�C�g�i�r�f�I��ʂ̍ŉ��s�M���o�͏I���܂ő҂j
	n+=drawcount;
	while(drawcount!=n) asm(WAIT);
}
int filesystemretry(){
// SD�t�@�C���V�X�e���̍ď������m�F�Ǝ��{
// SD�t�@�C���ւ̕ۑ���ǂݍ��ݎ��Ƀt�@�C���G���[�����������ꍇ�ɌĂяo��
// �߂�l�@0�F�����������A-1�F�������邱�ƂȂ�Escape�Ŕ�����
	unsigned short vk;
	while(1){
		setcursorcolor(COLOR_NORMALTEXT);
		printstr((unsigned char *)Message3); //Retry / Quit
		while(1){
			inputchar(); //1�������͑҂�
			vk=vkey & 0xff;
			if(vk==VK_RETURN || vk==VK_SEPARATOR) break;
			if(vk==VK_ESCAPE || vk==VK_F1) return -1;
		}
		//�t�@�C���V�X�e��������
		if(FSInit()!=FALSE) return 0; //����
		//�G���[�̏ꍇ
		setcursorcolor(COLOR_ERRORTEXT);
		printstr((unsigned char *)Message2);//File System Error
	}
}
void printfilename(unsigned char x,unsigned char y,int f,int num_dir){
// x,y�̈ʒu�Ƀt�@�C�����܂��̓f�B���N�g������\��

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
// filenames[]�z��ɓǂݍ��܂ꂽ�t�@�C���܂��̓f�B���N�g������ʕ\�����L�[�{�[�h�őI������
// filenum:�t�@�C���{�f�B���N�g����
// num_dir:�f�B���N�g�����ifilenames[]�͐擪����num_dir-1�܂ł��f�B���N�g���j
// msg:��ʏ㕔�ɕ\�����郁�b�Z�[�W
// �߂�l
//�@filenames[]�̑I�����ꂽ�t�@�C���܂��̓f�B���N�g���ԍ�
//�@-1�F�V�K�f�B���N�g���쐬�Atempfile[]�Ƀf�B���N�g����
//�@-2�F�V�K�t�@�C���쐬�Atempfile[]�Ƀt�@�C����
//�@-3�FESC�L�[�������ꂽ
	int top,f;
	unsigned char *ps,*pd;
	int x,y;
	unsigned char vk;
	//�t�@�C���ꗗ����ʂɕ\��
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
	top=-2;//��ʈ�Ԑ擪�̃t�@�C���ԍ�
	f=-2;//���ݑI�𒆂̃t�@�C���ԍ�
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
		for(x=0;x<twidth-1;x++) printchar(' '); //�ŉ��s�̃X�e�[�^�X�\��������
		switch(vk){
			case VK_UP:
			case VK_NUMPAD8:
				//����L�[
				if(f>=0){
					f-=2;
					if(f<top){
						//��ʍŏ㕔�̏ꍇ�A���ɃX�N���[�����čŏ㕔�Ƀt�@�C����2�\��
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
				//�����L�[
				if(((f+2)&0xfffe)<filenum){
					f+=2;
					if(f>=filenum) f--;
					if(f-top>=(WIDTH_Y-2)*2){
						//��ʍŉ����̏ꍇ�A��ɃX�N���[�����čŉ����Ƀt�@�C����1��or2�\��
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
				//�����L�[
				if(f&1) f--;
				break;
			case VK_RIGHT:
			case VK_NUMPAD6:
				//�E���L�[
				if((f&1)==0 && f+1<filenum) f++;
				break;
			case VK_RETURN: //Enter�L�[
			case VK_SEPARATOR: //�e���L�[��Enter
				if(f==-2){
					//�V�K�t�@�C��
				}
				else if(f==-1){
					//�V�K�f�B���N�g��
				}
				else{
					//�t�@�C�����܂��̓f�B���N�g������tempfile�ɃR�s�[
					ps=filenames[f];
					pd=tempfile;
					while(*ps) *pd++=*ps++;
					*pd=0;
					return f;
				}
//			case VK_ESCAPE:
//			case VK_F1: // START�{�^��
				//ESC�L�[
//				return -3;
		}
	}
}
int getfilelist(int *p_num_dir){
// �J�����g�f�B���N�g���ł̃f�B���N�g���A.BAS�A.TXT�A.INI�t�@�C���ꗗ��ǂݍ���
// *p_num_dir:�f�B���N�g������Ԃ�
// filenames[]:�t�@�C��������уf�B���N�g�����ꗗ
// �߂�l�@�t�@�C���{�f�B���N�g����

	unsigned char *ps,*pd;
	int filenum;
	SearchRec sr;
	filenum=0;
	//�f�B���N�g���̃T�[�`
	if(FindFirst("*",ATTR_DIRECTORY,&sr)==0){
		do{
			//filenames[]�Ƀf�B���N�g�����̈ꗗ��ǂݍ���
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
	//�g���q BAS�t�@�C���̃T�[�`
	if(FindFirst("*.BAS",ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_ARCHIVE,&sr)==0){
		do{
			//filenames[]�Ƀt�@�C�����̈ꗗ��ǂݍ���
			ps=sr.filename;
			pd=filenames[filenum];
			while(*ps!=0) *pd++=*ps++;
			*pd=0;
			filenum++;
		}
		while(!FindNext(&sr) && filenum<MAXFILENUM);
	}
	if(filenum>=MAXFILENUM) return filenum;
	//�g���q TXT�t�@�C���̃T�[�`
	if(FindFirst("*.TXT",ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_ARCHIVE,&sr)==0){
		do{
			//filenames[]�Ƀt�@�C�����̈ꗗ��ǂݍ���
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
// SD�J�[�h����t�@�C����I��
// runfile[]�Ƀt�@�C�������L��
// �Ώۃt�@�C���g���q BAS�����TXT
	int filenum,f;
	unsigned char *ps,*pd;
	int num_dir;//�f�B���N�g����

	//�t�@�C���̈ꗗ��SD�J�[�h����ǂݏo��
	cls();
	while(1){
		filenum=getfilelist(&num_dir); //�f�B���N�g���A�t�@�C�����ꗗ��ǂݍ���
		if(filenum==0){
			setcursorcolor(COLOR_ERRORTEXT);
			printstr(".BAS or .TXT File Not Found\n");
			printstr((unsigned char *)Message1);// Hit Any Key
			inputchar(); //1�������͑҂�
			continue;
		}
		//�t�@�C���̑I��
		f=select_dir_file(filenum,num_dir,"Load");
		if(f<num_dir){
			//�f�B���N�g���ύX���āA�ēx�t�@�C���ꗗ��ʂ�
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
//KM-BASIC�R���p�C�������s
// test 0:�R���p�C���Ǝ��s�A0�ȊO:�R���p�C���݂̂ŏI��
	int er2;

	cls();
	setcursor(0,0,COLOR_NORMALTEXT);
//	set_width(0);//30�������[�h�ɐݒ�

	// Enable Break key
	g_disable_break=0;
	//KM-BASIC���s
	er2=runbasic(runfile,test);

	stopPCG();//�V�X�e���t�H���g�ɖ߂�
	setcursorcolor(COLOR_NORMALTEXT);
	printchar('\n');
	printstr((unsigned char *)Message1);// Hit Any Key
	do ps2readkey(); //�L�[�o�b�t�@����ɂȂ�܂œǂݏo��
	while(vkey!=0);
	inputchar(); //1�������͑҂�
	init_textgraph(); //�p���b�g�������̂��߉�ʏ�����
	//��ʃ��[�h��߂�
//	if(widthmode==WIDTH_X1) set_width(0);
//	else set_width(1);

}
void texteditor(void){
	editormallocp=RAM;
	cwdpath=editormalloc(PATHNAMEMAX);
	filenames=(unsigned char (*)[])editormalloc(MAXFILENUM*13);

	cwdpath[0]='\\'; //�J�����g�f�B���N�g�������[�g�ɐݒ�
	cwdpath[1]=0;
	while(1){
		fileselect();
		run(0);
	}
}
