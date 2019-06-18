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
  Public functions:
    char* get_string(void);
	char* simple_string(void);
*/

#include "api.h"
#include "compiler.h"

char* simple_string(void){
	char* err;
	unsigned char b1,b2;
	int i,j;
	next_position();
	b1=g_source[g_srcpos];
	b2=g_source[g_srcpos+1];
	if (b1=='"') {
		// Constant string
		// Count character number (+1 for \0)
		// This may be more than actual character number due to escape sequence.
		i=0;
		while(1){
			i++;
			b1=g_source[g_srcpos+i];
			if (b1=='"') {
				break;
			} else if (b1=='\\') {
				i++;
			} else if (b1<0x20) {
				// This is an error, that will be detected later again.
				break;
			}
		};
		g_srcpos++;
		// Determine required word number
		i=(i+3)/4;
		// Determine address containing the string
		j=(int)(&(g_object[g_objpos+3]));
		// Note that using "bgezal zero," must be used to skip some region.
		// This is to find embed string in the code.
		check_obj_space(2+i);
		g_object[g_objpos++]=0x04110000|((i+1)&0x0000FFFF);   // bgezal zero,xxxx
		g_object[g_objpos++]=0x03E01021;                      // addu   v0,ra,zero
		for(j=0;(b1=g_source[g_srcpos++])!='"';j++) {
			if (b1=='\\') {
				b1=g_source[g_srcpos++];
				if (b1=='x' || b1=='X') {
					// Hexadecimal
					b1=g_source[g_srcpos++];
					if ('0'<=b1 && b1<='9') b1=b1-'0';
					else if ('A'<=b1 && b1<='F') b1=b1-'A'+10;
					else if ('a'<=b1 && b1<='f') b1=b1-'a'+10;
					else return ERR_SYNTAX;
					b2=g_source[g_srcpos++];
					if ('0'<=b2 && b2<='9') b2=b2-'0';
					else if ('A'<=b2 && b2<='F') b2=b2-'A'+10;
					else if ('a'<=b2 && b2<='f') b2=b2-'a'+10;
					else return ERR_SYNTAX;
					b1=(b1<<4)|b2;
				} else if (b1<0x20) {
					return ERR_SYNTAX;
				} else {
					// \\ or \"
				}
			} else if (b1<0x20) {
				return ERR_SYNTAX;
			}
			((char*)(&g_object[g_objpos]))[j]=b1;
		}
		((char*)(&g_object[g_objpos]))[j]=0x00;
		g_objpos+=i;
	} else if ('A'<=b1 && b1<='Z') {
		i=get_var_number();
		if (i<0) {
			// Function
			// String would be pointed by $v0
			// Otherwise, it will be assinged in xxx_function() function.
			err=str_function();
			if (err) return err;
			// Temp area would be used when executing.
			g_temp_area_used=1;
			return 0;
		}
		if (g_source[g_srcpos]=='.') {
			// This is an object field or method to return string
			check_obj_space(1);
			g_object[g_objpos++]=0x8FC20000|(i*4); // lw v0,xx(s8)
			g_srcpos++;
			return string_obj_field();
		} else if (g_source[g_srcpos]=='(') {
			// An array element contains pointer to an object.
			g_srcpos++;
			err=get_dim_value(i);
			if (err) return err;
			if (g_source[g_srcpos]!='.') return ERR_SYNTAX;
			g_srcpos++;
			return string_obj_field();
		}
		if (g_source[g_srcpos]!='$') return ERR_SYNTAX;
		g_srcpos++;
		// String variable
		next_position();
		if (g_source[g_srcpos]=='(') {
			// A part of string
			g_srcpos++;
			err=get_value();
			if (err) return err;
			if (g_source[g_srcpos]==')') {
				g_srcpos++;
				// Put -1 to $a0
				check_obj_space(1);
				g_object[g_objpos++]=0x2404FFFF;          // addiu a0,zero,-1
			} else if (g_source[g_srcpos]==',') {
				g_srcpos++;
				// Store $v0 in stack
				g_sdepth+=4;
				if (g_maxsdepth<g_sdepth) g_maxsdepth=g_sdepth;
				check_obj_space(1);
				g_object[g_objpos++]=0xAFA20000|g_sdepth; // sw v0,xx(sp)
				// Get next value
				err=get_value();
				if (err) return err;
				// Copy $v0 to $a0 and get value from stack to $a0.
				check_obj_space(2);
				g_object[g_objpos++]=0x00402021;          // addu a0,v0,zero
				g_object[g_objpos++]=0x8FA20000|g_sdepth; // lw v0,xx(sp)
				g_sdepth-=4;
				if (g_source[g_srcpos]!=')') return ERR_SYNTAX;
				g_srcpos++;
				// Temp area will be used when executing.
				g_temp_area_used=1;
			} else {
				return ERR_SYNTAX;
			}
			// $a1 is var number, $v0 is position, $a0 is length
			check_obj_space(1);
			g_object[g_objpos++]=0x24050000|(i);          // addiu a1,zero,xx
			call_lib_code(LIB_MIDSTR);
		} else {
			// Simple string
			check_obj_space(1);
			g_object[g_objpos++]=0x8FC20000|(i*4);        // lw v0,xx(s8)
			// String is pointed by $v0
		}
	} else {
		return ERR_SYNTAX;
	}
	return 0;
}

char* get_string_sub(){
	char* err;
	char b1;
	// Obtain initial string
	err=simple_string();
	if (err) return err;
	// Check if connection operator exists
	next_position();
	b1=g_source[g_srcpos];
	if (b1!='+' && b1!='&') return 0; // Exit if connection operator does not exist.
	g_srcpos++;
	// Connection required.
	// Prepare one level of stack for handling
	g_sdepth+=4;
	if (g_maxsdepth<g_sdepth) g_maxsdepth=g_sdepth;
	while(1) {
		// Store current pointer in stack
		check_obj_space(1);
		g_object[g_objpos++]=0xAFA20000|g_sdepth; // sw v0,xx(sp)
		// Obtain next string (pointer will be in $v0)
		err=simple_string();
		if (err) return err;
		// Restore previous pointer from stack in $a0 and copy $v0 to $a1
		// Call library
		check_obj_space(2);
		g_object[g_objpos++]=0x8FA40000|g_sdepth;             // lw    a0,xx(sp)
		call_lib_code(LIB_CONNECT_STRING);
		// Temp area will be used when executing.
		g_temp_area_used=1;
		// Check if further connection operator exists
		next_position();
		b1=g_source[g_srcpos];
		if (b1!='+' && b1!='&') break;
		g_srcpos++;
	}
	g_sdepth-=4;
	return 0;
}

char* get_string(){
	// This is only the public function.
	// Note that this can be called recursively.
	// String may contain a function with a parameter of value
	// that is a function with a parameter of string.
	// Result will be in $v0 as a pointer.
	char* err;
	char b1;
	int i,prevpos;
	if (g_sdepth==0) {
		// Initialize stack handler
		g_maxsdepth=0;
		prevpos=g_objpos;
		// Stack decrement command will be filled later
		check_obj_space(1);
		g_object[g_objpos++]=0x00000000; // nop (will be replaced by "addiu sp,sp,-xx")
	}
	err=get_string_sub();
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
	g_lastvar=VAR_STRING;
	return 0;
}

