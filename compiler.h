/*
   This file is provided under the LGPL license ver 2.1.
   Written by Katsumi.
   http://hp.vector.co.jp/authors/VA016157/
   kmorimatsu@users.sourceforge.jp
*/

/*
	This file is shared by Megalopa and Zoea
*/

// Include envilonment specific configurations
#include "envspecific.h"

/* Definitions */
// Number of variables (including temporary ones)
#define ALLOC_VAR_NUM 36
// Block # dedicated for PCG
#define ALLOC_PCG_BLOCK 36
// Block # dedicated for GRAPHIC
#define ALLOC_GRAPHIC_BLOCK 37
// Block # dedicated for PLAYWAVE
#define ALLOC_WAVE_BLOCK 38
// Start # for long name variables
#define ALLOC_LNV_BLOCK 39
// Number of long name variables
#define ALLOC_LNV_NUM 190
// Start # of permanent blocks
#define ALLOC_PERM_BLOCK 229
// Number of blocks that can be assigned for memory allocation (including all above)
#define ALLOC_BLOCK_NUM 329

// Persistent RAM bytes used for object, heap and exception data
#ifndef PERSISTENT_RAM_SIZE
	// This must be defined in envspecific.h
	#define PERSISTENT_RAM_SIZE (1024*53)
#endif
// Exception data area bytes
#define EXCEPTION_DATA_SIZE (64)
// RAM size used for object and heap
#define RAMSIZE (PERSISTENT_RAM_SIZE-EXCEPTION_DATA_SIZE)

/* Enums */
enum variable{
	VAR_INTEGER,
	VAR_FLOAT,
	VAR_STRING
};

#define OP_MASK 0x001F
enum operator{
	OP_VOID=0,
	OP_OR  =1,
	OP_AND =2,
	OP_XOR =3,
	OP_EQ  =4,
	OP_NEQ =5,
	OP_LT  =6,
	OP_LTE =7,
	OP_MT  =8,
	OP_MTE =9,
	OP_SHL =10,
	OP_SHR =11,
	OP_ADD =12,
	OP_SUB =13,
	OP_MUL =14,
	OP_DIV =15,
	OP_REM =16
};

#define LIB_MASK 0xFE00
#define LIB_STEP 0x0200
enum libs{
	LIB_SOUND          =LIB_STEP*0,
	LIB_MUSICFUNC      =LIB_STEP*1,
	LIB_MUSIC          =LIB_STEP*2,
	LIB_SETDRAWCOUNT   =LIB_STEP*3,
	LIB_DRAWCOUNT      =LIB_STEP*4,
	LIB_PALETTE        =LIB_STEP*5,
	LIB_GPALETTE       =LIB_STEP*6,
	LIB_BGCOLOR        =LIB_STEP*7,
	LIB_CURSOR         =LIB_STEP*8,
	LIB_CLS            =LIB_STEP*9,
	LIB_GCLS           =LIB_STEP*10,
	LIB_COLOR          =LIB_STEP*11,
	LIB_GCOLOR         =LIB_STEP*12,
	LIB_KEYS           =LIB_STEP*13,
	LIB_RESTORE        =LIB_STEP*14,
	LIB_RESTORE2       =LIB_STEP*15,
	LIB_READ           =LIB_STEP*16,
	LIB_MIDSTR         =LIB_STEP*17,
	LIB_CLEAR          =LIB_STEP*18,
	LIB_DIV0           =LIB_STEP*19,
	LIB_LETSTR         =LIB_STEP*20,
	LIB_STRNCMP        =LIB_STEP*21,
	LIB_RND            =LIB_STEP*22,
	LIB_DEC            =LIB_STEP*23,
	LIB_HEX            =LIB_STEP*24,
	LIB_CHR            =LIB_STEP*25,
	LIB_CONNECT_STRING =LIB_STEP*26,
	LIB_STRING         =LIB_STEP*27,
	LIB_PRINTSTR       =LIB_STEP*28,
	LIB_LABEL          =LIB_STEP*29,
	LIB_DIM            =LIB_STEP*30,
	LIB_VAL            =LIB_STEP*31,
	LIB_INPUT          =LIB_STEP*32,
	LIB_INKEY          =LIB_STEP*33,
	LIB_USEPCG         =LIB_STEP*34,
	LIB_PCG            =LIB_STEP*35,
	LIB_SCROLL         =LIB_STEP*36,
	LIB_WAIT           =LIB_STEP*37,
	LIB_VAR_PUSH       =LIB_STEP*38,
	LIB_VAR_POP        =LIB_STEP*39,
	LIB_SYSTEM         =LIB_STEP*40,
	LIB_SPRINTF        =LIB_STEP*41,
	LIB_FLOAT          =LIB_STEP*42,
	LIB_FLOATFUNCS     =LIB_STEP*43,
	LIB_CREAD          =LIB_STEP*44,
	LIB_USEGRAPHIC     =LIB_STEP*45,
	LIB_GRAPHIC        =LIB_STEP*46,
	LIB_WIDTH          =LIB_STEP*47,
	LIB_FILE           =LIB_STEP*48,
	LIB_PLAYWAVE       =LIB_STEP*49,
	LIB_PLAYWAVEFUNC   =LIB_STEP*50,
	LIB_SETDIR         =LIB_STEP*51,
	LIB_SETDIRFUNC     =LIB_STEP*52,
	LIB_GETDIR         =LIB_STEP*53,
	LIB_READKEY        =LIB_STEP*54,
	LIB_DEBUG          =LIB_STEP*127,
};

