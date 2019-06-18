/*
   This file is provided under the LGPL license ver 2.1.
   Written by Katsumi.
   http://hp.vector.co.jp/authors/VA016157/
   kmorimatsu@users.sourceforge.jp
*/

/*
	This file is shared by Megalopa and Zoea
*/

#include "./compiler.h"
#include "stdlib.h"

char* get_float_sub(int pr);

char* get_simple_float(void){
	int i;
	float f;
	char* err;
	char b1,b2,b3;
	next_position();
	b1=g_source[g_srcpos];
	if (b1=='(') {
		// (...)
		// Parenthesis
		g_srcpos++;
		next_position();
		err=get_float_sub(priority(OP_VOID));
		if (err) return err;
		next_position();
		if (g_source[g_srcpos]!=')') return ERR_SYNTAX;
		g_srcpos++;
	} else if (b1=='-') {
		// Unary '-' operator
		// Note that unary operators ( + and - ) have higher priority than the other operators
		g_srcpos++;
		err=get_simple_float();
		if (err) return err;
		check_obj_space(1);
		g_object[g_objpos++]=0x34040000;                     // ori a0,zero,0
		call_lib_code(LIB_FLOAT | OP_SUB);
	} else {
		// Main routine of getting float value here
		if (b1=='+') g_srcpos++; // Ignore unary '+' operator
		next_position();
		b1=g_source[g_srcpos];
		b2=g_source[g_srcpos+1];
		b3=g_source[g_srcpos+2];
		if ('0'<=b1 && b1<='9') {
			f=strtof((const char*)&g_source[g_srcpos],&err);
			if (&g_source[g_srcpos]==err) return ERR_SYNTAX;
			g_srcpos=err-g_source;
			i=((int*)(&f))[0];
			if (i&0xFFFF0000) {
				// 32 bit
				check_obj_space(2);
				g_object[g_objpos++]=0x3C020000|((i>>16)&0x0000FFFF); // lui   v0,xxxx
				g_object[g_objpos++]=0x34420000|(i&0x0000FFFF);       // ori v0,v0,xxxx
			} else {
				// 16 bit
				check_obj_space(1);
				g_object[g_objpos++]=0x34020000|(i&0x0000FFFF); // ori v0,zero,xxxx
			}
		} else {
			i=get_var_number();
			if (i<0) {
				// Must be a function.
				return float_function();
			}
			if (g_source[g_srcpos]=='.') {
				// This is an object field or method to return string
				check_obj_space(1);
				g_object[g_objpos++]=0x8FC20000|(i*4); // lw v0,xx(s8)
				g_srcpos++;
				return float_obj_field();
			} else if (g_source[g_srcpos]=='(') {
				// An array element contains pointer to an object.
				g_srcpos++;
				err=get_dim_value(i);
				if (err) return err;
				if (g_source[g_srcpos]!='.') return ERR_SYNTAX;
				g_srcpos++;
				return float_obj_field();
			}
			if (g_source[g_srcpos]!='#') return ERR_SYNTAX;
			g_srcpos++;
			if (g_source[g_srcpos]=='(') {
				// Dimension
				g_srcpos++;
				return get_dim_value(i);
			}
			// Simple value
			check_obj_space(1);
			g_object[g_objpos++]=0x8FC20000|(i*4); // lw v0,xx(s8)
		}
	}
	// No error 
	return 0;
}

char* get_float_sub(int pr){
	char* err;
	enum operator op;
	char b1,b2,b3;
	int prevpos;
	// Get a value in $v0.
	err=get_simple_float();
	if (err) return err;
	while(1){
		// Get the operator in op. If not valid operator, simply return without error.
		prevpos=g_srcpos;
		err=get_floatOperator();
		if (err) return 0;
		op=g_last_op;
		// Compair current and previous operators.
		// If the previous operator has higher priolity, return.
		if (pr>=priority(op)) {
			g_srcpos=prevpos;
			return 0;
		}
		// Store $v0 in stack
		g_sdepth+=4;
		if (g_maxsdepth<g_sdepth) g_maxsdepth=g_sdepth;
		check_obj_space(1);
		g_object[g_objpos++]=0xAFA20000|g_sdepth; // sw v0,xx(sp)
		// Get next value.
		err=get_float_sub(priority(op));
		if (err) return err;
		// Get value from stack to $a0.
		check_obj_space(1);
		g_object[g_objpos++]=0x8FA40000|g_sdepth; // lw a0,xx(sp)
		g_sdepth-=4;
		// Calculation. Result will be in $v0.
		err=calculation_float(op);
		if (err) return err;
	}
}

char* get_float(){
	// Note that this can be called recursively.
	// Value may contain function with a parameter of another value.
	char* err;
	int prevpos;
	if (g_sdepth==0) {
		// Initialize stack handler
		g_maxsdepth=0;
		prevpos=g_objpos;
		// Stack decrement command will be filled later
		check_obj_space(1);
		g_object[g_objpos++]=0x00000000; // nop (will be replaced by "addiu sp,sp,-xx")
	}
	err=get_float_sub(priority(OP_VOID));
	if (err) return err;
	if (g_sdepth==0) {
		if (g_maxsdepth==0) {
			// Stack was not used.
			if (g_allow_shift_obj) {
				shift_obj(&g_object[prevpos+1],&g_object[prevpos],g_objpos-prevpos-1);
				g_objpos--;
			}
		} else {
			// Stack was used.
			check_obj_space(1);
			g_object[prevpos]=0x27BD0000 | (0-g_maxsdepth) & 0x0000FFFF; // addiu sp,sp,-xx
			g_object[g_objpos++]=0x27BD0000 | g_maxsdepth & 0x0000FFFF;  // addiu sp,sp,xx
		}
	}
	g_lastvar=VAR_FLOAT;
	return 0;
}