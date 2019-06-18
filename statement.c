/*
   This file is provided under the LGPL license ver 2.1.
   Written by Katsumi.
   http://hp.vector.co.jp/authors/VA016157/
   kmorimatsu@users.sourceforge.jp
*/

/*
	This file is shared by Megalopa and Zoea
*/

#include "api.h"
#include "compiler.h"

char* rem_statement(){
	if (g_source[g_srcpos-4]<0x20) {
		// This line contains only "REM" statement
		// Delete $s6-setting command if exists.
		if ((g_object[g_objpos-1]&0xffff0000)==0x34160000) g_objpos--;
	}
	while(0xE0 & g_source[g_srcpos]){
		g_srcpos++;
	}
	return 0;
}

char* sound_statement(){
	char *err;
	err=get_label();
	if (err) return err;
	if (g_label) {
		// Label/number is constant.
		// Linker will change following codes later.
		// Note that 0x0814xxxx and 0x0815xxxx are specific codes for these.
		check_obj_space(2);
		g_object[g_objpos++]=0x08140000|((g_label>>16)&0x0000FFFF); // lui   v0,xxxx
		g_object[g_objpos++]=0x08150000|(g_label&0x0000FFFF);       // ori v0,v0,xxxx
	} else {
		// Label/number will be dynamically set when executing code.
		err=get_value();
		if (err) return err;
		call_lib_code(LIB_LABEL);
	}
	// 2nd param is optional
	next_position();
	if (g_source[g_srcpos]==',') {
		g_srcpos++;
		check_obj_space(2);
		g_object[g_objpos++]=0x27BDFFFC; // addiu       sp,sp,-4
		g_object[g_objpos++]=0xAFA20004; // sw          v0,4(sp)
		err=get_value();
		if (err) return err;
		check_obj_space(3);
		g_object[g_objpos++]=0x00402021; // addu        a0,v0,zero
		g_object[g_objpos++]=0x8FA20004; // lw          v0,4(sp)
		g_object[g_objpos++]=0x27BD0004; // addiu       sp,sp,4
	} else {
		// Set 3 if omitted
		check_obj_space(1);
		g_object[g_objpos++]=0x24040003; // addiu       a0,zero,xx
	}
	call_lib_code(LIB_SOUND);
	return 0;
}
char* music_statement(){
	char *err;
	err=get_string();
	if (err) return err;
	// 2nd param is optional
	next_position();
	if (g_source[g_srcpos]==',') {
		g_srcpos++;
		check_obj_space(2);
		g_object[g_objpos++]=0x27BDFFFC; // addiu       sp,sp,-4
		g_object[g_objpos++]=0xAFA20004; // sw          v0,4(sp)
		err=get_value();
		if (err) return err;
		check_obj_space(3);
		g_object[g_objpos++]=0x00402021; // addu        a0,v0,zero
		g_object[g_objpos++]=0x8FA20004; // lw          v0,4(sp)
		g_object[g_objpos++]=0x27BD0004; // addiu       sp,sp,4
	} else {
		// Set 3 if omitted
		check_obj_space(1);
		g_object[g_objpos++]=0x24040003; // addiu       a0,zero,xx
	}
	call_lib_code(LIB_MUSIC);
	return 0;
}

char* exec_statement(){
	char *err;
	char b1;
	int i,prevpos;
	b1=g_source[g_srcpos];
	while('0'<=b1 && b1<='9' || b1=='-' || b1=='$'){
		prevpos=g_objpos;
		g_valueisconst=1;
		err=get_simple_value();
		if (!g_valueisconst) err=ERR_SYNTAX;
		if (err) return err;
		check_obj_space(1);
		g_objpos=prevpos;
		g_object[g_objpos++]=g_intconst;
		next_position();
		b1=g_source[g_srcpos];
		if (b1!=',') break;
		g_srcpos++;
		next_position();
		b1=g_source[g_srcpos];
		if (b1==0x0d || b1==0x0a) {
			// Multiline DATA/EXEC statement
			g_line++;
			g_fileline++;
			if (b1==0x0D && g_source[g_srcpos+1]==0x0A) g_srcpos++;
			g_srcpos++;
			// Maintain at least 256 characters in cache.
			if (256<=g_srcpos) read_file(256);
			next_position();
			b1=g_source[g_srcpos];
		}			
	}
	return 0;
}

char* cdata_statement(){
	// 0x00000020, 0x00000021, 0x00000022, and 0x00000023 (add/addu/sub/subu zero,zero,zero) 
	// are the sign of data region
	int beginpos,prevpos;
	char* err;
	char b1;
	char* cpy;
	int shift=0;
	int i=0;
	// This statement is not valid in class file.
	if (g_compiling_class) return ERR_INVALID_CLASS;
	beginpos=g_objpos;
	check_obj_space(2);
	g_object[g_objpos++]=0x04110000; // bgezal      zero,xxxx
	g_object[g_objpos++]=0x00000020; // add         zero,zero,zero
	next_position();
	b1=g_source[g_srcpos];
	while('0'<=b1 && b1<='9' || b1=='-' || b1=='$'){
		prevpos=g_objpos;
		g_valueisconst=1;
		err=get_simple_value();
		if (!g_valueisconst) err=ERR_SYNTAX;
		if (g_intconst<0x00 || 0xff<g_intconst) err=ERR_SYNTAX;
		if (err) return err;
		g_objpos=prevpos;
		i|=g_intconst<<shift;
		shift+=8;
		if (32<=shift) {
			check_obj_space(1);
			g_object[g_objpos++]=i;
			shift=0;
			i=0;
		}
		next_position();
		b1=g_source[g_srcpos];
		if (b1!=',') break;
		g_srcpos++;
		next_position();
		b1=g_source[g_srcpos];
		if (b1==0x0d || b1==0x0a) {
			// Multiline CDATA statement
			g_line++;
			g_fileline++;
			if (b1==0x0D && g_source[g_srcpos+1]==0x0A) g_srcpos++;
			g_srcpos++;
			// Maintain at least 256 characters in cache.
			if (256<=g_srcpos) read_file(256);
			next_position();
			b1=g_source[g_srcpos];
		}			
	}
	// Write the last 1-3 bytes and shift data if total # is not multipes of 4.
	if (0<shift) {
		// Convert shift value from bit-shift to data byte-shift.
		shift=4-shift/8;
		check_obj_space(1);
		g_object[g_objpos++]=i;
		for(cpy=(char*)&g_object[g_objpos]-1;(char*)&g_object[beginpos+2]<cpy;cpy--){
			cpy[0]=cpy[0-shift];
		}
	}
	// Determine the size of data
	i=g_objpos-beginpos-1;
	g_object[beginpos]  =0x04110000|i;     // bgezal zero,xxxx
	g_object[beginpos+1]=0x00000020|shift; // add         zero,zero,zero
	return 0;
}

char* data_statement(){
	// 0x00000020, 0x00000021, 0x00000022, and 0x00000023 (add/addu/sub/subu zero,zero,zero) 
	// are the sign of data region
	int i,prevpos;
	char* err;
	// This statement is not valid in class file.
	if (g_compiling_class) return ERR_INVALID_CLASS;
	while(1){
		prevpos=g_objpos;
		check_obj_space(2);
		g_object[g_objpos++]=0x04110000; // bgezal      zero,xxxx
		g_object[g_objpos++]=0x00000020; // add         zero,zero,zero
		next_position();
		if (g_source[g_srcpos]=='"') {
			// Constant string
			// Store pointer to string. This is 3 words bellow of current position
			g_object[g_objpos]=(int)(&g_object[g_objpos+3]);
			g_objpos++;
			g_object[prevpos]=0x04110002; // bgezal zero,xxxx
			err=simple_string();
			if (err) return err;
			next_position();
			if (g_source[g_srcpos]==',') {
				g_srcpos++;
				continue;
			}
			return 0;
		}
		err=exec_statement();
		if (err) return err;
		// Determine the size of data
		i=g_objpos-prevpos-1;
		g_object[prevpos]=0x04110000|i; // bgezal zero,xxxx
		if (g_source[g_srcpos]=='"') {
			// Constant string
			continue;
		}
		return 0;
	}
}

