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

/*
Operators: (upper ones have higher priority
()
* / %
+ -
<< >>
< <= > >=
= !=
XOR
AND
OR
*/


const unsigned char g_priority[]={
	0, // OP_VOID
	1, // OP_OR
	2, // OP_AND
	3, // OP_XOR
	4,4, // OP_EQ, OP_NEQ
	5,5,5,5, // OP_LT, OP_LTE, OP_MT, OP_MTE
	6,6, // OP_SHL, OP_SHR
	7,7, // OP_ADD, OP_SUB
	8,8,8 // OP_MUL, OP_DIV, OP_REM
};

enum operator g_last_op;

char* get_operator(void){
	char b1,b2,b3;
	next_position();
	b1=g_source[g_srcpos];
	b2=g_source[g_srcpos+1];
	b3=g_source[g_srcpos+2];
	switch(b1){
		case '%': g_last_op=OP_REM; break;
		case '/': g_last_op=OP_DIV; break;
		case '*': g_last_op=OP_MUL; break;
		case '-': g_last_op=OP_SUB; break;
		case '+': g_last_op=OP_ADD; break;
		case '>':
			if (b2=='>') {
				g_srcpos++;
				g_last_op=OP_SHR;
			} else if (b2=='=') {
				g_srcpos++;
				g_last_op=OP_MTE;
			} else {
				g_last_op=OP_MT;
			}
			break;
		case '<':
			if (b2=='<') {
				g_srcpos++;
				g_last_op=OP_SHL;
			} else if (b2=='=') {
				g_srcpos++;
				g_last_op=OP_LTE;
			} else {
				g_last_op=OP_LT;
			}
			break;
		case '!':
			if (b2!='=') return ERR_SYNTAX;
			g_srcpos++;
			g_last_op=OP_NEQ;
			break;
		case '=':
			if (b2=='=') g_srcpos++;
			g_last_op=OP_EQ;
			break;
		case 'X':
			if (b2!='O') return ERR_SYNTAX;
			if (b3!='R') return ERR_SYNTAX;
			g_srcpos++;
			g_srcpos++;
			g_last_op=OP_XOR;
			break;
		case 'O':
			if (b2!='R') return ERR_SYNTAX;
			g_srcpos++;
			g_last_op=OP_OR;
			break;
		case 'A':
			if (b2!='N') return ERR_SYNTAX;
			if (b3!='D') return ERR_SYNTAX;
			g_srcpos++;
			g_srcpos++;
			g_last_op=OP_AND;
			break;
		default:
			return ERR_SYNTAX;
	}
	g_srcpos++;
	return 0;
}

char* get_floatOperator(void){
	char* err;
	int spos;
	next_position();
	spos=g_srcpos;
	err=get_operator();
	if (err) return err;
	switch(g_last_op){
		// Following operators cannot be used for float values.
		case OP_XOR:
		case OP_REM:
		case OP_SHR:
		case OP_SHL:
			g_srcpos=spos;
			return ERR_SYNTAX;
		default:
			return 0;
	}
}

