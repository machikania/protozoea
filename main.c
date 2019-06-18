/*
   This file is provided under the LGPL license ver 2.1.
   Written by K.Tanaka & Katsumi
   http://www.ze.em-net.ne.jp/~kenken/index.html
   http://hp.vector.co.jp/authors/VA016157/
*/

// main.c
// MachiKania BASIC System Ver Zoea
// KM-BASIC �����J�����s�� for PIC32MX170F256B / PIC32MX270F256B by K.Tanaka
// LCD�o�̓o�[�W�����i�e�X�g�j by K.Tanaka

// ���p�V�X�e��
// libsdfsio.a �F SD�J�[�h�A�N�Z�X�p���C�u����

// System clock :48MHz  FRC with PLL (8/2*24/2)
// Peripheral clock :48MHz

/*
	PIC32MX �y���t�F�����g�p��
	
	���荞��
		NTSC,   Timer2, vector  8, priority 5
		MUSIC,  CS0,    vector  1, priority 2
		TIMER,  Timer1, vector  4, priority 3
		INT,    CS1,    vector  2, priority 1
	
	�^�C�}�[
		Timer1 BASIC�p�^�C�}�[
		Timer2 NTSC
		Timer3 MUSIC/PWM
		Timer4 MUSIC
		Timer5 ���g�p
	
	DMA
		DMA0 ���g�p
		DMA1 ���g�p
		DMA2 MUSIC
		DMA3 ���g�p
	
	Output compair
		OC1 ���g�p
		OC2 ���g�p
		OC3 ���g�p
		OC4 MUSIC/PWM
		OC5 ���g�p
		
	SPI
		SPI1 ���g�p
		SPI2 �}���`���f�B�A�J�[�h
	
	I2C
		I2C1 ���g�p
		I2C2 ���g�p
	
	�|�[�g�g�p
		A0  MMC
		A1  MMC
		A2  Audio
		A3  LCD/button�؊���
		A4  MMC
		B0  LCD
		B1  LCD
		B2  LCD/button
		B3  LCD/button
		B4  LCD/button
		B5  LCD/button
		B6  ���g�p
		B7  LCD/button
		B8  LCD/button
		B9  PS2/button
		B10 LCD
		B11 LCD
		B12 ���g�p
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

// INI�t�@�C���w��L�[���[�h�i8�����ȓ��j
const char InitKeywords[][9]={
	"106KEY","101KEY","NUMLOCK","CAPSLOCK","SCRLLOCK"
};

void freadline(char *s,FSFILE *fp){
// �t�@�C������1�s�ǂݍ��݁A�z��s�ɕԂ�
// �ő�8�����܂ŁB9�����ȏ�̏ꍇ����
// #�܂���0x20�ȉ��̃R�[�h���������ꍇ�A�ȍ~�͖���
// s:9�o�C�g�ȏ�̔z��
// fp:�t�@�C���|�C���^
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
	if(n>8) *s=0; //9�����ȏ�̕�����̏ꍇ�͖���
	//�ȍ~�̕����͖���
	while(FSfread(&c,1,1,fp) && c!='\n') ;
}
int searchinittext(char *s){
// InitKeywords�z��̒����當����s��T���A�ʒu�����ꍇ���Ԗڂ���Ԃ�
// ������Ȃ������ꍇ-1��Ԃ�
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
//	lockkey=0; //INI�t�@�C�������݂���ꍇ�ALock�֘A�L�[��INI�t�@�C���ɏ]��
	while(1){
		if(FSfeof(fp)) break;
		freadline(inittext,fp);
		switch(searchinittext(inittext)){
			case 0:
//				keytype=0;//���{��L�[�{�[�h
				break;
			case 1:
//				keytype=1;//�p��L�[�{�[�h
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

	/* �|�[�g�̏����ݒ� */
	TRISA = 0x0000; // PORTA�S�ďo��
	ANSELA = 0x0000; // �S�ăf�W�^��
	TRISB = 0x0000; // PORTB�S�ďo�́i�{�^���ǂݍ��ݎ��̂ݓ��͂ɐ؂�ւ���j
	ANSELB = 0x0000; // �S�ăf�W�^��
	KEY_DS;
	LCD_CS_LO;

	// ���Ӌ@�\�s�����蓖��
	SDI2R=2; //RPA4:SDI2
	RPA1R=4; //RPA1:SDO2

	// Make RAM executable. See also "char RAM[RAMSIZE]" in globalvars.c
	mBMXSetRAMKernProgOffset(PIC32MX_RAMSIZE-RAMSIZE);
	mBMXSetRAMUserDataOffset(PIC32MX_RAMSIZE);
	mBMXSetRAMUserProgOffset(PIC32MX_RAMSIZE);

	INTEnableSystemMultiVectoredInt();
	IPC2bits.T2IP = 4;			// ���荞�݃��x��4
	IFS0bits.T2IF = 0;
	IEC0bits.T2IE = 1;			// �^�C�}2���荞�ݗL����
	PR2=3125;				// 60����1�b
	T2CON = 0x8070;				// �v���X�P�[��1:256�A�^�C�}2�J�n

	init_textgraph();// �p���b�g�ݒ�ALCD�N���A
	setcursor(0,0,COLOR_NORMALTEXT);
	keystatus=0;

	// Show blue screen if exception before soft reset.
	blue_screen();

	printstr("MachiKania BASIC System\n");
	printstr(" Ver "SYSVER1" "SYSVER2" by KENKEN\n");
	printstr("BASIC Compiler "BASVER"\n");
	printstr(" by Katsumi\n\n");
	//SD�J�[�h�t�@�C���V�X�e��������
	setcursorcolor(COLOR_NORMALTEXT);
	printstr("Init File System...");
	// Initialize the File System
	if(FSInit()==FALSE){ //�t�@�C���V�X�e��������
		//�G���[�̏ꍇ��~
		setcursorcolor(COLOR_ERRORTEXT);
		printstr("\nFile System Error\n");
		printstr("Insert Correct Card\n");
		printstr("And Reset\n");
		while(1) asm("wait");
	}
	printstr("OK\n");
	readinifile(); //INI�t�@�C���ǂݍ���

	wait60thsec(60); //1�b�҂�

	// ���s��HEX�t�@�C������HEXFILE�ƈ�v�����ꍇ�̓G�f�B�^�N��
	appname=(char*)FILENAME_FLASH_ADDRESS;
	s=HEXFILE;
	while(*s++==*appname++) if(*s==0) texteditor(); //�e�L�X�g�G�f�B�^�[�Ăяo��

	// ���s��HEX�t�@�C�����́u.HEX�v���u.BAS�v�ɒu��������BAS�t�@�C�������s
	appname=(char*)FILENAME_FLASH_ADDRESS;
	s=tempfile;
	while(*appname!='.') *s++=*appname++;
	appname=".BAS";
	while(*appname!=0) *s++=*appname++;
	*s=0;
	// buttonmode(); //�{�^���L����
	g_disable_break=1; // Break�L�[������
	runbasic(tempfile,0);
	while(1) asm(WAIT);
}