char* clear_statement(){
	call_lib_code(LIB_CLEAR);
	return 0;
}

char* poke_statement_sub(int bits){
	char* err;
	err=get_value();
	if (err) return err;
	if (g_source[g_srcpos]!=',') return ERR_SYNTAX;
	g_srcpos++;
	check_obj_space(2);
	g_object[g_objpos++]=0x27BDFFFC; // addiu       sp,sp,-4
	g_object[g_objpos++]=0xAFA20004; // sw          v0,4(sp)
	err=get_value();
	if (err) return err;
	check_obj_space(3);
	g_object[g_objpos++]=0x8FA30004; // lw          v1,4(sp)
	g_object[g_objpos++]=0x27BD0004; // addiu       sp,sp,4
	switch(bits){
		case 32:
			g_object[g_objpos++]=0xAC620000; // sw          v0,0(v1)
			break;
		case 16:
			g_object[g_objpos++]=0xA4620000; // sh          v0,0(v1)
			break;
		case 8:
		default:
			g_object[g_objpos++]=0xA0620000; // sb          v0,0(v1)
			break;
	}
	return 0;
}

char* dim_statement(){
	char* err;
	char b1;
	int i;
	int spos;
	int stack;
	while(1){
		stack=0;
		next_position();
		i=get_var_number();
		if (i<0) return ERR_SYNTAX;
		if (g_source[g_srcpos]=='#') g_srcpos++;
		next_position();
		if (g_source[g_srcpos]!='(') return ERR_SYNTAX;
		check_obj_space(1);
		spos=g_objpos++;           // addiu       sp,sp,xxxx
		do {
			g_srcpos++;
			err=get_value();
			if (err) return err;
			stack+=4;
			check_obj_space(1);
			g_object[g_objpos++]=0xAFA20000|stack; // sw          v0,8(sp)
		} while (g_source[g_srcpos]==',');
		if (g_source[g_srcpos]!=')') return ERR_SYNTAX;
		g_srcpos++;
		check_obj_space(3);
		g_object[g_objpos++]=0x24040000|(i);       // addiu       a0,zero,xx
		g_object[g_objpos++]=0x24050000|(stack/4); // addiu       a1,zero,xxxx
		g_object[g_objpos++]=0x03A01025;           // or          v0,sp,zero
		call_lib_code(LIB_DIM);
		// Stack -/+
		check_obj_space(1);
		g_object[g_objpos++]=0x27BD0000|stack;     // addiu       sp,sp,xxxx
		stack=(0-stack)&0x0000FFFF;
		g_object[spos]=0x27BD0000|stack;           // addiu       sp,sp,xxxx
		next_position();
		if (g_source[g_srcpos]!=',') break;
		g_srcpos++;
	}
	return 0;
}

char* label_statement(){
	char* err;
	char b1;
	b1=g_source[g_srcpos];
	if ('0'<=b1 && b1<='9') return ERR_SYNTAX; // Number is not allowed here.
	err=get_label();
	if (err) return err;
	// Check existing label with the same name here.
	if (search_label(g_label)) {
		// Error: duplicate labels
		printstr("Label ");
		printstr(resolve_label(g_label));
		return ERR_MULTIPLE_LABEL;
	}
	check_obj_space(2);
	g_object[g_objpos++]=0x3C160000|((g_label>>16)&0x0000FFFF); //lui s6,yyyy;
	g_object[g_objpos++]=0x36D60000|(g_label&0x0000FFFF);       //ori s6,s6,zzzz;
	return 0;
}

char* restore_statement(){
	char* err;
	// This statement is not valid in class file.
	if (g_compiling_class) return ERR_INVALID_CLASS;
	err=get_label();
	if (err) return err;
	if (g_label) {
		// Constant label/number
		// Use 32 bit mode also for values<65536
		// This code will be replaced to code for v0 for pointer in linker.
		check_obj_space(2);
		g_object[g_objpos++]=0x3C020000|(g_label>>16);        // lui         v0,xxxx
		g_object[g_objpos++]=0x34420000|(g_label&0x0000FFFF); // ori         v0,v0,xxxx
	} else {
		// Dynamic number
		err=get_value();
		if (err) return err;
	}
	call_lib_code(LIB_RESTORE);
	return 0;
}

char* gosub_statement_sub(){
	char* err;
	err=get_label();
	if (err) return err;
	if (g_label) {
		// Label/number is constant.
		// Linker will change following codes later.
		// Note that 0x0812xxxx and 0x0813xxxx are specific codes for these.
		check_obj_space(7);
		g_object[g_objpos++]=0x27BDFFFC;                            // addiu       sp,sp,-4
		g_object[g_objpos++]=0x04130003;                            // bgezall     zero,label1
		g_object[g_objpos++]=0xAEBD0000|ARGS_S5_SP;                 // sw          sp,-12(s5)
		g_object[g_objpos++]=0x10000003;                            // beq         zero,zero,label2
		g_object[g_objpos++]=0x08120000|((g_label>>16)&0x0000FFFF); // nop         
		                                                            // label1:
		g_object[g_objpos++]=0x08130000|(g_label&0x0000FFFF);       // j           xxxx
		g_object[g_objpos++]=0xAFBF0004;                            // sw          ra,4(sp)
		                                                            // label2:
	} else {
		// Label/number will be dynamically set when executing code.
		err=get_value();
		if (err) return err;
		call_lib_code(LIB_LABEL);
		check_obj_space(7);
		g_object[g_objpos++]=0x27BDFFFC;            // addiu       sp,sp,-4
		g_object[g_objpos++]=0x04130003;            // bgezall     zero,label1
		g_object[g_objpos++]=0xAEBD0000|ARGS_S5_SP; // sw          sp,-12(s5)
		g_object[g_objpos++]=0x10000003;            // beq         zero,zero,label2
		g_object[g_objpos++]=0x00000000;            // nop         
		                                            // label1:
		g_object[g_objpos++]=0x00400008;            // jr          v0
		g_object[g_objpos++]=0xAFBF0004;            // sw          ra,4(sp)
		                                            // label2:
	}
	return 0;
}

char* gosub_statement(){
	char* err;
	int opos,spos,stack;
	// Skip label first (see below)
	opos=g_objpos;
	spos=g_srcpos;
	err=gosub_statement_sub();
	if (err) return err;
	next_position();
	// Rewind object and construct argument-creating routine.
	g_objpos=opos;
	// Begin parameter(s) construction routine
	g_object[g_objpos++]=0x8EA20000|ARGS_S5_V0_OBJ;           // lw          v0,-8(s5)
	err=prepare_args_stack(',');
	if (err) return err;
	// Rewind source and construct GOSUB routine again.
	opos=spos;
	spos=g_srcpos;
	g_srcpos=opos;
	err=gosub_statement_sub();
	if (err) return err;
	// Remove stack
	err=remove_args_stack();
	if (err) return err;
	// All done, go back to right source position
	g_srcpos=spos;
	return 0;
}

char* return_statement(){
	char* err;
	char b1;
	next_position();
	b1=g_source[g_srcpos];
	if (0x20<b1 && b1!=':') {
		// There is a return value.
		err=get_stringFloatOrValue();
		if (err) return err;
	}
	check_obj_space(4);
	g_object[g_objpos++]=0x8EBD0000|ARGS_S5_SP; // lw          sp,-12(s5)
	g_object[g_objpos++]=0x8FA30004;            // lw          v1,4(sp)
	g_object[g_objpos++]=0x00600008;            // jr          v1
	g_object[g_objpos++]=0x27BD0004;            // addiu       sp,sp,4
	return 0;
}

