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
#include "api.h"

char* music_function(){
	char* err;
	next_position();
	if (g_source[g_srcpos]==')') {
		check_obj_space(1);
		g_object[g_objpos++]=0x34020003; //ori         v0,zero,0x03
	} else {
		err=get_value();
		if (err) return err;
	}
	call_lib_code(LIB_MUSICFUNC);
	return 0;
}

char* read_function(){
	// This function is not valid in class file.
	if (g_compiling_class) return ERR_INVALID_CLASS;
	call_lib_code(LIB_READ);
	return 0;
}

char* cread_function(){
	call_lib_code(LIB_CREAD);
	return 0;
}

char* gosub_function(){
	// Check if garbage collection has been done.
	// This check is required because the used temporary area would be changed 
	// in sub routine. 
	if (g_temp_area_used) return ERR_GOSUB_ASH;
	return gosub_statement();
}
char* strncmp_function(){
	char* err;
	err=get_string();
	if (err) return err;
	check_obj_space(2);
	g_object[g_objpos++]=0x27BDFFF8; // addiu       sp,sp,-8
	g_object[g_objpos++]=0xAFA20004; // sw          v0,4(sp)
	if (g_source[g_srcpos]!=',') return ERR_SYNTAX;
	g_srcpos++;
	err=get_string();
	if (err) return err;
	check_obj_space(1);
	g_object[g_objpos++]=0xAFA20008; // sw          v0,8(sp)
	if (g_source[g_srcpos]!=',') return ERR_SYNTAX;
	g_srcpos++;
	err=get_value();
	if (err) return err;
	call_lib_code(LIB_STRNCMP);
	check_obj_space(1);
	g_object[g_objpos++]=0x27BD0008; // addiu       sp,sp,8
	return 0;
}
char* len_function(){
	char* err;
	err=get_string();
	if (err) return err;
	check_obj_space(5);
	g_object[g_objpos++]=0x2443FFFF; // addiu       v1,v0,-1
	                                 // loop:
	g_object[g_objpos++]=0x80640001; // lb          a0,1(v1)
	g_object[g_objpos++]=0x1480FFFE; // bne         a0,zero,loop
	g_object[g_objpos++]=0x24630001; // addiu       v1,v1,1
	g_object[g_objpos++]=0x00621023; // subu        v0,v1,v0
	return 0;
}

char* asc_function(){
	char* err;
	err=get_string();
	if (err) return err;
	check_obj_space(1);
	g_object[g_objpos++]=0x90420000; // lbu         v0,0(v0)
	return 0;
}

char* val_function(){
	char* err;
	err=get_string();
	if (err) return err;
	call_lib_code(LIB_VAL);
	return 0;	
}

char* peek_function_sub(int bits){
	char* err;
	err=get_value();
	if (err) return err;
	check_obj_space(1);
	switch(bits){
		case 32:
			g_object[g_objpos++]=0x8C420000; // lw          v0,0(v0)
			break;
		case 16:
			g_object[g_objpos++]=0x94420000; // lhu         v0,0(v0)
			break;
		case 8:
		default:
			g_object[g_objpos++]=0x90420000; // lbu         v0,0(v0)
			break;
	}
	return 0;
}

char* sgn_function(){
	char* err;
	err=get_value();
	if (err) return err;
	check_obj_space(5);
	g_object[g_objpos++]=0x10400004; // beq         v0,zero,end
	g_object[g_objpos++]=0x24030001; // addiu       v1,zero,1
	g_object[g_objpos++]=0x1C400002; // bgtz        v0,end
	g_object[g_objpos++]=0x00601021; // addu        v0,v1,zero
	g_object[g_objpos++]=0x00031023; // subu        v0,zero,v1
	                                 // end:
	return 0;
}

char* abs_function(){
	char* err;
	err=get_value();
	if (err) return err;
	check_obj_space(3);
	g_object[g_objpos++]=0x00021FC3; //sra         v1,v0,0x1f
	g_object[g_objpos++]=0x00621026; //xor         v0,v1,v0
	g_object[g_objpos++]=0x00431023; //subu        v0,v0,v1
	return 0;
}

char* not_function(){
	char* err;
	err=get_value();
	if (err) return err;
	check_obj_space(1);
	g_object[g_objpos++]=0x2C420001; //sltiu       v0,v0,1
	return 0;
}

char* rnd_function(){
	call_lib_code(LIB_RND);
	return 0;
}

