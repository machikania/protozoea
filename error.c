/*
   This file is provided under the LGPL license ver 2.1.
   Written by Katsumi.
   http://hp.vector.co.jp/authors/VA016157/
   kmorimatsu@users.sourceforge.jp
*/

/*
	This file is shared by Megalopa and Zoea
*/

#include "compiler.h"

const char* g_err_str[]={
	"Syntax error",
	"Not enough binary space",
	"Not enough memory",
	"Divided by zero",
	"Not yet implemented",
	"Label or line number not found: ",
	"Label too long or too short",
	"String too complexed",
	"Data not found",
	"Unknown error:",
	"Music syntax error:'",
	" found more than twice",
	"Break",
	"Unexpected NEXT or RETURN statement",
	"Cannot assign temporary block",
	"GOSUB fuction cannot be used after string-handling",
	"Invalid BREAK statement in line ",
	"Invalid ELSE/IF statement in line ",
	"Invalid parameter(s)",
	"File error",
	"Invalid variable name",
	"WAVE format error",
	"ERR_COMPILE_CLASS",
	"Class not found",
	"Not an object",
	" is not public field/method",
	"Valid only in class file",
	"Invalid in class file",
	"INIT method does not exist",
	"ERR_OPTION_CLASSCODE",
};

char* resolve_label(int s6){
	static char str[7];
	int i,j;
	if (s6<65536) {
		// Line number
		for(i=0;i<5;i++){
			str[5-i]='0'+rem10_32(s6);
			s6=div10_32(s6);
		}
		str[6]=0x00;
		for(j=1;j<5;j++){
			if (str[j]!='0') break;
		}
		return (char*)(str+j);
	} else {
		// Label
		s6-=65536;
		str[6]=0x00;
		for(i=5;0<=i;i--){
			if (s6<37) {
				// First character must be A-Z or _, corresponding to 1-27 but not 10-36.
				// See get_label() for the detail.
				if (s6==27) str[i]='_';
				else str[i]=s6-1+'A';
				break;
			} else {
				// From second, 0-9 corresponds to 0-9 and A-Z corresponds to 10-36.
				str[i]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_"[rem37_31(s6)];
				s6=div37_31(s6);
			}
		}
		return (char*)(str+i);
	}
}

void pre_end_addr(int s6){
	int i,j;
	char str[7];
	// Cool down the system
	stop_music();
	// Resolve line and show it
	if (s6<0) s6=s6&0x7fffffff;
	g_label=s6;
	if (s6<65536) {
		// Line number
		printstr("\nIn line ");
	} else {
		// Label
		printstr("\nAfter label ");
	}
	printstr(resolve_label(s6));
	asm volatile("la $v0,%0"::"i"(&g_end_addr));
	asm volatile("lw $v0,0($v0)");
	asm volatile("nop");
	asm volatile("jr $v0");
}

#define end_exec() \
	asm volatile("addu $a0,$s6,$zero");\
	asm volatile("j pre_end_addr")

void err_break(void){
	stop_music();
	printstr(ERR_BREAK);
	end_exec();
}

void err_data_not_found(void){
	printstr(ERR_DATA_NF);
	end_exec();
}

void err_label_not_found(void){
	printstr(ERR_LABEL_NF);
	printstr(resolve_label(g_label));
	printstr("\n");
	end_exec();
}

void err_div_zero(void){
	printstr(ERR_DIV_0);
	end_exec();
}

void err_no_mem(void){
	printstr(ERR_NE_MEMORY);
	end_exec();
}

void err_str_complex(void){
	printstr(ERR_STR_COMPLEX);
	end_exec();
}

void err_unknown(void){
	asm volatile("la $v0,%0"::"i"(&g_temp));
	asm volatile("sw $ra,0($v0)");
	printstr(ERR_UNKNOWN);
	printhex32(g_temp);
	end_exec();
}

void err_music(char* str){
	printstr(ERR_MUSIC);
	printstr(str);
	printstr("'\n");
	// Restore s6 from g_s6
	asm volatile("la $s6,%0"::"i"(&g_s6));
	asm volatile("lw $s6,0($s6)");
	end_exec();
}

void err_unexp_next(void){
	printstr(ERR_UNEXP_NEXT);
	end_exec();	
}

void err_no_block(void){
	printstr(ERR_NO_BLOCK);
	end_exec();	
}

void err_invalid_param(void){
	printstr(ERR_INVALID_PARAM);
	end_exec();	
}

void err_file(void){
	printstr(ERR_FILE);
	end_exec();	
}

void err_wave(void){
	printstr(ERR_WAVE);
	end_exec();	
}

void err_not_obj(void){
	printstr(ERR_NOT_OBJ);
	end_exec();	
}

void err_not_field(int fieldname, int classname){
	printstr(resolve_label(classname));
	printchar('.');
	printstr(resolve_label(fieldname & 0x7FFFFFFF));
	printstr(ERR_NOT_FIELD);
	end_exec();	
}

void err_str(char* str){
	printstr(str);
	end_exec();	
}