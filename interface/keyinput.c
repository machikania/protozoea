/*
   This file is provided under the LGPL license ver 2.1.
   Written by K.Tanaka & Katsumi
   http://www.ze.em-net.ne.jp/~kenken/index.html
   http://hp.vector.co.jp/authors/VA016157/
*/
// �L�[���́A�J�[�\���\���֘A�@�\ by K.Tanaka
// PS/2�L�[�{�[�h���̓V�X�e���A�J���[�e�L�X�g�o�̓V�X�e�����p

#include "videoout.h"
#include "ps2keyboard.h"
#include "keyinput.h"
#include "plib.h"

volatile unsigned short vkey; //���z�L�[�R�[�h
unsigned short keystatus,keystatus2,oldkey; //�ŐV�̃{�^����ԂƑO��̃{�^�����

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
//�{�^����ԓǂݎ��
//keystatus :���݉�����Ă���{�^���ɑΉ�����r�b�g��1�ɂ���
//keystatus2:�O�񉟂���Ă��Ȃ��āA���񉟂��ꂽ�{�^���ɑΉ�����r�b�g��1�ɂ���
	oldkey=keystatus;
	keystatus=readButton();
	keystatus2=keystatus & ~oldkey; //�{�^�������𗣂������`�F�b�N
}
unsigned char ps2readkey(){
// �{�^���ňȉ���PS/2�L�[�{�[�h�p�֐����[���I�ɗ��p�B�߂�l��0
	
// ���͂��ꂽ1�̃L�[�̃L�[�R�[�h���O���[�o���ϐ�vkey�Ɋi�[�i������Ă��Ȃ����0��Ԃ��j
// ����8�r�b�g�F�L�[�R�[�h
// ���8�r�b�g�F�V�t�g��ԁi�����F1�j�A��ʂ���<0><CAPSLK><NUMLK><SCRLK><Win><ALT><CTRL><SHIFT>
// �p���E�L�������̏ꍇ�A�߂�l�Ƃ���ASCII�R�[�h�i����ȊO��0��Ԃ��j
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
// �L�[�{�[�h����1�L�[���͑҂�
// �߂�l �ʏ핶���̏ꍇASCII�R�[�h�A���̑���0�A�O���[�o���ϐ�vkey�ɉ��z�L�[�R�[�h
	unsigned char k;
	unsigned short d;
	d=drawcount;
	while(1){
		while(d==drawcount) asm("wait"); //60����1�b�E�F�C�g
		d=drawcount;
		k=ps2readkey();  //�L�[�o�b�t�@����ǂݍ��݁Ak:�ʏ핶�����͂̏ꍇASCII�R�[�h
		if(vkey) return k;
	}
}