char* chr_function(void){
	char* err;
	err=get_value();
	if (err) return err;
	call_lib_code(LIB_CHR);
	return 0;
}
char* hex_function(void){
	char* err;
	err=get_value();
	if (err) return err;
	if (g_source[g_srcpos]==',') {
		// Second argument found.
		// Get is as $a0.
		g_srcpos++;
		check_obj_space(2);
		g_object[g_objpos++]=0x27BDFFFC; //addiu       sp,sp,-4
		g_object[g_objpos++]=0xAFA20004; //sw          v0,4(sp)
		err=get_value();
		if (err) return err;
		check_obj_space(3);
		g_object[g_objpos++]=0x00022021; //a0,zero,v0
		g_object[g_objpos++]=0x8FA20004; //lw          v0,4(sp)
		g_object[g_objpos++]=0x27BD0004; //addiu       sp,sp,4
	} else {
		// Second argument not found.
		// Set $a0 to 0.
		check_obj_space(1);
		g_object[g_objpos++]=0x24040000; //addiu       a0,zero,0
	}
	call_lib_code(LIB_HEX);
	return 0;
}

char* dec_function(void){
	char* err;
	err=get_value();
	if (err) return err;
	call_lib_code(LIB_DEC);
	return 0;
}

char* keys_function(void){
	char* err;
	next_position();
	if (g_source[g_srcpos]==')') {
		check_obj_space(1);
		g_object[g_objpos++]=0x3402003F; //ori         v0,zero,0x3f
	} else {
		err=get_value();
		if (err) return err;
	}	
	call_lib_code(LIB_KEYS);
	return 0;
}

char* tvram_function(void){
	char* err;
	int i;
	next_position();
	if (g_source[g_srcpos]==')') {
		i=(int)(&TVRAM[0]);
		i-=g_gp;
		check_obj_space(1);
		g_object[g_objpos++]=0x27820000|(i&0x0000FFFF);       // addiu       v0,gp,xxxx
	} else {
		err=get_value();
		if (err) return err;
		i=(int)(&TVRAM[0]);
		i-=g_gp;
		check_obj_space(3);
		g_object[g_objpos++]=0x27830000|(i&0x0000FFFF);       // addiu       v1,gp,xxxx
		g_object[g_objpos++]=0x00621821;                      // addu        v1,v1,v0
		g_object[g_objpos++]=0x90620000;                      // lbu         v0,0(v1)
	}	
	return 0;
}

char* drawcount_function(void){
	call_lib_code(LIB_DRAWCOUNT);
	return 0;
}

char* input_function(void){
	call_lib_code(LIB_INPUT);
	return 0;
}

char* inkey_function(void){
	char* err;
	next_position();
	if (g_source[g_srcpos]==')') {
		check_obj_space(1);
		g_object[g_objpos++]=0x34020000; //ori         v0,zero,0x00
	} else {
		err=get_value();
		if (err) return err;
	}	
	call_lib_code(LIB_INKEY);
	return 0;
}

char* args_function(void){
	return args_function_main();
}

char* system_function(void){
	char* err;
	err=get_value();
	if (err) return err;
	g_object[g_objpos++]=0x00402021; //   addu        a0,v0,zero
	call_lib_code(LIB_SYSTEM);
	return 0;
}

char* sprintf_function(void){
	char* err;
	err=get_string();
	if (err) return err;
	next_position();
	if (g_source[g_srcpos]!=',') return ERR_SYNTAX;
	g_srcpos++;
	check_obj_space(2);
	g_object[g_objpos++]=0x27BDFFFC; //addiu       sp,sp,-4
	g_object[g_objpos++]=0xAFA20004; //sw          v0,4(sp)
	err=get_float();
	if (err) return err;
	check_obj_space(3);
	g_object[g_objpos++]=0x00022021; //addu        a0,zero,v0
	g_object[g_objpos++]=0x8FA20004; //lw          v0,4(sp)
	g_object[g_objpos++]=0x27BD0004; //addiu       sp,sp,4
	call_lib_code(LIB_SPRINTF);
	return 0;
}

char* floatstr_function(void){
	char* err;
	err=get_float();
	if (err) return err;
	check_obj_space(2);
	g_object[g_objpos++]=0x00022021; //addu        a0,zero,v0
	g_object[g_objpos++]=0x34020000; //ori         v0,zero,0x0000
	call_lib_code(LIB_SPRINTF);
	return 0;
}

char* floatsharp_function(void){
	char* err;
	err=get_value();
	if (err) return err;
	call_lib_code(LIB_FLOATFUNCS | FUNC_FLOAT);
	return 0;
}

char* valsharp_function(void){
	char* err;
	err=get_string();
	if (err) return err;
	call_lib_code(LIB_FLOATFUNCS | FUNC_VALSHARP);
	return 0;
}

char* int_function(void){
	char* err;
	err=get_float();
	if (err) return err;
	call_lib_code(LIB_FLOATFUNCS | FUNC_INT);
	return 0;
}

char* fseek_function(){
	call_lib_code(LIB_FILE | FUNC_FTELL);
	return 0;
}