char* calculation(enum operator op){
	// $v0 = $v1 <op> $v0;
	switch(op){
		case OP_OR:
			check_obj_space(1);
			g_object[g_objpos++]=0x00621025; // or          v0,v1,v0
			break;
		case OP_AND:
			check_obj_space(1);
			g_object[g_objpos++]=0x00621024; // and         v0,v1,v0
			break;
		case OP_XOR:
			check_obj_space(1);
			g_object[g_objpos++]=0x00621026; // xor         v0,v1,v0
			break;
		case OP_EQ:
			check_obj_space(2);
			g_object[g_objpos++]=0x00621026; // xor         v0,v1,v0
			g_object[g_objpos++]=0x2C420001; // sltiu       v0,v0,1
			break;
		case OP_NEQ:
			check_obj_space(2);
			g_object[g_objpos++]=0x00621026; // xor         v0,v1,v0
			g_object[g_objpos++]=0x0002102B; // sltu        v0,zero,v0
			break;
		case OP_LT:
			check_obj_space(1);
			g_object[g_objpos++]=0x0062102A; // slt         v0,v1,v0
			break;
		case OP_LTE:
			check_obj_space(2);
			g_object[g_objpos++]=0x0043102A; // slt         v0,v0,v1
			g_object[g_objpos++]=0x38420001; // xori        v0,v0,0x1
			break;
		case OP_MT:
			check_obj_space(1);
			g_object[g_objpos++]=0x0043102A; // slt         v0,v0,v1
			break;
		case OP_MTE:
			check_obj_space(2);
			g_object[g_objpos++]=0x0062102A; // slt         v0,v1,v0
			g_object[g_objpos++]=0x38420001; // xori        v0,v0,0x1
			break;
		case OP_SHR:
			check_obj_space(1);
			g_object[g_objpos++]=0x00431006; // srlv        v0,v1,v0
			break;
		case OP_SHL:
			check_obj_space(1);
			g_object[g_objpos++]=0x00431004; // sllv        v0,v1,v0
			break;
		case OP_ADD:
			check_obj_space(1);
			g_object[g_objpos++]=0x00621021; // addu        v0,v1,v0
			break;
		case OP_SUB:
			check_obj_space(1);
			g_object[g_objpos++]=0x00621023; // subu        v0,v1,v0
			break;
		case OP_MUL:
			check_obj_space(1);
			g_object[g_objpos++]=0x70621002; // mul         v0,v1,v0
			break;
		case OP_DIV:
			// Note that intterupt functions do not use mflo and mfhi.
			// Probably using div does not cause delay of interrupt.
			check_obj_space(5);
			g_object[g_objpos++]=0x14400003;                                        // bne         v0,zero,label
			g_object[g_objpos++]=0x0062001A;                                        // div         v1,v0
			call_lib_code(LIB_DIV0); // 2 words
 			                                                                        // label:
			g_object[g_objpos++]=0x00001012;                                        // mflo        v0
			break;
		case OP_REM:
			check_obj_space(5);
			g_object[g_objpos++]=0x14400003;                                        // bne         v0,zero,label
			g_object[g_objpos++]=0x0062001A;                                        // div         v1,v0
			call_lib_code(LIB_DIV0); // 2 words
 			                                                                        // label:
			g_object[g_objpos++]=0x00001010;                                        // mfhi        v0
			break;
		default:
			return ERR_SYNTAX;
	}
	return 0;
}

char* calculation_float(enum operator op){
	// $v0 = $a0 <op> $v0;
	// All the calculations will be done in library code, lib_float function (see below).
	call_lib_code(LIB_FLOAT | op);
	return 0;
}

int lib_float(int ia0,int iv0, enum operator a1){
	// This function was called from _call_library().
	// Variable types must be all int.
	// Casting cannot be used.
	// Instead, by using pointer, put as int value, get as float value, 
	// calculate, put as float value, then get as int value for returning.
	volatile float a0,v0;
	((int*)(&a0))[0]=ia0;
	((int*)(&v0))[0]=iv0;
	switch(a1){
		case OP_EQ:
			v0= a0==v0?1:0;
			break;
		case OP_NEQ:
			v0= a0!=v0?1:0;
			break;
		case OP_LT:
			v0= a0<v0?1:0;
			break;
		case OP_LTE:
			v0= a0<=v0?1:0;
			break;
		case OP_MT:
			v0= a0>v0?1:0;
			break;
		case OP_MTE:
			v0= a0>=v0?1:0;
			break;
		case OP_ADD:
			v0= a0+v0;
			break;
		case OP_SUB:
			v0= a0-v0;
			break;
		case OP_MUL:
			v0= a0*v0;
			break;
		case OP_DIV:
			if (v0==0) err_div_zero();
			v0= a0/v0;
			break;
		case OP_OR:
			v0= a0||v0?1:0;
			break;
		case OP_AND:
			v0= a0&&v0?1:0;
			break;
		default:
			err_unknown();
			return 0;
	}
	return ((int*)(&v0))[0];
}; 