char* goto_statement(){
	char* err;
	err=get_label();
	if (err) return err;
	if (g_label) {
		// Label/number is constant.
		// Linker will change following codes later.
		// Note that 0x0810xxxx and 0x0811xxxx are specific codes for these.
		check_obj_space(2);
		g_object[g_objpos++]=0x08100000|((g_label>>16)&0x0000FFFF); // j xxxx
		g_object[g_objpos++]=0x08110000|(g_label&0x0000FFFF);       // nop
	} else {
		// Label/number will be dynamically set when executing code.
		err=get_value();
		if (err) return err;
		call_lib_code(LIB_LABEL);
		check_obj_space(2);
		g_object[g_objpos++]=0x00400008; // jr          v0
		g_object[g_objpos++]=0x00000000; // nop
	}
	return 0;
}

char* if_statement(){
	char* err;
	int prevpos,bpos;
	// Get value.
	err=get_floatOrValue();
	if (err) return err;
	// Check "THEN"
	if (!nextCodeIs("THEN")) return ERR_SYNTAX;
	// Check if statement follows after THEN statement
	next_position();
	if (nextCodeIs("REM")) {
		// If REM statement follows, skip comment words.
		rem_statement();
	}
	if (g_source[g_srcpos]<0x20) {
		// End of line.
		// Use IF-THEN-ENDIF mode (multiple line mode)
		check_obj_space(3);
		g_object[g_objpos++]=0x30000000; // nop (see linker)
		g_object[g_objpos++]=0x10400000; // beq         v0,zero,xxxx
		g_object[g_objpos++]=0x30000000; // nop (see linker)
		return 0;
	}
	// One line mode
	// If $v0=0 then skip.
	bpos=g_objpos;
	check_obj_space(2);
	g_object[g_objpos++]=0x10400000; // beq         v0,zero,xxxx
	g_object[g_objpos++]=0x00000000; // nop
	prevpos=g_srcpos;
	if (statement()) {
		// May be label
		g_srcpos=prevpos;
		err=goto_statement();
		if (err) return err;
	} else {
		// Must be statement(s)
		while(1) {
			if (g_source[g_srcpos]!=':') break;
			g_srcpos++;
			err=statement();
			if (err) return err;
		}
	}
	// Check if "ELSE" exists.
	if (!nextCodeIs("ELSE ")) {
		// "ELSE" not found. This is the end of "IF" statement.
		// Previous branch command must jump to this position.
		g_object[bpos]=0x10400000|(g_objpos-bpos-1); // beq         v0,zero,xxxx	
		return 0;
	}
	// Skip after ELSE if required.
	check_obj_space(2);
	g_object[g_objpos++]=0x10000000; // beq         zero,zero,xxxx
	g_object[g_objpos++]=0x00000000; // nop
	// Previous branch command must jump to this position.
	g_object[bpos]=0x10400000|(g_objpos-bpos-1); // beq         v0,zero,xxxx	
	bpos=g_objpos-2;
	// Next statement is either label or general statement
	prevpos=g_srcpos;
	if (statement()) {
		// May be label
		g_srcpos=prevpos;
		err=goto_statement();
		if (err) return err;
	} else {
		// Must be statement(s)
		while(1) {
			if (g_source[g_srcpos]!=':') break;
			g_srcpos++;
			err=statement();
			if (err) return err;
		}
	}
	// Previous branch command must jump to this position.
	g_object[bpos]=0x10000000|(g_objpos-bpos-1); // beq         zero,zero,xxxx	
	return 0;
}

char* elseif_statement(void){
	// Multiple line mode
	char* err;
	g_object[g_objpos++]=0x08160100; // breakif (see linker)
	g_object[g_objpos++]=0x30008000; // nop (see linker)
	// Get value.
	err=get_floatOrValue();
	if (err) return err;
	// Check "THEN"
	if (!nextCodeIs("THEN")) return ERR_SYNTAX;
	// Check if statement follows after THEN statement
	if (nextCodeIs("REM")) {
		// If REM statement follows, skip comment words.
		rem_statement();
	}
	if (0x20<=g_source[g_srcpos]) return ERR_SYNTAX;
	// Statement didn't follow after THEM statement (that is correct).
	g_object[g_objpos++]=0x10400000; // beq         v0,zero,xxxx
	g_object[g_objpos++]=0x30000000; // nop (see linker)
	return 0;
	
}

char* else_statement(void){
	// Multiple line mode
	g_object[g_objpos++]=0x08160100; // breakif (see linker)
	g_object[g_objpos++]=0x30008000; // nop (see linker)
	g_object[g_objpos++]=0x30000000; // nop (see linker)
	// Check if statement follows after THEN statement
	if (nextCodeIs("REM")) {
		// If REM statement follows, skip comment words.
		rem_statement();
	}
	if (0x20<=g_source[g_srcpos]) return ERR_SYNTAX;
	// Statement didn't follow after THEM statement (that is correct).
	return 0;
}

char* endif_statement(void){
	// Multiple line mode
	g_object[g_objpos++]=0x30008000; // nop (see linker)
	g_object[g_objpos++]=0x30008000; // nop (see linker)
	// Check if statement follows after THEN statement
	if (nextCodeIs("REM")) {
		// If REM statement follows, skip comment words.
		rem_statement();
	}
	if (0x20<=g_source[g_srcpos]) return ERR_SYNTAX;
	// Statement didn't follow after THEM statement (that is correct).
	return 0;
}

char* end_statement(void){
	int i;
	i=(int)&g_end_addr;
	i-=g_gp;
	check_obj_space(3);
	g_object[g_objpos++]=0x8F820000|(i&0x0000FFFF);       // lw v0,xxxx(gp)
	g_object[g_objpos++]=0x00400008;                      // jr v0
	g_object[g_objpos++]=0x00000000;                      // nop
	return 0;
}

char* let_dim_sub(int i){
	char* err;
	g_srcpos++;
	err=get_value();
	if (err) return err;
	check_obj_space(4);
	g_object[g_objpos++]=0x00021080;              // sll v0,v0,0x2
	g_object[g_objpos++]=0x8FC30000|(i*4);        // lw    v1,xx(s8)
	g_object[g_objpos++]=0x00621821;              // addu  v1,v1,v0
	g_object[g_objpos++]=0xAFA30004;              // sw    v1,4(sp)
	while(g_source[g_srcpos]==','){
		g_srcpos++;
		err=get_value();
		if (err) return err;
		check_obj_space(4);
		g_object[g_objpos++]=0x00021080;          // sll v0,v0,0x2
		g_object[g_objpos++]=0x8FA30004;          // lw    v1,4(sp)
		g_object[g_objpos++]=0x8C630000;          // lw    v1,0(v1)
		g_object[g_objpos++]=0x00621821;          // addu  v1,v1,v0
		g_object[g_objpos++]=0xAFA30004;          // sw    v1,4(sp)
	}
	if (g_source[g_srcpos]!=')') return ERR_SYNTAX;
	g_srcpos++;
	return 0;
};