char* flen_function(){
	call_lib_code(LIB_FILE | FUNC_FLEN);
	return 0;
}

char* fgetc_function(){
	call_lib_code(LIB_FILE | FUNC_FGETC);
	return 0;
}

char* finput_function(){
	char* err;
	next_position();
	if (g_source[g_srcpos]!=')') {
		err=get_value();
		if (err) return err;
	} else {
		// Parameter will be zero if not defined
		check_obj_space(1);
		g_object[g_objpos++]=0x34020000; // ori v0,zero,0
	}
	call_lib_code(LIB_FILE | FUNC_FINPUT);
	return 0;
}

char* feof_function(){
	call_lib_code(LIB_FILE | FUNC_FEOF);
	return 0;
}

char* playwave_function(){
	char* err;
	next_position();
	if (g_source[g_srcpos]!=')') {
		// Get param
		err=get_value();
		if (err) return err;
	} else {
		// If parameter is omitted, use 0.
		check_obj_space(1);
		g_object[g_objpos++]=0x24020000;      // addiu       v0,zero,0
	}
	call_lib_code(LIB_PLAYWAVEFUNC);
	return 0;
}

char* setdir_function(){
	char* err;
	err=get_string();
	if (err) return err;
	call_lib_code(LIB_SETDIRFUNC);
	return 0;
}

char* getdir_function(){
	call_lib_code(LIB_GETDIR);
	return 0;
}

char* exec_function(){
	return exec_statement();
}

char* readkey_function(){
	call_lib_code(LIB_READKEY);
	return 0;
}

char* float_constant(float val){
	volatile int i;
	((float*)(&i))[0]=val;
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
	return 0;	
}

char* float_1param_function(enum functions func){
	char* err;
	err=get_float();
	if (err) return err;
	call_lib_code(LIB_FLOATFUNCS | func);
	return 0;
}

char* float_2param_function(enum functions func){
	char* err;
	err=get_float();
	if (err) return err;
	next_position();
	if (g_source[g_srcpos]!=',') return ERR_SYNTAX;
	g_srcpos++;
	check_obj_space(2);
	g_object[g_objpos++]=0x27BDFFFC; //addiu       sp,sp,-4
	g_object[g_objpos++]=0xAFA20004; //sw          v0,4(sp)
	err=get_float();
	if (err) return err;
	check_obj_space(3);
	g_object[g_objpos++]=0x00022021; //addu        a0,zero,v0
	g_object[g_objpos++]=0x8FA20004; //lw          v0,4(sp)
	g_object[g_objpos++]=0x27BD0004; //addiu       sp,sp,4
	call_lib_code(LIB_FLOATFUNCS | func);
	return 0;
}

char* float_function(void){
	char* err;
	if (nextCodeIs("FLOAT#(")) {
		err=floatsharp_function();
	} else if (nextCodeIs("VAL#(")) {
		err=valsharp_function();
	} else if (nextCodeIs("SIN#(")) {
		err=float_1param_function(FUNC_SIN);
	} else if (nextCodeIs("COS#(")) {
		err=float_1param_function(FUNC_COS);
	} else if (nextCodeIs("TAN#(")) {
		err=float_1param_function(FUNC_TAN);
	} else if (nextCodeIs("ASIN#(")) {
		err=float_1param_function(FUNC_ASIN);
	} else if (nextCodeIs("ACOS#(")) {
		err=float_1param_function(FUNC_ACOS);
	} else if (nextCodeIs("ATAN#(")) {
		err=float_1param_function(FUNC_ATAN);
	} else if (nextCodeIs("ATAN2#(")) {
		err=float_2param_function(FUNC_ATAN2);
	} else if (nextCodeIs("SINH#(")) {
		err=float_1param_function(FUNC_SINH);
	} else if (nextCodeIs("COSH#(")) {
		err=float_1param_function(FUNC_COSH);
	} else if (nextCodeIs("TANH#(")) {
		err=float_1param_function(FUNC_TANH);
	} else if (nextCodeIs("EXP#(")) {
		err=float_1param_function(FUNC_EXP);
	} else if (nextCodeIs("LOG#(")) {
		err=float_1param_function(FUNC_LOG);
	} else if (nextCodeIs("LOG10#(")) {
		err=float_1param_function(FUNC_LOG10);
	} else if (nextCodeIs("POW#(")) {
		err=float_2param_function(FUNC_POW);
	} else if (nextCodeIs("SQRT#(")) {
		err=float_1param_function(FUNC_SQRT);
	} else if (nextCodeIs("CEIL#(")) {
		err=float_1param_function(FUNC_CEIL);
	} else if (nextCodeIs("FLOOR#(")) {
		err=float_1param_function(FUNC_FLOOR);
	} else if (nextCodeIs("FABS#(")) {
		err=float_1param_function(FUNC_FABS);
	} else if (nextCodeIs("MODF#(")) {
		err=float_1param_function(FUNC_MODF);
	} else if (nextCodeIs("FMOD#(")) {
		err=float_2param_function(FUNC_FMOD);
	} else if (nextCodeIs("GOSUB#(")) {
		err=gosub_function();
	} else if (nextCodeIs("ARGS#(")) {
		err=args_function();
	} else if (nextCodeIs("PI#")) {
		return float_constant(3.141593);
	} else {
		// Check if static method of a class
		err=static_method('#');
		//return ERR_SYNTAX;
	}
	if (err) return err;
	if (g_source[g_srcpos]!=')') return ERR_SYNTAX;
	g_srcpos++;
	return 0;
}

