/*
   This file is provided under the LGPL license ver 2.1.
   Written by Katsumi.
   http://hp.vector.co.jp/authors/VA016157/
   kmorimatsu@users.sourceforge.jp
*/

/*
	This file is shared by Megalopa and Zoea
*/

/*
  Public function is only get_value().
*/

#include "compiler.h"

char* get_value();
char* get_value_sub(int pr);

char* get_dim_value(int i){
	char* err;
	err=get_value_sub(priority(OP_VOID));
	if (err) return err;
	check_obj_space(4);
	g_object[g_objpos++]=0x00021080;              // sll v0,v0,0x2
	g_object[g_objpos++]=0x8FC30000|(i*4);        // lw v1,xx(s8)
	g_object[g_objpos++]=0x00621821;              // addu v1,v1,v0
	g_object[g_objpos++]=0x8C620000;              // lw v0,0(v1)
	next_position();
	if (g_source[g_srcpos]==','){
		// 2D, 3D or more
		// Use a stack
		g_sdepth+=4;
		if (g_maxsdepth<g_sdepth) g_maxsdepth=g_sdepth;
		check_obj_space(1);
		do {
			g_srcpos++;
			g_object[g_objpos++]=0xAFA20000|g_sdepth;      // sw v0,xx(sp)
			err=get_value_sub(priority(OP_VOID));
			if (err) return err;
			check_obj_space(5);
			g_object[g_objpos++]=0x00021080;               // sll v0,v0,0x2
			g_object[g_objpos++]=0x8FA30000|g_sdepth;      // lw v1,xx(sp)
			g_object[g_objpos++]=0x00621821;               // addu v1,v1,v0
			g_object[g_objpos++]=0x8C620000;               // lw v0,0(v1)
		} while (g_source[g_srcpos]==',');
		g_sdepth-=4;
	}
	if (g_source[g_srcpos]!=')') return ERR_SYNTAX;
	g_srcpos++;
	return 0;
}

char* get_simple_value(void){
	int i;
	char* err;
	char b1,b2;
	next_position();
	b1=g_source[g_srcpos];
	if (b1=='(') {
		// (...)
		// Parenthesis
		g_srcpos++;
		next_position();
		err=get_value_sub(priority(OP_VOID));
		if (err) return err;
		next_position();
		if (g_source[g_srcpos]!=')') return ERR_SYNTAX;
		g_srcpos++;
	} else if (b1=='-') {
		// Unary '-' operator
		// Note that unary operators ( + and - ) have higher priority than the other operators
		g_srcpos++;
		err=get_simple_value();
		if (err) return err;
		check_obj_space(1);
		g_object[g_objpos++]=0x00021023; // subu v0,zero,v0
		g_intconst=-g_intconst;
	} else if (b1=='&') {
		// '&' operator
		g_srcpos++;
		i=get_var_number();
		if (i<0) return ERR_SYNTAX;
		check_obj_space(1);
		g_object[g_objpos++]=0x27C20000|(i*4); // addiu       v0,s8,xxxx
	} else {
		// Main routine of getting value here
		if (b1=='+') g_srcpos++; // Ignore unary '+' operator
		next_position();
		b1=g_source[g_srcpos];
		b2=g_source[g_srcpos+1];
		if (b1=='0' && b2=='X' || b1=='$') {
			// Starts with '0x' or '$'
			// Hex number
			g_srcpos++;
			if (b1=='0') g_srcpos++;
			i=0;
			while(b1=g_source[g_srcpos]) {
				if ('0'<=b1 && b1<='9') {
					i*=16;
					i+=b1-'0';
				} else if ('A'<=b1 && b1<='F') {
					i*=16;
					i+=b1-'A'+0x0A;
				} else if (b1==' ') { // Skip ' '
					// Avoid "ELSE" statement etc
					if ('F'<g_source[g_srcpos+2]) break;
				} else {
					break;
				}
				g_srcpos++;
			}
			g_intconst=i;
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
		} else if ('0'<=b1 && b1<='9') {
			// Starts with 0-9
			// Decimal number
			i=0;
			while(b1=g_source[g_srcpos]) {
				if ('0'<=b1 && b1<='9') {
					i*=10;
					i+=b1-'0';
				} else if (b1!=' ') { // Skip ' '
					break;
				}
				g_srcpos++;
			}
			// The next character should not be '.' or 'E'.
			// Or, it must be recognized as a float value.
			if (b1=='.') return ERR_SYNTAX;
			if (b1=='E') {
				b2=g_source[g_srcpos+1];
				if (b2==' ' || '0'<=b2 && b2<='9') return ERR_SYNTAX;
			}
			g_intconst=i;
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
			g_valueisconst=0;
			i=get_var_number();
			if (i<0) {
				// Must be a function.
				return function();
			}
			if (g_source[g_srcpos]=='(') {
				// Dimension
				g_srcpos++;
				err=get_dim_value(i);
				if (err) return err;
			} else {
				// Simple value
				check_obj_space(1);
				g_object[g_objpos++]=0x8FC20000|(i*4); // lw v0,xx(s8)
			}
			// Check if this is an object
			if (g_source[g_srcpos]=='.') {
				// This is an object. See the filed of it.
				g_srcpos++;
				return integer_obj_field();
			}
		}
	}
	// No error 
	return 0;
}

char* get_value_sub(int pr){
	char* err;
	enum operator op;
	char b1,b2,b3;
	int prevpos;
	// Get a value in $v0.
	err=get_simple_value();
	if (err) return err;
	while(1){
		// Get the operator in op. If not valid operator, simply return without error.
		prevpos=g_srcpos;
		err=get_operator();
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
		err=get_value_sub(priority(op));
		if (err) return err;
		// Get value from stack to $v1.
		check_obj_space(1);
		g_object[g_objpos++]=0x8FA30000|g_sdepth; // lw v1,xx(sp)
		g_sdepth-=4;
		// Calculation. Result will be in $v0.
		err=calculation(op);
		if (err) return err;
	}
}

char* get_value(){
	// This is only the public function.
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
	err=get_value_sub(priority(OP_VOID));
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
	g_lastvar=VAR_INTEGER;
	return 0;
}

char* get_floatOrValue(){
	char* err;
	char b;
	int opos,spos,sdpt;
	sdpt=g_sdepth;
	opos=g_objpos;
	spos=g_srcpos;
	// First try integer.
	// Integer value has the higher priolity than float value.
	err=get_value();
	b=g_source[g_srcpos];
	if (err || b=='#') {
		// Value is not integer. Let's try float.
		g_sdepth=sdpt;
		g_objpos=opos;
		g_srcpos=spos;
		return get_float();
	} else {
		// Value was recognized as an integer.
		return 0;
	}
}

char* get_stringFloatOrValue(){
	char* err;
	int opos,spos;
	opos=g_objpos;
	spos=g_srcpos;
	// First try string, float, then integer.
	err=get_string();
	if (err) {
		g_objpos=opos;
		g_srcpos=spos;
		return get_floatOrValue();
	} else {
		return 0;
	}
}