char* let_statement(){
	char* err;
	char b2,b3;
	int i,spos,opos;
	next_position();
	i=get_var_number();
	if (i<0) return ERR_SYNTAX;
	b2=g_source[g_srcpos];
	b3=g_source[g_srcpos+1];
	if (b2=='#' && b3=='(') {
		// Float dimension
		g_srcpos++;
		check_obj_space(1);
		g_object[g_objpos++]=0x27BDFFFC;              // addiu sp,sp,-4
		let_dim_sub(i);
		next_position();
		if (g_source[g_srcpos]!='=') return ERR_SYNTAX;
		g_srcpos++;
		err=get_float();
		if (err) return err;
		check_obj_space(3);
		g_object[g_objpos++]=0x8FA30004;              // lw    v1,4(sp)
		g_object[g_objpos++]=0x27BD0004;              // addiu sp,sp,4
		g_object[g_objpos++]=0xAC620000;              // sw    v0,0(v1)
		return 0;
	} else if (b2=='#') {
		// Float A-Z
		g_srcpos++;
		next_position();
		if (g_source[g_srcpos]!='=') return ERR_SYNTAX;
		g_srcpos++;
		err=get_float();
		if (err) return err;
		check_obj_space(1);
		g_object[g_objpos++]=0xAFC20000|(i*4);        // sw v0,xxx(s8)
		return 0;
	} else 	if (b2=='$') {
		// String
		g_srcpos++;
		next_position();
		if (g_source[g_srcpos]!='=') return ERR_SYNTAX;
		g_srcpos++;
		err=get_string();
		if (err) return err;
		check_obj_space(1);
		g_object[g_objpos++]=0x24040000|(i);          //addiu       a0,zero,xx
		call_lib_code(LIB_LETSTR);
		return 0;
	} else if (b2=='(') {
		// Dimension
		check_obj_space(1);
		g_object[g_objpos++]=0x27BDFFFC;              // addiu sp,sp,-4
		let_dim_sub(i);
		if (g_source[g_srcpos]=='.') {
			// This is an object. Determine the filed of this object.
			// 4(sp) contains the address of dimension value
			// The dimension value is the pointer to object
			g_srcpos++;
			check_obj_space(3);
			g_object[g_objpos++]=0x8FA20004;          // lw    v0,4(sp)
			g_object[g_objpos++]=0x8C420000;          // lw    v0,0(v0)			g_object[g_objpos++]=0x27BD0004;          // addiu sp,sp,4
			return let_object_field();
		}
		next_position();
		if (g_source[g_srcpos]!='=') return ERR_SYNTAX;
		g_srcpos++;
		err=get_value();
		if (err) return err;
		check_obj_space(3);
		g_object[g_objpos++]=0x8FA30004;              // lw    v1,4(sp)
		g_object[g_objpos++]=0x27BD0004;              // addiu sp,sp,4
		g_object[g_objpos++]=0xAC620000;              // sw    v0,0(v1)
		return 0;
	} else if (b2=='.') {
		// Field of object
		g_srcpos++;
		check_obj_space(1);
		g_object[g_objpos++]=0x8FC20000|(i*4);        // lw    v0,xx(s8)
		return let_object_field();
	} else {
		// Integer A-Z
		next_position();
		if (g_source[g_srcpos]!='=') return ERR_SYNTAX;
		g_srcpos++;
		err=get_value();
		if (err) return err;
		check_obj_space(1);
		g_object[g_objpos++]=0xAFC20000|(i*4);        // sw v0,xxx(s8)
	}
	return 0;
}

char* print_statement_main(enum libs lib_printstr, enum libs lib_string){
	char* err;
	char b1;
	int i;
	int status=0;// 1:',' 2:';' 0:none
	while(1){
		next_position();
		if (endOfStatement()) break;
		if (!strncmp(g_source+g_srcpos,"ELSE "  ,5)) break;	
		err=get_stringFloatOrValue();
		if (err) return err;
		switch(g_lastvar){
			case VAR_INTEGER:
				// Use DEC$() function.
				call_lib_code(LIB_DEC);
				break;
			case VAR_FLOAT:
				// Use FLOAT$() function.
				check_obj_space(2);
				g_object[g_objpos++]=0x00022021; //addu        a0,zero,v0
				g_object[g_objpos++]=0x34020000; //ori         v0,zero,0x0000
				call_lib_code(LIB_SPRINTF);
				break;
			case VAR_STRING:
			default:
				break;
		}
		// Call printstr() function
		// First argument is the pointer to string
		call_lib_code(lib_printstr);
		next_position();
		b1=g_source[g_srcpos];
		if (b1==',') {
			status=1;
			g_srcpos++;
			// Call lib_string() function for comma (,)
			check_obj_space(1);
			g_object[g_objpos++]=0x34020001;                      // ori   v0,zero,1
			call_lib_code(lib_string);
		} else if (b1==';') {
			status=2;
			g_srcpos++;
		} else {
			status=0;
		}
	}
	if (status==0) {
		// Call lib_string() function for CR (\n)
		check_obj_space(1);
		g_object[g_objpos++]=0x34020000;                      // ori   v0,zero,0
		call_lib_code(lib_string);
	}
	return 0;
}

char* break_statement(){
	check_obj_space(2);
	g_object[g_objpos++]=0x08160000; // j           xxxx     (See link() function)
	g_object[g_objpos++]=0x00000000; // nop
	return 0;
}

char* continue_statement(){
	check_obj_space(2);
	g_object[g_objpos++]=0x08160008; // j           xxxx     (See link() function)
	g_object[g_objpos++]=0x00000000; // nop
	return 0;
}

char* for_statement(){
	char* err;
//	char b1;
	int i;
	int prepos=g_srcpos;
	// Initialization of variable
//	next_position();
//	b1=g_source[g_srcpos];
	i=get_var_number();
//	if (b1<'A' || 'Z'<b1) return ERR_SYNTAX;
	if (i<0) return ERR_SYNTAX;
	g_srcpos=prepos;
	err=let_statement();
	if (err) return err;
	// Check if "TO" exists
	if (!nextCodeIs("TO ")) return ERR_SYNTAX;
	err=get_value();
	if (err) return err;
	// Usage of stack:
	//   12(sp): "TO" value
	//    8(sp): "STEP" value
	//    4(sp): Address to return to in "NEXT" statement.
	// Store "TO" value in stack
	check_obj_space(2);
	g_object[g_objpos++]=0x0820FFF4; // addiu sp,sp,-12 (see linker)
	g_object[g_objpos++]=0xAFA2000C; // sw v0,12(sp)
	// Check if "STEP" exists
	g_valueisconst=1;
	if (nextCodeIs("STEP ")) {
		// "STEP" exists. Get value
		err=get_value();
		if (err) return err;
	} else {
		// "STEP" not exist. Use "1".	
		check_obj_space(1);
		g_object[g_objpos++]=0x24020001; // addiu v0,zero,1
		g_intconst=1;
	}
	check_obj_space(14);
	g_object[g_objpos++]=0xAFA20008;              // sw          v0,8(sp)       (STEP value)
	g_object[g_objpos++]=0x04130004;              // bgezall     zero,check
//	g_object[g_objpos++]=0x8FC40000|((b1-'A')*4); // lw          a0,xx(s8)      (current var value)
	g_object[g_objpos++]=0x8FC40000|(i*4);        // lw          a0,xx(s8)      (current var value)
	// After executing "NEXT" statement, process reaches following line.
	// Update variable value by adding STEP value
	// Note that STEP value is loaded onto $v0 in NEXT statement
//	g_object[g_objpos++]=0x8FC40000|((b1-'A')*4); // lw          a0,xx(s8)      (current var value)
	g_object[g_objpos++]=0x8FC40000|(i*4);        // lw          a0,xx(s8)      (current var value)
	g_object[g_objpos++]=0x00822021;              // addu        a0,a0,v0
//	g_object[g_objpos++]=0xAFC40000|((b1-'A')*4); // sw          a0,xx(s8)      (new var value)
	g_object[g_objpos++]=0xAFC40000|(i*4);        // sw          a0,xx(s8)      (new var value)
	// Value-checking routine and storing ra in stack
	                                              // check:
	g_object[g_objpos++]=0x8FA3000C;              // lw          v1,12(sp)      (TO value)
	g_object[g_objpos++]=0x00641823;              // subu        v1,v1,a0
	g_object[g_objpos++]=0x04420001;              // bltzl       v0,negative
	g_object[g_objpos++]=0x00031823;              // subu        v1,zero,v1
	                                              // negative:
	g_object[g_objpos++]=0x04610003;              // bgez        v1,continue
	g_object[g_objpos++]=0xAFBF0004;              // sw          ra,4(sp)
	break_statement();                            // (2 words)
	                                              // continue:
	return 0;
}