static const void* str_func_list[]={
	"CHR$(",chr_function,
	"HEX$(",hex_function,
	"DEC$(",dec_function,
	"INPUT$(",input_function,
	"GOSUB$(",gosub_function,
	"ARGS$(",args_function,
	"READ$(",read_function,
	"SPRINTF$(",sprintf_function,
	"FLOAT$(",floatstr_function,
	"SYSTEM$(",system_function,
	"FINPUT$(",finput_function,
	"GETDIR$(",getdir_function,
	// Additional functions follow
	ADDITIONAL_STR_FUNCTIONS
};

char* str_function(void){
	char* err;
	int i;
	char* (*f)();
	// Seek the function
	for (i=0;i<sizeof(str_func_list)/sizeof(str_func_list[0]);i+=2){
		if (nextCodeIs((char*)str_func_list[i])) break;
	}
	if (i<sizeof(str_func_list)/sizeof(str_func_list[0])) {
		// Function found. Call it.
		f=str_func_list[i+1];
		err=f();
	} else {
		// Check if static method of a class
		err=static_method('$');
		//return ERR_SYNTAX;
	}
	if (err) return err;
	if (g_source[g_srcpos]!=')') return ERR_SYNTAX;
	g_srcpos++;
	return 0;
}

// Aliases follow

char* peek_function(){
	return peek_function_sub(8);
}

char* peek16_function(){
	return peek_function_sub(16);
}

char* peek32_function(){
	return peek_function_sub(32);
}

char* gcolor_function(){
	return graphic_statement(FUNC_GCOLOR);
}

char* fopen_function(){
	return fopen_statement_main(FUNC_FOPEN);
}

static const void* int_func_list[]={
	"NOT(",not_function,
	"DRAWCOUNT(",drawcount_function,
	"MUSIC(",music_function,
	"TVRAM(",tvram_function,
	"KEYS(",keys_function,
	"READ(",read_function,
	"CREAD(",cread_function,
	"GOSUB(",gosub_function,
	"STRNCMP(",strncmp_function,
	"PEEK(",peek_function,
	"PEEK16(",peek16_function,
	"PEEK32(",peek32_function,
	"LEN(",len_function,
	"ASC(",asc_function,
	"SGN(",sgn_function,
	"ABS(",abs_function,
	"RND(",rnd_function,
	"VAL(",val_function,
	"INKEY(",inkey_function,
	"ARGS(",args_function,
	"SYSTEM(",system_function,
	"INT(",int_function,
	"GCOLOR(",gcolor_function,
	"FOPEN(",fopen_function,
	"FSEEK(",fseek_function,
	"FLEN(",flen_function,
	"FGET(",fget_statement,
	"FPUT(",fput_statement,
	"FGETC(",fgetc_function,
	"FPUTC(",fputc_statement,
	"FREMOVE(",fremove_statement,
	"FEOF(",feof_function,
	"PLAYWAVE(",playwave_function,
	"NEW(",new_function,
	"SETDIR(",setdir_function,
	"TIMER(",timer_function,
	"EXEC(",exec_function,
	"CORETIMER(",coretimer_function,
	"READKEY(",readkey_function,
	// Additional functions follow
	ADDITIONAL_INT_FUNCTIONS
};

char* function(void){
	char* err;
	int i;
	char* (*f)();
	// Seek the function
	for (i=0;i<sizeof(int_func_list)/sizeof(int_func_list[0]);i+=2){
		if (nextCodeIs((char*)int_func_list[i])) break;
	}
	if (i<sizeof(int_func_list)/sizeof(int_func_list[0])) {
		// Function found. Call it.
		f=int_func_list[i+1];
		err=f();
	} else {
		// Check if static method of a class
		err=static_method(0);
		//return ERR_SYNTAX;
	}
	if (err) return err;
	if (g_source[g_srcpos]!=')') return ERR_SYNTAX;
	g_srcpos++;
	return 0;
}