// Note: OP_XXXX and FUNC_XXXX cannot be used simultaneously
#define FUNC_MASK 0x003F
#define FUNC_STEP 0x0001
enum functions{
	FUNC_FLOAT       =FUNC_STEP*0,
	FUNC_INT         =FUNC_STEP*1,
	FUNC_VALSHARP    =FUNC_STEP*2,
	FUNC_SIN         =FUNC_STEP*3,
	FUNC_COS         =FUNC_STEP*4,
	FUNC_TAN         =FUNC_STEP*5,
	FUNC_ASIN        =FUNC_STEP*6,
	FUNC_ACOS        =FUNC_STEP*7,
	FUNC_ATAN        =FUNC_STEP*8,
	FUNC_ATAN2       =FUNC_STEP*9,
	FUNC_SINH        =FUNC_STEP*10,
	FUNC_COSH        =FUNC_STEP*11,
	FUNC_TANH        =FUNC_STEP*12,
	FUNC_EXP         =FUNC_STEP*13,
	FUNC_LOG         =FUNC_STEP*14,
	FUNC_LOG10       =FUNC_STEP*15,
	FUNC_POW         =FUNC_STEP*16,
	FUNC_SQRT        =FUNC_STEP*17,
	FUNC_CEIL        =FUNC_STEP*18,
	FUNC_FLOOR       =FUNC_STEP*19,
	FUNC_FABS        =FUNC_STEP*20,
	FUNC_MODF        =FUNC_STEP*21,
	FUNC_FMOD        =FUNC_STEP*22,
	FUNC_PSET        =FUNC_STEP*23,
	FUNC_LINE        =FUNC_STEP*24,
	FUNC_BOXFILL     =FUNC_STEP*25,
	FUNC_CIRCLE      =FUNC_STEP*26,
	FUNC_CIRCLEFILL  =FUNC_STEP*27,
	FUNC_GPRINT      =FUNC_STEP*28,
	FUNC_PUTBMP      =FUNC_STEP*29,
	FUNC_PUTBMP2     =FUNC_STEP*30,
	FUNC_GCOLOR      =FUNC_STEP*31,
	FUNC_POINT       =FUNC_STEP*32,
	FUNC_FOPEN       =FUNC_STEP*33,
	FUNC_FOPENST     =FUNC_STEP*34,
	FUNC_FILE        =FUNC_STEP*35,
	FUNC_FCLOSE      =FUNC_STEP*36,
	FUNC_FINPUT      =FUNC_STEP*37,
	FUNC_FPRINTSTR   =FUNC_STEP*38,
	FUNC_FGET        =FUNC_STEP*39,
	FUNC_FPUT        =FUNC_STEP*40,
	FUNC_FSEEK       =FUNC_STEP*41,
	FUNC_FTELL       =FUNC_STEP*42,
	FUNC_FLEN        =FUNC_STEP*43,
	FUNC_FSTRING     =FUNC_STEP*44,
	FUNC_FGETC       =FUNC_STEP*45,
	FUNC_FPUTC       =FUNC_STEP*46,
	FUNC_FREMOVE     =FUNC_STEP*47,
	FUNC_FEOF        =FUNC_STEP*48,
	FUNC_FINIT       =FUNC_STEP*49,
	// MAX 63
};