char* next_statement(){
	// Return to address stored in 4($sp)
	// while set $v0 to 8($sp) (see for_statement)
	// Following assembly must be 4 words.
	// If the number of words will be changed, link.c must be reviced for CONTINUE statement.
	check_obj_space(4);
	g_object[g_objpos++]=0x8FBF0004; // lw          ra,4(sp)
	g_object[g_objpos++]=0x03E00008; // jr          ra
	g_object[g_objpos++]=0x8FA20008; // lw          v0,8(sp)       (STEP value)
	g_object[g_objpos++]=0x0830000C; // addiu       sp,sp,12 (see linker)
	return 0;
}

char* do_statement(){
	char* err;
	// Usage of stack:
	//    4(sp): Address to return to in "DO" statement.
	check_obj_space(3);
	g_object[g_objpos++]=0x04130001;//   bgezall     zero,label1:
	g_object[g_objpos++]=0x0822FFFC;//   addiu       sp,sp,-4 (see linker)
	                                //   label1:
	g_object[g_objpos++]=0xAFBF0004;//   sw          ra,4(sp)
	if (nextCodeIs("WHILE ")) {
		// DO WHILE
		err=get_floatOrValue();
		if (err) return err;
		check_obj_space(2);
		g_object[g_objpos++]=0x14400003; // bne         v0,zero,labe2
		g_object[g_objpos++]=0x00000000; // nop
		return break_statement();        // (2 words)
			                             // label2:

	} else if (nextCodeIs("UNTIL ")) {
		// DO UNTIL
		err=get_floatOrValue();
		if (err) return err;
		check_obj_space(2);
		g_object[g_objpos++]=0x10400003; // beq         v0,zero,label2
		g_object[g_objpos++]=0x00000000; // nop
		return break_statement();        // (2 words)
			                             // label2:
	} else {
		// DO statement without WHILE/UNTIL
		return 0;
	}
}

char* loop_statement(){
	char* err;
	int opos;
	opos=g_objpos;
	if (nextCodeIs("WHILE ")) {
		// LOOP WHILE
		err=get_floatOrValue();
		if (err) return err;
		check_obj_space(1);
		g_object[g_objpos++]=0x10400003; // beq         v0,zero,label1
	} else if (nextCodeIs("UNTIL ")) {
		// LOOP UNTIL
		err=get_floatOrValue();
		if (err) return err;
		check_obj_space(1);
		g_object[g_objpos++]=0x14400003; // bne         v0,zero,label
	} else {
		// LOOP statement without WHILE/UNTIL
	}
	check_obj_space(4);
	g_object[g_objpos++]=0x8FBF0004;      // lw          ra,4(sp)
	g_object[g_objpos++]=0x03E00008;      // jr          ra
	opos=g_objpos+1-opos;
	g_object[g_objpos++]=0x3000F000|opos; // nop                 (See linker, used for CONTINUE statement)
	                                      // label1:
	g_object[g_objpos++]=0x08320004;      // addiu       sp,sp,4       (See link() function)
	return 0;
}

char* while_statement(){
	char* err;
	check_obj_space(3);
	g_object[g_objpos++]=0x04130001; // bgezall     zero,label1:
	g_object[g_objpos++]=0x0821FFFC; // addiu       sp,sp,-4 (see linker)
	                                 // label1:
	g_object[g_objpos++]=0xAFBF0004; // sw          ra,4(sp)
	err=get_floatOrValue();
	if (err) return err;
	check_obj_space(2);
	g_object[g_objpos++]=0x14400003; // bne         v0,zero,label2
	g_object[g_objpos++]=0x00000000; // nop
	return break_statement();        // (2 words)
		                             // label2:
}

char* wend_statement(){
	check_obj_space(4);
	g_object[g_objpos++]=0x8FBF0004; // lw          ra,4(sp)
	g_object[g_objpos++]=0x03E00008; // jr          ra
	g_object[g_objpos++]=0x3000F003; // nop                 (See linker, used for CONTINUE statement)
	                                 // label1:
	g_object[g_objpos++]=0x08310004; // addiu       sp,sp,4 (See link() function)
	return 0;
}

char* param4_statement(enum libs lib){
	// lib is either LIB_PALETTE or LIB_GPALETTE
	// PALETTE N,R,G,B
	char* err;
	// Get N
	err=get_value();
	if (err) return err;
	if (g_source[g_srcpos]!=',') return ERR_SYNTAX;
	g_srcpos++;
	check_obj_space(2);
	g_object[g_objpos++]=0x27BDFFF4; // addiu       sp,sp,-12
	g_object[g_objpos++]=0xAFA20004; // sw          v0,4(sp)
	// Get R
	err=get_value();
	if (err) return err;
	if (g_source[g_srcpos]!=',') return ERR_SYNTAX;
	g_srcpos++;
	check_obj_space(1);
	g_object[g_objpos++]=0xAFA20008; // sw          v0,8(sp)
	// Get G
	err=get_value();
	if (err) return err;
	if (g_source[g_srcpos]!=',') return ERR_SYNTAX;
	g_srcpos++;
	check_obj_space(1);
	g_object[g_objpos++]=0xAFA2000C; // sw          v0,12(sp)
	// Get B
	err=get_value();
	if (err) return err;
	call_lib_code(lib);
	check_obj_space(1);
	g_object[g_objpos++]=0x27BD000C; // addiu       sp,sp,12
	return 0;
}

char* param3_statement(enum libs lib){
	char* err;
	// Get 1st parameter
	err=get_value();
	if (err) return err;
	if (g_source[g_srcpos]!=',') return ERR_SYNTAX;
	g_srcpos++;
	check_obj_space(2);
	g_object[g_objpos++]=0x27BDFFF8; // addiu       sp,sp,-8
	g_object[g_objpos++]=0xAFA20004; // sw          v0,4(sp)
	// Get 2nd parameter
	err=get_value();
	if (err) return err;
	if (g_source[g_srcpos]!=',') return ERR_SYNTAX;
	g_srcpos++;
	check_obj_space(1);
	g_object[g_objpos++]=0xAFA20008; // sw          v0,8(sp)
	// Get 3rd parameter
	err=get_value();
	if (err) return err;
	call_lib_code(lib);
	check_obj_space(1);
	g_object[g_objpos++]=0x27BD0008; // addiu       sp,sp,8
	return 0;
}

char* bgcolor_statement(){
	// BGCOLOR R,G,B
	return param3_statement(LIB_BGCOLOR);
}

char* pcg_statement(){
	// PCG ASCII,D1,D2
	return param3_statement(LIB_PCG);
}

char* usepcg_statement(){
	int objpos=g_objpos;
	if (get_value()) {
		// Getting integer failed.
		// It supporsed to be not parameter
		// and same as parameter=1.
		g_objpos=objpos;
		check_obj_space(1);
		g_object[g_objpos++]=0x34020001; //ori         v0,zero,0x01
	}
	call_lib_code(LIB_USEPCG);
	return 0;
}

char* usegraphic_statement(){
	int objpos=g_objpos;
	if (get_value()) {
		// Getting integer failed.
		// It supporsed to be not parameter
		// and same as parameter=1.
		g_objpos=objpos;
		check_obj_space(1);
		g_object[g_objpos++]=0x34020001; //ori         v0,zero,0x01
	}
	call_lib_code(LIB_USEGRAPHIC);
	return 0;
}

char* cls_statement(){
	call_lib_code(LIB_CLS);
	return 0;
}

