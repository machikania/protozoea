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

const unsigned int g_initial_s5_stack[3]={
	0,                                     // -8($s5): no object
	(unsigned int) &g_initial_s5_stack[2], // -4($s5): previous $s5 (recursive)
	0                                      //  0($s5): no parameter
};

static int g_args_stack;
/*
	See ARGS_SP_XXXX and ARGS_S5_XXXX in compiler.h
	At least 4 stacks are needed even without argument
		 4($sp) = -12($s5): $sp
		 8($sp) =  -8($s5): $v0 - pointer to object or previous -8($s5)
		12($sp) =  -4($s5): previous $s5
		16($sp) =   0($s5): number of arguments
		20($sp) =   4($s5): first argument
	$v0 must be the pointer to an object before comming to this code.
	After this code, -12($s5) must be set to $sp: 0xAEBDFFF4   sw          sp,-12(s5)
*/
char* prepare_args_stack(char start_char){
	// start_char is either ',' or '('
	// When ',' mode, there must be a ',' if argument(s) exist(s).
	// When '(' mode, there shouldn't be a '(', because this character has passed.
	char* err;
	int opos,stack;
	stack=0;
	opos=g_objpos;
	check_obj_space(2);
	g_object[g_objpos++]=0x27BD0000;             // addiu       sp,sp,-xx
	// 8(sp) is for $v0, which is pointer to an object
	g_object[g_objpos++]=0xAFA20008;             // sw          v0,8(sp)
	next_position();
	do {
		if (!stack) {
			// First parameter if exists
			stack=16;
			if (start_char==','){
				if (g_source[g_srcpos]!=',') break;
			} else if (start_char=='('){
				next_position();
				if (g_source[g_srcpos]==')') break;
				g_srcpos--;
			} else {
				return ERR_UNKNOWN;
			}
		}
		g_srcpos++;
		stack+=4;
		err=get_stringFloatOrValue();
		if (err) return err;
		check_obj_space(1);
		g_object[g_objpos++]=0xAFA20000|stack; // sw          v0,xx(sp)
		next_position();
	} while (g_source[g_srcpos]==',');
	// 12(sp) is for $s5, 16(sp) is for # of parameters
	check_obj_space(5);
	g_object[g_objpos++]=0xAFB5000C;             // sw          s5,12(sp)
	g_object[g_objpos++]=0x34020000|(stack/4-4); // ori         v0,zero,xx
	g_object[g_objpos++]=0xAFA20010;             // sw          v0,16(sp)
	g_object[g_objpos++]=0x27B50010;             // addiu       s5,sp,16
	g_object[opos]|=((0-stack)&0xFFFF);          // addiu       sp,sp,-xx (See above)
	// All done. Register # of stacks to global var.
	g_args_stack=stack;
	return 0;
}

char* remove_args_stack(void){
	// Remove stack
	check_obj_space(2);
	g_object[g_objpos++]=0x8FB5000C;              // lw          s5,12(sp)
	g_object[g_objpos++]=0x27BD0000|g_args_stack; // addiu       sp,sp,xx
	return 0;
}

char* args_function_main(void){
	char* err;
	int i;
	err=get_value();
	if (err) return err;
	i=g_object[g_objpos-1];
	if ((i>>16)==0x3402) {
		// Previous object is "ori v0,zero,xxxx".
		i&=0xffff;
		i=i<<2;
		g_object[g_objpos-1]=0x8EA20000|i; //   lw          v0,xx(s5)
	} else {
		check_obj_space(3);
		g_object[g_objpos++]=0x00021080;   //   sll         v0,v0,0x2
		g_object[g_objpos++]=0x02A21021;   //   addu        v0,s5,v0
		g_object[g_objpos++]=0x8C420000;   //   lw          v0,0(v0)
	}
	return 0;	
}