/* Global vars (see globalvers.c) */
extern int g_intconst;
extern char g_valueisconst;
extern unsigned int g_rnd_seed;
extern unsigned int g_label;
extern int g_sdepth;
extern int g_maxsdepth;
extern char g_allow_shift_obj;
extern enum variable g_lastvar;
extern char* g_source;
extern int g_srcpos;
extern int g_line;
extern int g_fileline;
extern int* g_object;
extern int g_objpos;
extern int* g_objmax;
extern const char* g_err_str[];
extern const unsigned char g_priority[];
extern enum operator g_last_op;
extern int g_end_addr;
extern int g_gp;
extern int g_s6;
extern char RAM[RAMSIZE];
extern unsigned int g_ex_data[EXCEPTION_DATA_SIZE/4];
extern int g_var_mem[ALLOC_BLOCK_NUM];
extern unsigned short g_var_pointer[ALLOC_BLOCK_NUM];
extern unsigned short g_var_size[ALLOC_BLOCK_NUM];
extern char g_temp_area_used;
extern char g_option_nolinenum;
extern int* g_heap_mem;
extern int g_max_mem;
extern char g_disable_break;
extern unsigned char* g_pcg_font;
extern char g_use_graphic;
extern unsigned short* g_graphic_area;
extern int* g_libparams;
extern int g_long_name_var_num;
extern char g_music_active;
extern int g_class;
extern int g_compiling_class;
extern unsigned char g_num_classes;
extern char g_option_fastfield;
extern int g_temp;

/* Prototypes */
int get_gp(void);
int get_fp(void);
void start_program(void* addr, void* memory);
void shift_obj(int* src, int* dst, int len);
char* compile_line(void);
int nextCodeIs(char* str);
int endOfStatement();

char* init_file(char* buff,char* appname);
void close_file();
int filepoint();
void read_file(int blocklen);
char* compile_file();
int compile_and_link_file(char* buff,char* appname);
int compile_and_link_main_file(char* buff,char* appname);
int compile_and_link_class(char* buff,int class);

void err_break(void);
void err_music(char* str);
void err_data_not_found(void);
void err_str_complex(void);
void err_label_not_found(void);
void err_no_mem(void);
void err_div_zero(void);
void err_unkonwn(void);
void err_unexp_next(void);
void err_no_block(void);
void err_invalid_param(void);
void err_file(void);
void err_wave(void);
void err_not_obj(void);
void err_not_field(int fieldname, int classname);
void err_str(char* str);
char* resolve_label(int s6);

void musicint();
void set_sound(unsigned long* data, int flagsLR);
int musicRemaining(int flagsLR);
int waveRemaining(int mode);
void set_music(char* str, int flagsLR);
void stop_music(void);
void init_music(void);
void play_wave(char* filename, int start);

char* statement(void);
char* gosub_statement();
char* graphic_statement(enum functions func);
char* fopen_statement_main(enum functions func);
char* fget_statement();
char* fput_statement();
char* fputc_statement();
char* fremove_statement();
char* label_statement();
char* exec_statement();

char* function(void);
char* str_function(void);
char* float_function(void);

void call_library(void);
void reset_dataread();

void free_temp_str(char* str);
void free_non_temp_str(char* str);
void free_perm_str(char* str);
void* alloc_memory(int size, int var_num);
void* calloc_memory(int size, int var_num);
void move_to_perm_block(int var_num);
void move_from_perm_block(int var_num);
int move_from_perm_block_if_exists(int var_num);
int get_permanent_var_num(void);
int get_varnum_from_address(void* address);
void* lib_calloc_memory(int size);
void lib_delete(int* object);

char* link(void);
char* get_label(void);
void* search_label(unsigned int label);

char* get_string();
char* simple_string(void);

char* get_operator(void);
char* get_floatOperator(void);
char* calculation(enum operator op);
char* calculation_float(enum operator op);
int lib_float(int ia0,int iv0, enum operator a1);

int lib_file(enum functions func, int a0, int a1, int v0);

char* get_dim_value(int i);
char* get_simple_value(void);
char* get_value();
char* get_floatOrValue();
char* get_stringFloatOrValue();

void blue_screen(void);

char* get_float();