char* gcls_statement(){
	call_lib_code(LIB_GCLS);
	return 0;
}

char* color_statement(){
	char* err;
	err=get_value();
	if (err) return err;
	call_lib_code(LIB_COLOR);
	return 0;
}

char* gcolor_statement(){
	char* err;
	err=get_value();
	if (err) return err;
	call_lib_code(LIB_GCOLOR);
	return 0;
}

char* param2_statement(enum libs lib){
	char* err;
	// Get 1st
	err=get_value();
	if (err) return err;
	if (g_source[g_srcpos]!=',') return ERR_SYNTAX;
	g_srcpos++;
	check_obj_space(2);
	g_object[g_objpos++]=0x27BDFFFC; // addiu       sp,sp,-4
	g_object[g_objpos++]=0xAFA20004; // sw          v0,4(sp)
	// Get 2nd
	err=get_value();
	if (err) return err;
	call_lib_code(lib);
	check_obj_space(1);
	g_object[g_objpos++]=0x27BD0004; // addiu       sp,sp,4
	return 0;
}

char* system_statement(){
	// SYSTEM X,Y
	char* err;
	// Get 1st
	err=get_value();
	if (err) return err;
	if (g_source[g_srcpos]!=',') return ERR_SYNTAX;
	g_srcpos++;
	check_obj_space(2);
	g_object[g_objpos++]=0x27BDFFFC; // addiu       sp,sp,-4
	g_object[g_objpos++]=0xAFA20004; // sw          v0,4(sp)
	// Get 2nd
	err=get_value();
	if (err) return err;
	check_obj_space(2);
	g_object[g_objpos++]=0x8FA40004; // lw          a0,4(sp)
	g_object[g_objpos++]=0x27BD0004; // addiu       sp,sp,4
	call_lib_code(LIB_SYSTEM);
	return 0;
}

char* cursor_statement(){
	// CURSOR X,Y
	return param2_statement(LIB_CURSOR);
}

char* scroll_statement(){
	// SCROLL X,Y
	return param2_statement(LIB_SCROLL);
}

char* drawcount_statement(){
	char* err;
	err=get_value();
	if (err) return err;
	call_lib_code(LIB_SETDRAWCOUNT);
	return 0;
}

char* wait_statement(){
	char* err;
	err=get_value();
	if (err) return err;
	call_lib_code(LIB_WAIT);
	return 0;
}

char* width_statement(){
	char* err;
	err=get_value();
	if (err) return err;
	call_lib_code(LIB_WIDTH);
	return 0;
}

char* var_statement_sub(int a0, int a1){
	// Construct parameter-setting scripts
	if (a0&0xffff0000) {
		check_obj_space(1);
		g_object[g_objpos++]=0x3C040000|(a0>>16);        // lui         a0,XXXX
		if (a0&0x0000ffff) {
			check_obj_space(1);
			g_object[g_objpos++]=0x34840000|(a0&0xffff); // ori         a0,a0,XXXX
		}
	} else if (a0&0x0000ffff) {
		check_obj_space(1);
		g_object[g_objpos++]=0x34040000|(a0&0xffff);     // ori         a0,zero,xxxx
	}
	if (a1&0xffff0000) {
		check_obj_space(1);
		g_object[g_objpos++]=0x3C050000|(a1>>16);        // lui         a1,XXXX
		if (a1&0x0000ffff) {
			check_obj_space(1);
			g_object[g_objpos++]=0x34A50000|(a1&0xffff); // ori         a1,a1,XXXX
		}
	} else if (a1&0x0000ffff) {
		check_obj_space(1);
		g_object[g_objpos++]=0x34050000|(a1&0xffff);     // ori         a1,zero,xxxx
	} else if (a0&0xff000000) {
		// # of variables is 4. Reset $a1 to let lib_var() know that there is no more variables to store.
		check_obj_space(1);
		g_object[g_objpos++]=0x34050000;                 // ori         a1,zero,0
	}
	return 0;
}

char* var_statement(){
	char* err;
	int i,j,a0,a1;
	static int prevpos;
	short stack;
	do {
		// For stack, 4 bytes are used for return address,
		// 4 bytes are used in lib_var(),
		// and additinal bytes are used for storing variable values.
		stack=8;
		a0=a1=0;
		for (i=0;i<8;i++){
			// Determine the variable number from source code
			j=get_var_number();
			if (j<0) return ERR_SYNTAX;
			stack+=4;
			// Create parameters in $a0, or $a1
			if (i<4) {
				a0=(a0<<8)|(j+1);
			} else {
				a1=(a1<<8)|(j+1);
			}
			// Check remaining variable(s) 
			if (g_source[g_srcpos]=='#' || g_source[g_srcpos]=='$') g_srcpos++;
			next_position();
			if (g_source[g_srcpos]!=',') break;
			g_srcpos++;
		}
		// Jump to push routine, first
		check_obj_space(2);
		prevpos=g_objpos;
		g_object[g_objpos++]=0x04130000;                    // bgezall     zero,label1
		g_object[g_objpos++]=0x27BD0000|((0-stack)&0xffff); // addiu       sp,sp,-xx
		// Process will be here after RETURN statement
		// Pop routine
		err=var_statement_sub(a0,a1);                       // Prepare a0, and a1
		if (err) return err;
		call_lib_code(LIB_VAR_POP);
		// Restore stack and return
		check_obj_space(3);
		g_object[g_objpos++]=0x8FA30000|stack;              // lw          v1,xx(sp)
		g_object[g_objpos++]=0x00600008;                    // jr          v1
		g_object[g_objpos++]=0x27BD0000|stack;              // addiu       sp,sp,xx
		// Push rotine
		g_object[prevpos]|=g_objpos-prevpos-1;              // label1:
		check_obj_space(1);
		g_object[g_objpos++]=0xAFBF0004;                    // sw          ra,4(sp)
		err=var_statement_sub(a0,a1);                       // Prepare a0, and a1
		if (err) return err;
		call_lib_code(LIB_VAR_PUSH);

	} while (g_source[g_srcpos-1]==',');
	// Renew sp stored in s5 stack.
	check_obj_space(1);
	g_object[g_objpos++]=0xAEBD0000|ARGS_S5_SP; // sw          sp,-12(s5)
	return 0;
}


