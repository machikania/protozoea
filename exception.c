/*
   This file is provided under the LGPL license ver 2.1.
   Written by K.Tanaka & Katsumi
   http://www.ze.em-net.ne.jp/~kenken/index.html
   http://hp.vector.co.jp/authors/VA016157/
*/

/*
	This file is shared by Megalopa and Zoea
*/

#include <xc.h>
#include "api.h"
#include "main.h"
#include "compiler.h"

/*
	Enable following line if memory dump is needed when exception occurs.
*/

//#define DUMPFILE "~~MEMORY.DMP"

#ifdef DUMPFILE
void dumpMemory(){
	unsigned int i;
	FSFILE *fp;
	printstr("\n"DUMPFILE" ");
	if(FSInit()==FALSE){
		printstr("cannot be created.\n");
		return;
	}
	fp=FSfopen(DUMPFILE,"w");
	if(fp==NULL) {
		printstr("not saved.\n");
		return;
	}
	for(i=0;i<PERSISTENT_RAM_SIZE;i+=512){
		if (FSfwrite(&RAM[i],1,512,fp)<512) break;
	}
	FSfclose(fp);	
	printstr("saved.\n");
}
#else
void dumpMemory(){}
#endif //ifdef DUMPFILE

void _general_exception_handler_main (void);
void _general_exception_handler (void){
	asm volatile("la $sp,%0"::"i"(&RAM[RAMSIZE-4]));
	asm volatile("j _general_exception_handler_main");
}
void _general_exception_handler_main (void){
	int i;
	// $v1 is g_ex_data
	asm volatile("la $v1,%0"::"i"(&g_ex_data[0]));
	// Prepare proper stack area before SoftReset
	asm volatile("addiu $sp,$v1,0xfff0");
	// g_ex_data[2]=$s6
	asm volatile("sw $s6,8($v1)");
	// g_ex_data[3]=Cause
	asm volatile("mfc0 $v0,$13");
	asm volatile("sw $v0,12($v1)");
	// g_ex_data[4]=EPC 
	asm volatile("mfc0 $v0,$14");
	asm volatile("sw $v0,16($v1)");
	// Exception occured
	g_ex_data[0]=1;
	// g_s6
	g_ex_data[1]=g_s6;
	// Clear 2 MLB bits of EPC
	g_ex_data[4]&=0xfffffffc;
	// If EPC is within RAM, store data in exception area.
	if ((int)(&RAM[0])<=g_ex_data[4] && g_ex_data[4] <(int)(&RAM[RAMSIZE])) {
		// g_ex_data[5] - g_ex_data[12]: assembly
		for(i=-3;i<=3;i++){
			g_ex_data[i+8]=((int*)g_ex_data[4])[i];
		}
	}
	// Wait until all buttons are released and reset MachiKania.
	#ifdef __DEBUG
		asm volatile("j 0xBFC00000");
	#else
		for(i=0;i<100000;i++){
			if((readbuttons()&(KEYUP|KEYDOWN|KEYLEFT|KEYRIGHT|KEYSTART|KEYFIRE))
				!=(KEYUP|KEYDOWN|KEYLEFT|KEYRIGHT|KEYSTART|KEYFIRE)) i=0;
		}
		RCONbits.POR=0;
		RCONbits.EXTR=0;
		asm volatile("j 0x9FC00000");
	#endif
}

void blue_screen(void){
	int i,j,s6,s6g;
	unsigned int* opos;
	if (RCONbits.POR || RCONbits.EXTR) {
		// After power on or reset. Reset flags and return.
		RCONbits.POR=0;
		RCONbits.EXTR=0;
		for(i=0;i<RAMSIZE;i++){
			// Reset all RAM area including g_ex_data[]
			RAM[i]=0;
		}
		return;
	} else if (g_ex_data[0]==0) {
		// No exception found.
		return;
	}
	// Exception occured before SoftReset().
	// Prepare data
	s6=g_ex_data[2];
	s6g=g_ex_data[1];
	s6=s6&0x7fffffff;
	s6g=s6g&0x7fffffff;
	opos=(int*)g_ex_data[4];
	//set_bgcolor(255,0,0);
	printstr("STOP");
	printstr("\nException at ");
	printhex32(g_ex_data[4]);
	printstr("\n      Cause: ");
	printhex32(g_ex_data[3]);
	printstr("\n ");
	switch((g_ex_data[3]>>2)&0x1f){
		case 0:  printstr("(Interrupt)"); break;
		case 1:  printstr("(TLB modification)"); break;
		case 2:  printstr("(TLB load/fetch)"); break;
		case 3:  printstr("(TLB store)"); break;
		case 4:  printstr("(Address load/fetch error )"); break;
		case 5:  printstr("(Address store error)"); break;
		case 6:  printstr("(Bus fetch error)"); break;
		case 7:  printstr("(Bus load/store error)"); break;
		case 8:  printstr("(Syscall)"); break;
		case 9:  printstr("(Breakpoint)"); break;
		case 10: printstr("(Reserved instruction)"); break;
		case 11: printstr("(Coprocessor Unusable)"); break;
		case 12: printstr("(Integer Overflow)"); break;
		case 13: printstr("(Trap)"); break;
		case 23: printstr("(Reference to Watch address)"); break;
		case 24: printstr("(Machine check)"); break;
		default: printstr("(Unknown)"); break;
	}
	printstr("\n         s6: ");
	printstr(resolve_label(s6));
	printstr("\n       g_s6: ");
	printstr(resolve_label(s6g));
	printstr("\n");
	printstr("Reset MachiKania to contine.\n\n");
	// Show code where the exception happened.
	for(i=-3;i<=3;i++){
		printstr("\n ");
		printhex32((unsigned int)&opos[i]);
		printstr("  ");
		if ((unsigned int)&RAM[0]<=(unsigned int)&opos[i] && (unsigned int)&opos[i]<(unsigned int)&RAM[RAMSIZE]) {
			// Exception in RAM[RAMSIZE] area
			printhex32(g_ex_data[i+8]);		
		} else if (   0xA0000000<=(unsigned int)&opos[i] && (unsigned int)&opos[i]<0xA0000000+PIC32MX_RAMSIZE
			|| 0x9D000000<=(unsigned int)&opos[i] && (unsigned int)&opos[i]<=0x9D000000+PIC32MX_FLASHSIZE) {
			// Exception in outside RAM[RAMSIZE] or flash area
			printhex32(opos[i]);
		} else {
			printstr("********");
		}
	}
	printstr("\n");

#ifndef __DEBUG
	dumpMemory();
#endif
	while(1) asm("wait");
}