void cmpdata_init();
unsigned short cmpdata_get_id();
char* cmpdata_insert(unsigned char type, short data16, int* data, unsigned char num);
void cmpdata_reset();
int* cmpdata_find(unsigned char type);
int* cmpdata_findfirst(unsigned char type);
void cmpdata_delete(int* record);

int check_var_name();
int get_var_number();
int search_var_name(int nameint);
char* register_var_name(int nameint);

char* update_class_info(int class);
char* construct_class_structure(int class);
void delete_cmpdata_for_class(int class);

extern const unsigned int g_initial_s5_stack[3];
char* prepare_args_stack(char start_char);
char* remove_args_stack(void);
char* args_function_main(void);

char* begin_compiling_class(int class);
char* end_compiling_class(int class);
char* new_function();
char* field_statement();
char* integer_obj_field();
char* string_obj_field();
char* float_obj_field();
int lib_obj_field(int* object, int fieldname);
int lib_pre_method(int* object, int methodname);
int lib_post_method(int* object, int v0);
int lib_save_vars_to_fields(int* object,int v0);
int lib_load_vars_from_fields(int* object, int v0);

char* method_statement();
char* delete_statement();
char* call_statement();
void lib_let_str_field(char* str, char* prev_str);
char* let_object_field();
char* static_statement();
char* static_method(char type);
char* resolve_unresolved(int class);

void init_timer();
void stop_timer();
char* usetimer_statement();
char* timer_statement();
char* timer_function();
char* coretimer_statement();
char* coretimer_function();
char* interrupt_statement();

/* Error messages */
#define ERR_SYNTAX (char*)(g_err_str[0])
#define ERR_NE_BINARY (char*)(g_err_str[1])
#define ERR_NE_MEMORY (char*)(g_err_str[2])
#define ERR_DIV_0 (char*)(g_err_str[3])
#define ERR_NY_I (char*)(g_err_str[4])
#define ERR_LABEL_NF (char*)(g_err_str[5])
#define ERR_LABEL_LONG (char*)(g_err_str[6])
#define ERR_STR_COMPLEX (char*)(g_err_str[7])
#define ERR_DATA_NF (char*)(g_err_str[8])
#define ERR_UNKNOWN (char*)(g_err_str[9])
#define ERR_MUSIC (char*)(g_err_str[10])
#define ERR_MULTIPLE_LABEL (char*)(g_err_str[11])
#define ERR_BREAK (char*)(g_err_str[12])
#define ERR_UNEXP_NEXT (char*)(g_err_str[13])
#define ERR_NO_BLOCK (char*)(g_err_str[14])
#define ERR_GOSUB_ASH (char*)(g_err_str[15])
#define ERR_INVALID_BREAK (char*)(g_err_str[16])
#define ERR_INVALID_ELSEIF (char*)(g_err_str[17])
#define ERR_INVALID_PARAM (char*)(g_err_str[18])
#define ERR_FILE (char*)(g_err_str[19])
#define ERR_INVALID_VAR_NAME (char*)(g_err_str[20])
#define ERR_WAVE (char*)(g_err_str[21])
#define ERR_COMPILE_CLASS (char*)(g_err_str[22])
#define ERR_NO_CLASS (char*)(g_err_str[23])
#define ERR_NOT_OBJ (char*)(g_err_str[24])
#define ERR_NOT_FIELD (char*)(g_err_str[25])
#define ERR_INVALID_NON_CLASS (char*)(g_err_str[26])
#define ERR_INVALID_CLASS (char*)(g_err_str[27])
#define ERR_NO_INIT (char*)(g_err_str[28])
#define ERR_OPTION_CLASSCODE (char*)(g_err_str[29])

/* compile data type numbers */
#define CMPDATA_RESERVED  0
#define CMPDATA_USEVAR    1
#define CMPDATA_CLASS     2
#define CMPDATA_FIELD     3
#define CMPDATA_STATIC    4
#define CMPDATA_UNSOLVED  5
#define CMPDATA_TEMP      6
#define CMPDATA_FASTFIELD 7
// Sub types follow
#define CMPTYPE_PUBLIC_FIELD 0
#define CMPTYPE_PRIVATE_FIELD 1
#define CMPTYPE_PUBLIC_METHOD 2
#define CMPTYPE_NEW_FUNCTION 0
#define CMPTYPE_STATIC_METHOD 1