char* graphic_statement(enum functions func){
	/*
		PSET X1,Y1[,C]
		LINE X1,Y1,X2,Y2[,C]
		BOXFILL X1,Y1,X2,Y2[,C]
		CIRCLE X1,Y1,R[,C]
		CIRCLEFILL X1,Y1,R[,C]
		GPRINT X1,Y1,C,BC,S$
		PUTBMP X1,Y1,M,N,BMP
		4(sp): X1
		8(sp): Y1
		12(sp): X2/R/M
		16(sp): Y2/N
		v0: C/S$/BMP
	*/
	char* err;
	int spos;
	int paramnum;
	switch(func){
		case FUNC_PSET:// X1,Y1[,C]
		case FUNC_GCOLOR:// X1,Y1
		case FUNC_POINT:// X1,Y1
			paramnum=2;
			break;
		case FUNC_CIRCLE:// X1,Y1,R[,C]
		case FUNC_CIRCLEFILL:// X1,Y1,R[,C]
			paramnum=3;
			break;
		case FUNC_LINE:// X1,Y1,X2,Y2[,C]
		case FUNC_BOXFILL:// X1,Y1,X2,Y2[,C]
			paramnum=4;
			break;
		case FUNC_GPRINT:// X1,Y1,C,BC,S$
		case FUNC_PUTBMP:// X1,Y1,M,N,BMP
			paramnum=5;
			break;
		default:
			return ERR_UNKNOWN;
	}

	next_position();
	if (g_source[g_srcpos]==',') {
		// X1 and Y1 is omitted. Set 0x80000000 for both.
		check_obj_space(4);
		g_object[g_objpos++]=0x27BDFFF0;              // addiu       sp,sp,-16
		g_object[g_objpos++]=0x3C028000;              // lui         v0,0x0080
		g_object[g_objpos++]=0xAFA20004;              // sw          v0,4(sp)
		g_object[g_objpos++]=0xAFA20008;              // sw          v0,8(sp)
	} else {
		// X1
		err=get_value();
		if (err) return err;
		check_obj_space(2);
		g_object[g_objpos++]=0x27BDFFF0;              // addiu       sp,sp,-16
		g_object[g_objpos++]=0xAFA20004;              // sw          v0,4(sp)
		// Y1
		if (g_source[g_srcpos]!=',') return ERR_SYNTAX;
		g_srcpos++;
		err=get_value();
		if (err) return err;
		check_obj_space(1);
		g_object[g_objpos++]=0xAFA20008;              // sw          v0,8(sp)
	}
	if (2<paramnum) {
		// X2, R, or M
		if (g_source[g_srcpos]!=',') return ERR_SYNTAX;
		g_srcpos++;
		err=get_value();
		if (err) return err;
		check_obj_space(1);
		g_object[g_objpos++]=0xAFA2000C;          // sw          v0,12(sp)
		if (3<paramnum) {
			// Y2, BC, or N
			if (g_source[g_srcpos]!=',') return ERR_SYNTAX;
			g_srcpos++;
			err=get_value();
			if (err) return err;
			check_obj_space(1);
			g_object[g_objpos++]=0xAFA20010;      // sw          v0,16(sp)
		}
	}
	if (func==FUNC_GPRINT) {
		// S$
		if (g_source[g_srcpos]!=',') return ERR_SYNTAX;
		g_srcpos++;
		err=get_string();
		if (err) return err;
	} else if (func==FUNC_PUTBMP) {
		// BMP
		if (g_source[g_srcpos]!=',') return ERR_SYNTAX;
		g_srcpos++;
		spos=g_srcpos;
		err=get_label();
		if (g_label && !err) {
			if (search_var_name(g_label)!=-1) {
				// This is a long var name.
				g_label=0;
				g_srcpos=spos;
			}
		}
		if (g_label && !err) {
			// Label/number is constant.
			// Linker will change following codes later.
			// Note that 0x0814xxxx and 0x0815xxxx are specific codes for these.
			check_obj_space(2);
			g_object[g_objpos++]=0x08140000|((g_label>>16)&0x0000FFFF); // lui   v0,xxxx
			g_object[g_objpos++]=0x08150000|(g_label&0x0000FFFF);       // ori v0,v0,xxxx
			// Change func to FUNC_PUTBMP2 (label mode).
			func=FUNC_PUTBMP2;
		} else {
			err=get_value();
			if (err) return err;
		}
	} else {
		// [,C]
		if (g_source[g_srcpos]==',') {
			g_srcpos++;
			err=get_value();
			if (err) return err;
		} else {
			// If C is omitted, use -1.
			check_obj_space(1);
			g_object[g_objpos++]=0x2402FFFF;      // addiu       v0,zero,-1
		}
	}
	// Call library
	call_lib_code(LIB_GRAPHIC | func);
	// Restore stack pointer
	check_obj_space(1);
	g_object[g_objpos++]=0x27BD0010;              // addiu       sp,sp,16
	return 0;
}

char* fopen_statement_main(enum functions func){
	// func is either FUNC_FOPENST or FUNC_FOPEN
	char* err;
	// Get 1st
	err=get_string();
	if (err) return err;
	if (g_source[g_srcpos]!=',') return ERR_SYNTAX;
	g_srcpos++;
	check_obj_space(2);
	g_object[g_objpos++]=0x27BDFFF8; // addiu       sp,sp,-8
	g_object[g_objpos++]=0xAFA20004; // sw          v0,4(sp)
	// Get 2nd
	err=get_string();
	if (err) return err;
	check_obj_space(1);
	g_object[g_objpos++]=0xAFA20008; // sw          v0,8(sp)
	// Get 3rd
	if (g_source[g_srcpos]==',') {
		g_srcpos++;
		err=get_value();
		if (err) return err;
	} else {
		// If 3rd parameter is omitted, use 0.
		check_obj_space(1);
		g_object[g_objpos++]=0x24020000;      // addiu       v0,zero,0
	}
	call_lib_code(LIB_FILE | func);
	check_obj_space(1);
	g_object[g_objpos++]=0x27BD0008; // addiu       sp,sp,8
	return 0;
}

char* file_statement(){
	char* err;
	err=get_value();
	if (err) return err;
	call_lib_code(LIB_FILE | FUNC_FILE);
	return 0;
}

char* fclose_statement(){
	char* err;
	int orgpos=g_srcpos;
	if (endOfStatement()) {
		// If no argument, use 0
		check_obj_space(1);
		g_object[g_objpos++]=0x24020000;      // addiu       v0,zero,0
	} else {
		err=get_value();
		if (err) return err;
	}
	call_lib_code(LIB_FILE | FUNC_FCLOSE);
	return 0;
}

/*

TODO: candidates to add:

int FSattrib (FSFILE * file, unsigned char attributes);
int FSrename (const char * fileName, FSFILE * fo);
int FSmkdir (char * path);
int FSrmdir (char * path, unsigned char rmsubdirs);
int SetClockVars (unsigned int year, unsigned char month, unsigned char day, unsigned char hour, unsigned char minute, unsigned char second);
int FindFirst (const char * fileName, unsigned int attr, SearchRec * rec);
int FindNext (SearchRec * rec);

*/

char* fget_statement(){
	return param2_statement(LIB_FILE | FUNC_FGET);
}

char* fput_statement(){
	return param2_statement(LIB_FILE | FUNC_FPUT);
}

char* fseek_statement(){
	char* err;
	err=get_value();
	if (err) return err;
	call_lib_code(LIB_FILE | FUNC_FSEEK);
	return 0;
}

char* fputc_statement(){
	char* err;
	err=get_value();
	if (err) return err;
	call_lib_code(LIB_FILE | FUNC_FPUTC);
	return 0;
}

char* fremove_statement(){
	char* err;
	err=get_string();
	if (err) return err;
	call_lib_code(LIB_FILE | FUNC_FREMOVE);
	return 0;
}

char* usevar_statement(){
	char* err;
	int i;
	do {
		next_position();
		i=check_var_name();
		if (i<65536) return ERR_SYNTAX;
		err=register_var_name(i);
		if (err) return err;
		if (g_source[g_srcpos]=='#' || g_source[g_srcpos]=='$') g_srcpos++;
		next_position();
		if (g_source[g_srcpos]==',') {
			g_srcpos++;
		} else {
			break;
		}
	} while(1);
	return 0;
}

char* playwave_statement(){
	char* err;
	err=get_string();
	if (err) return err;
	check_obj_space(2);
	g_object[g_objpos++]=0x27BDFFFC; // addiu       sp,sp,-4
	g_object[g_objpos++]=0xAFA20004; // sw          v0,4(sp)
	if (g_source[g_srcpos]==',') {
		g_srcpos++;
		// Get 2nd
		err=get_value();
		if (err) return err;
	} else {
		// If 2rd parameter is omitted, use 0.
		check_obj_space(1);
		g_object[g_objpos++]=0x24020000;      // addiu       v0,zero,0
	}
	call_lib_code(LIB_PLAYWAVE);
	check_obj_space(1);
	g_object[g_objpos++]=0x27BD0004; // addiu       sp,sp,4
	return 0;
}

char* useclass_statement(){
	char* err;
	int i;
	int* cmpdata;
	do {
		next_position();
		i=check_var_name();
		if (i<65536) return ERR_SYNTAX;
		// Check if the class already exists
		cmpdata_reset();
		while(cmpdata=cmpdata_find(CMPDATA_CLASS)){
			if (cmpdata[1]==i) {
				// The class was already defined.
				i=0;
				break;
			}
		}
		if (i) {
			// Remove a objects before USECLASS statement
			g_objpos=0;
			// Insert a NOP assembly. This will be replaced by jump statement.
			check_obj_space(1);
			g_object[g_objpos++]=0x00000000; // nop
			// Load new file to define class statement.
			g_class=i;
			return ERR_COMPILE_CLASS;
		}
		if (g_source[g_srcpos]==',') {
			g_srcpos++;
		} else {
			break;
		}
	} while(1);
	return 0;
}

char* setdir_statement(){
	char* err;
	err=get_string();
	if (err) return err;
	call_lib_code(LIB_SETDIR);
	return 0;
}

char* option_statement(){
	while(1){
		next_position();
		if (nextCodeIs("NOLINENUM")) {
			g_option_nolinenum=1;
		} else if (nextCodeIs("FASTFIELD")) {
			g_option_fastfield=1;
		} else if (nextCodeIs("CLASSCODE")) {
			if (g_compiling_class) {
				// Do nothing. Do not try to rewind the object,
				// as an exception will occur if you will do this.
				// There are many things to reset if the created
				// object will be deleted.
			} else {
				// End the compile of main file
				return ERR_OPTION_CLASSCODE;
			}
		} else {
			return ERR_SYNTAX;
		}
		next_position();
		if (g_source[g_srcpos]==',') {
			g_srcpos++;
		} else {
			break;
		}
	}
	return 0;
}

char* idle_statement(){
	check_obj_space(2);
	g_object[g_objpos++]=0x42000020; // wait
	return 0;	
}

#ifdef __DEBUG
	char* debug_statement(){
		call_lib_code(LIB_DEBUG);
		return 0;
	}
#endif

// Aliases follow

char* poke_statement(){
	return poke_statement_sub(8);
}

char* poke16_statement(){
	return poke_statement_sub(16);
}

char* poke32_statement(){
	return poke_statement_sub(32);
}

char* palette_statement(){
	return param4_statement(LIB_PALETTE);
}

char* gpalette_statement(){
	return param4_statement(LIB_GPALETTE);
}

char* print_statement(){
	return print_statement_main(LIB_PRINTSTR,LIB_STRING);
}

char* pset_statement(){
	return graphic_statement(FUNC_PSET);
}

char* line_statement(){
	return graphic_statement(FUNC_LINE);
}

char* boxfill_statement(){
	return graphic_statement(FUNC_BOXFILL);
}

char* circle_statement(){
	return graphic_statement(FUNC_CIRCLE);
}

char* circlefill_statement(){
	return graphic_statement(FUNC_CIRCLEFILL);
}

char* gprint_statement(){
	return graphic_statement(FUNC_GPRINT);
}

char* putbmp_statement(){
	return graphic_statement(FUNC_PUTBMP);
}

char* point_statement(){
	return graphic_statement(FUNC_POINT);
}

char* fopen_statement(){
	return fopen_statement_main(FUNC_FOPENST);
}

char* fprint_statement(){
	return print_statement_main(LIB_FILE | FUNC_FPRINTSTR,LIB_FILE | FUNC_FSTRING);
}

static const void* statement_list[]={
	"REM",rem_statement,
	"SOUND ",sound_statement,
	"MUSIC ",music_statement,
	"DRAWCOUNT ",drawcount_statement,
	"CURSOR ",cursor_statement,
	"PALETTE ",palette_statement,
	"GPALETTE ",gpalette_statement,
	"BGCOLOR ",bgcolor_statement,
	"CLS",cls_statement,
	"GCLS",gcls_statement,
	"COLOR ",color_statement,
	"GCOLOR ",gcolor_statement,
	"RESTORE ",restore_statement,
	"DATA ",data_statement,
	"CDATA ",cdata_statement,
	"LABEL ",label_statement,
	"DIM ",dim_statement,
	"CLEAR",clear_statement,
	"PRINT",print_statement,
	"IF ",if_statement,
	"ELSEIF ",elseif_statement,
	"ELSE",else_statement,
	"ENDIF",endif_statement,
	"END",end_statement,
	"EXEC ",exec_statement,
	"GOTO ",goto_statement,
	"GOSUB ",gosub_statement,
	"RETURN",return_statement,
	"POKE ",poke_statement,
	"POKE16 ",poke16_statement,
	"POKE32 ",poke32_statement,
	"FOR ",for_statement,
	"NEXT",next_statement,
	"LET ",let_statement,
	"PCG ",pcg_statement,
	"USEPCG",usepcg_statement,
	"SCROLL ",scroll_statement,
	"WAIT ",wait_statement,
	"USEGRAPHIC",usegraphic_statement,
	"PSET ",pset_statement,
	"LINE ",line_statement,
	"BOXFILL ",boxfill_statement,
	"CIRCLE ",circle_statement,
	"CIRCLEFILL ",circlefill_statement,
	"GPRINT ",gprint_statement,
	"PUTBMP ",putbmp_statement,
	"POINT ",point_statement,
	"VAR ",var_statement,
	"DO",do_statement,
	"LOOP",loop_statement,
	"WHILE ",while_statement,
	"WEND",wend_statement,
	"BREAK",break_statement,
	"CONTINUE",continue_statement,
	"SYSTEM",system_statement,
	"WIDTH ",width_statement,
	"FOPEN ",fopen_statement,
	"FILE ",file_statement,
	"FCLOSE",fclose_statement,
	"FPRINT ",fprint_statement,
	"FGET ",fget_statement,
	"FPUT ",fput_statement,
	"FPUTC ",fputc_statement,
	"FSEEK ",fseek_statement,
	"FREMOVE ",fremove_statement,
	"USEVAR ",usevar_statement,
	"PLAYWAVE ",playwave_statement,
	"USECLASS ",useclass_statement,
	"FIELD ",field_statement,
	"METHOD ",method_statement,
	"DELETE ",delete_statement,
	"CALL ",call_statement,
	"STATIC ",static_statement,
	"SETDIR ",setdir_statement,
	"OPTION ",option_statement,
	"USETIMER ",usetimer_statement,
	"TIMER ",timer_statement,
	"INTERRUPT ",interrupt_statement,
	"IDLE",idle_statement,
	"CORETIMER",coretimer_statement,
	// List of additional statements follows
	ADDITIONAL_STATEMENTS
};

char* statement(void){
	char* err;
	int prevpos;
	int i;
	char* (*f)();
	// Clear flag for temp area usage.
	g_temp_area_used=0;
	// Initialize stack handler used for value
	g_sdepth=g_maxsdepth=0;
	// Allow shifting code object when stack is used.
	// This will be disalloed when CMPDATA_UNSOLVED etc is used.
	g_allow_shift_obj=1;
	// Seek the statement
	for (i=0;i<sizeof(statement_list)/sizeof(statement_list[0]);i+=2){
		if (nextCodeIs((char*)statement_list[i])) break;
	}
	if (i<sizeof(statement_list)/sizeof(statement_list[0])) {
		// Statement found. Call it.
		f=statement_list[i+1];
		err=f();
#ifdef __DEBUG
	} else if (nextCodeIs("DEBUG")) {
		err=debug_statement();
#endif
	} else {
		err=let_statement();
	}
	if (err) return err;
	// Stack handler must be zero here.
	if (g_sdepth!=0) return ERR_UNKNOWN;
	// Check if temp area is used
	if (g_temp_area_used) {
		// Temp area is used. Insert a garbage collection flag setting routine.
		check_obj_space(1);
		g_object[g_objpos++]=0x7ED6F000;//   ext         s6,s6,0,31
	}
	return 0;
}