/* Stack position for values in args.c */
#define ARGS_SP_SP       4
#define ARGS_SP_V0_OBJ   8
#define ARGS_SP_PREV_S5  12
#define ARGS_SP_NUM_ARGS 16
#define ARGS_S5_SP       (-12 & 0xFFFF)
#define ARGS_S5_V0_OBJ   (-8  & 0xFFFF)
#define ARGS_S5_PREV_S5  (-4  & 0xFFFF)
#define ARGS_S5_NUM_ARGS (0   & 0xFFFF)

/*
	Hidden varname 31 bit values
	Note that max number of 31 bit value is 0x61504BFF (for ZZZZZZ)
*/
#define HIDDEN_VAR_THIS_OBJECT 0x7FFF0000

/* Macros */

// Lables as 31 bit integer
#define LABEL_INIT 0x0008727b

// Skip blanc(s) in source code
#define next_position() while(g_source[g_srcpos]==' ') {g_srcpos++;}

// Check if object area is not full.
#define check_obj_space(x) if (g_objmax<g_object+g_objpos+(x)) return ERR_NE_BINARY

// Returns priority of operator
#define priority(x) (int)g_priority[(int)(x)]

// Insert code for calling library
//02E0F809   jalr        ra,s7
//24070000   addiu       a3,zero,0000
#define call_lib_code(x) \
	check_obj_space(2);\
	g_object[g_objpos++]=0x02E0F809;\
	g_object[g_objpos++]=0x24070000|((x)&0x0000FFFF)

// Insert code for calling quick library
//3C081234   lui         t0,0x1234
//35085678   ori         t0,t0,0x5678
//0100F809   jalr        ra,t0
//00000000   nop         
#define call_quicklib_code(x,y) do {\
		check_obj_space(4);\
		g_object[g_objpos++]=0x3C080000|(((unsigned int)(x))>>16);\
		g_object[g_objpos++]=0x35080000|(((unsigned int)(x))&0x0000FFFF);\
		g_object[g_objpos++]=0x0100F809;\
		g_object[g_objpos++]=(y);\
	} while (0)	

#define ASM_NOP 0x00000000
#define ASM_ADDU_A0_V0_ZERO 0x00402021
#define ASM_ADDU_A1_V0_ZERO 0x00402821
#define ASM_ADDU_A2_V0_ZERO 0x00403021
#define ASM_ADDU_A3_V0_ZERO 0x00403821
#define ASM_ORI_A0_ZERO_ 0x34040000
#define ASM_LW_A0_XXXX_S8 0x8FC40000
#define ASM_LW_A0_XXXX_S5 0x8EA40000

// Interrupt macros
// 32 different type interruptions are possible
// See also envspecific.h for additional interruptions
#define INTERRUPT_TIMER     0
#define INTERRUPT_DRAWCOUNT 1
#define INTERRUPT_KEYS      2
#define INTERRUPT_INKEY     3
#define INTERRUPT_MUSIC     4
#define INTERRUPT_WAVE      5
#define INTERRUPT_CORETIMER 6

extern int g_interrupt_flags;
extern int g_int_vector[];
#define raise_interrupt_flag(x) do {\
	if (g_int_vector[x]) {\
		IFS0bits.CS1IF=1;\
		g_interrupt_flags|=(1<<(x));\
	}\
} while(0)

// Division macro for unsigned long
// Valid for 31 bits for all cases and 32 bits for some cases
#define div32(x,y,z) ((((unsigned long long)((unsigned long)(x)))*((unsigned long long)((unsigned long)(y))))>>(z))

// Divide by 8 (valid for 32 bits)
#define div8_32(x) (((unsigned long)(x))>>3)
#define rem8_32(x) ((x)&0x07)

// Divide by 9 (valid for 32 bits)
#define div9_32(x) div32(x,0xe38e38e4,35)
#define rem9_32(x) ((x)-9*div9_32(x))

// Divide by 10 (valid for 32 bits)
#define div10_32(x) div32(x,0xcccccccd,35)
#define rem10_32(x) ((x)-10*div10_32(x))

// Divide by 36 (valid for 32 bits)
#define div36_32(x) div32(x,0xe38e38e4,37)
#define rem36_32(x) (x-36*div36_32(x))

// Divide by 37 (valid for 31 bits)
#define div37_31(x) div32(x,0xdd67c8a7,37)
#define rem37_31(x) (x-37*div37_31(x))

// Check if within RAM
#define withinRAM(x) ((&RAM[0])<=((char*)(x)) && ((char*)(x))<(&RAM[RAMSIZE]))
