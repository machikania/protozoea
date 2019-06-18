/*
   This file is provided under the LGPL license ver 2.1.
   Written by Katsumi.
   http://hp.vector.co.jp/authors/VA016157/
   kmorimatsu@users.sourceforge.jp
*/

/*
	This file is shared by Megalopa and Zoea
*/

#include <xc.h>
#include "main.h"
#include "compiler.h"
#include "api.h"
#include "interface/keyinput.h"
#include "stdlib.h"
#include "math.h"

/*
   Local global variables used for graphic
 */

static int g_gcolor=7;
static int g_prev_x=0;
static int g_prev_y=0;

int lib_setdir(int mode,char* path){
	int ret;
	ret=FSchdir(path);
	if (mode==LIB_SETDIR && ret) err_file();
	return ret;
}

int lib_getdir(){
	char* path;
	path=calloc_memory(32,-1);
	FSgetcwd (path,128);
	return (int)path;
}

int lib_read(int mode, unsigned int label){
	unsigned int i,code,code2;
	static unsigned int pos=0;
	static unsigned int in_data=0;
	static unsigned char skip=0;
	if (label) {
		// RESTORE function
		switch(mode){
			case 0:
				// label is label data
				i=(int)search_label(label);
				if (!i) {
					err_data_not_found();
					return 0;
				}
				break;
			case 1:
				// label is pointer
				i=label;
				break;
			case 2:
			default:
				// Reset data/read
				pos=0;
				in_data=0;
				skip=0;
				return 0;
		}
		i-=(int)(&g_object[0]);
		pos=i/4;
		in_data=0;
	}
	// Get data
	if (in_data==0) {
		for(i=pos;i<g_objpos;i++){
			code=g_object[i];
			code2=g_object[i+1];
			if ((code&0xFFFF0000)!=0x04110000) continue;
			// "bgezal zero," assembly found.
			// Check if 0x00000020,0x00000021,0x00000022, or 0x00000023 follows
			if ((code2&0xfffffffc)!=0x00000020) {// add/addu/sub/subu        zero,zero,zero
				// If not, skip following block (it's strig).
				i+=code&0x0000FFFF;
				continue;
			}
			// DATA region found.
			in_data=(code&0x0000FFFF)-1;
			pos=i+2;
			skip=code2&0x03;
			break;
		}
		if (g_objpos<=i) {
			err_data_not_found();
			return 0;
		}
	}
	if (label) {
		// RESTORE function. Return pointer.
		return ((int)&g_object[pos])+skip;
	} else {
		switch(mode){
			case 0:
				// READ() function
				in_data--;
				return g_object[pos++];
			case 1:
			default:
				// CREAD() function
				i=g_object[pos];
				i>>=skip*8;
				i&=0xff;
				if ((++skip)==4) {
					skip=0;
					in_data--;
					pos++;
				}
				return i;
		}
	}
}

void reset_dataread(){
	lib_read(2,1);
}

char* lib_midstr(int var_num, int pos, int len){
	int i;
	char* str;
	char* ret;
	if (0<=pos) {
		// String after "pos" position.
		str=(char*)(g_var_mem[var_num]+pos);
	} else {
		// String right "pos" characters.
		// Determine length
		str=(char*)g_var_mem[var_num];
		for(i=0;str[i];i++);
		if (0<=(i+pos)) {
			str=(char*)(g_var_mem[var_num]+i+pos);
		}
	}
	if (len<0) {
		// Length is not specified.
		// Return the string to the end.
		return str;
	}
	// Length is specified.
	// Construct temporary string containing specified number of characters.
	ret=alloc_memory((len+1+3)/4,-1);
	// Copy string.
	for(i=0;(ret[i]=str[i])&&(i<len);i++);
	ret[len]=0x00;
	return ret;
}

void lib_clear(void){
	int i;
	// All variables (including temporary and permanent ones) will be integer 0
	for(i=0;i<ALLOC_BLOCK_NUM;i++){
		g_var_mem[i]=0;
	}
	// Clear memory allocation area
	for(i=0;i<ALLOC_BLOCK_NUM;i++){
		g_var_size[i]=0;
	}
	// Cancel PCG
	stopPCG();
	g_pcg_font=0;
	// Stop using graphic
	g_use_graphic=1; // Force set_graphmode(0) (see usegraphic() function)
	usegraphic(0);
}

void lib_let_str(char* str, int var_num){
	int begin,end,size;
	// Save pointer
	g_var_mem[var_num]=(int)str;
	// Determine size
	for(size=0;str[size];size++);
	// Check if str is in heap area.
	begin=(int)str;
	end=(int)(&str[size]);
	if (begin<(int)(&g_heap_mem[0]) || (int)(&g_heap_mem[g_max_mem])<=end) {
		// String is not within allcated block
		return;
	}
	// Str is in heap area. Calculate values stored in heap data dimension
	begin-=(int)(&g_heap_mem[0]);
	begin>>=2;
	end-=(int)(&g_heap_mem[0]);
	end>>=2;
	size=end-begin+1;
	g_var_pointer[var_num]=begin;
	g_var_size[var_num]=size;
}

int lib_rnd(){
	int y;
	y=g_rnd_seed;
	y = y ^ (y << 13);
	y = y ^ (y >> 17);
	y = y ^ (y << 5);
	g_rnd_seed=y;
	return y&0x7fff;
}

char* lib_chr(int num){
	char* str;
	str=alloc_memory(1,-1);
	str[0]=num&0x000000FF;
	str[1]=0x00;
	return str;
}

char* lib_dec(int num){
	char* str;
	int i,j,minus;
	char b[12];
	b[11]=0x00;
	if (num<0) {
		minus=1;
		num=0-num;
	} else {
		minus=0;
	}
	for (i=10;0<i;i--) {
		if (num==0 && i<10) break; 
		b[i]='0'+rem10_32(num);
		num=div10_32(num);
	}
	if (minus) {
		b[i]='-';
	} else {
		i++;
	}
	str=alloc_memory(3,-1);
	for(j=0;str[j]=b[i++];j++);
	return str;
}

char* lib_hex(int num, int width){
	char* str;
	int i,j,minus;
	char b[8];
	str=alloc_memory(3,-1);
	for(i=0;i<8;i++){
		b[i]="0123456789ABCDEF"[(num>>(i<<2))&0x0F];
	}
	// Width must be between 0 and 8;
	if (width<0||8<width) width=8;
	if (width==0) {
		// Width not asigned. Use minimum width.
		for(i=7;0<i;i--){
			if ('0'<b[i]) break;
		}
	} else {
		// Constant width
		i=width-1;
	}
	// Copy string to allocated block.
	for(j=0;0<=i;i--){
		str[j++]=b[i];
	}
	str[j]=0x00;
	return str;
}

char* lib_connect_string(char* str1, char* str2){
	int i,j;
	char b;
	char* result;
	// Determine total length
	for(i=0;str1[i];i++);
	for(j=0;str2[j];j++);
	// Allocate a block for new string
	result=alloc_memory((i+j+1+3)/4,-1);
	// Create connected strings 
	for(i=0;b=str1[i];i++) result[i]=b;
	for(j=0;b=str2[j];j++) result[i+j]=b;
	result[i+j]=0x00;
	free_temp_str(str1);
	free_temp_str(str2);
	return result;
}

void lib_string(int mode){
	int i;
	switch(mode){
		case 0:
			// CR
			printchar('\n');
			return;
		case 1:
			// ,
			printcomma();
			return;
		default:
			return;
	}
}

void* lib_label(unsigned int label){
	// This routine is used to jump to address dynamically determined
	// in the code; for example: "GOTO 100+I"
	unsigned int i,code,search;
	void* ret;  
	if (label&0xFFFF0000) {
		// Label is not supported.
		// Line number must bs less than 65536.
		err_label_not_found();
	} else {
		// Line number
		ret=search_label(label);
		if (ret) return ret;
		// Line number not found.
		err_label_not_found();
	}
}

int lib_keys(int mask){
	int keys;
	keys=readbuttons();
	keys=
		((keys&KEYUP)?    0:1)|
		((keys&KEYDOWN)?  0:2)|
		((keys&KEYLEFT)?  0:4)|
		((keys&KEYRIGHT)? 0:8)|
		((keys&KEYSTART)? 0:16)|
		((keys&KEYFIRE)?  0:32);
	return mask&keys;
}

int lib_val(char* str){
	int i;
	int val=0;
	int sign=1;
	char b;
	// Skip blanc
	for(i=0;0<=str[i] && str[i]<0x21;i++);
	// Skip '+'
	if (str[i]=='+') i++;
	// Check '-'
	if (str[i]=='-') {
		sign=-1;
		i++;
	}
	// Check '0x' or '$'
	if (str[i]=='$' || str[i]=='0' && (str[i+1]=='x' || str[i+1]=='X')) {
		// Hexadecimal
		if (str[i++]=='0') i++;
		while(1) {
			b=str[i++];
			if ('0'<=b && b<='9') {
				val<<=4;
				val+=b-'0';
			} else if ('a'<=b && b<='f') {
				val<<=4;
				val+=b-'a'+10;
			} else if ('A'<=b && b<='F') {
				val<<=4;
				val+=b-'A'+10;
			} else {
				break;
			}
		}
	} else {
		// Decimal
		while(1) {
			b=str[i++];
			if ('0'<=b && b<='9') {
				val*=10;
				val+=b-'0';
			} else {
				break;
			}
		}
	}
	return val*sign;
}

char* lib_input(){
	// Allocate memory for strings with 63 characters
	char *str=calloc_memory((63+1)/4,-1);
	// Enable PS/2 keyboard
	if (!inPS2MODE()) {
		ps2mode();
		ps2init();
	}
	// Clear key buffer
	do ps2readkey();
	while(vkey!=0);
	// Get string as a line
	lineinput(str,63);
	check_break();
	return str;
}

unsigned char lib_inkey(int key){
	int i;
	// Enable PS/2 keyboard
	if (!inPS2MODE()) {
		ps2mode();
		ps2init();
	}
	if (key) {
		return ps2keystatus[key&0xff];
	} else {
		for(i=0;i<256;i++){
			if (ps2keystatus[i]) return i;
		}
		return 0;
	}
}

void lib_usepcg(int mode){
	// Modes; 0: stop PCG, 1: use PCG, 2: reset PCG and use it
	switch(mode){
		case 0:
			// Stop PCG
			stopPCG();
			break;
		case 2:
			// Reset PCG and use it
			if (g_pcg_font) {
				free_non_temp_str(g_pcg_font);
				g_pcg_font=0;
			}
			// Continue to case 1:
		case 1:
		default:
			// Use PCG
			if (g_pcg_font) {
				startPCG(g_pcg_font,0);
			} else {
				g_pcg_font=alloc_memory(256*8/4,ALLOC_PCG_BLOCK);
				startPCG(g_pcg_font,1);
			}
			break;
	}
}

void lib_pcg(unsigned int ascii,unsigned int fontdata1,unsigned int fontdata2){
	unsigned int* pcg;
	// If USEPCG has not yet executed, do now.
	if (!g_pcg_font) lib_usepcg(1);
	pcg=(unsigned int*)g_pcg_font;
	// 0 <= ascii <= 0xff
	ascii&=0xff;
	// Update font data
	ascii<<=1;
	pcg[ascii]=(fontdata1>>24)|((fontdata1&0xff0000)>>8)|((fontdata1&0xff00)<<8)|(fontdata1<<24);
	pcg[ascii+1]=(fontdata2>>24)|((fontdata2&0xff0000)>>8)|((fontdata2&0xff00)<<8)|(fontdata2<<24);
}

void lib_usegraphic(int mode){
	usegraphic(mode);
	// Move current point to (0,0)
	g_prev_x=g_prev_y=0;
}
void lib_wait(int period){
	int i;
	unsigned short dcount;
	for(i=0;i<period;i++){
		dcount=drawcount;
		while(dcount==drawcount){
			asm (WAIT);
			check_break();
		}
	}
}

int lib_graphic(int v0,enum functions func){
	unsigned char b;
	int x1=g_libparams[1];
	int y1=g_libparams[2];
	int x2=g_libparams[3];
	int y2=g_libparams[4];
	// Disable if graphic area is not defined.
	if (!g_graphic_area) return;
	// If C is omitted in parameters, use current color.
	if (v0==-1) {
		v0=g_gcolor;
	}
	// If X1 or Y1 is 0x80000000, use the previous values.
	if (x1==0x80000000) x1=g_prev_x;
	if (y1==0x80000000) y1=g_prev_y;
	switch(func){
		case FUNC_POINT:// X1,Y1
			g_prev_x=x1;
			g_prev_y=y1;
			break;
		case FUNC_PSET:// X1,Y1[,C]
			g_pset(x1,y1,v0);
			g_prev_x=x1;
			g_prev_y=y1;
			break;
		case FUNC_LINE:// X1,Y1,X2,Y2[,C]
			if (y1==y2) g_hline(x1,x2,y1,v0);
			else g_gline(x1,y1,x2,y2,v0);
			g_prev_x=x2;
			g_prev_y=y2;
			break;
		case FUNC_BOXFILL:// X1,Y1,X2,Y2[,C]
			g_boxfill(x1,y1,x2,y2,v0);
			g_prev_x=x2;
			g_prev_y=y2;
			break;
		case FUNC_CIRCLE:// X1,Y1,R[,C]
			g_circle(x1,y1,x2,v0);
			g_prev_x=x1;
			g_prev_y=y1;
			break;
		case FUNC_CIRCLEFILL:// X1,Y1,R[,C]
			g_circlefill(x1,y1,x2,v0);
			g_prev_x=x1;
			g_prev_y=y1;
			break;
		case FUNC_GPRINT:// X1,Y1,C,BC,S$
			g_printstr(x1,y1,x2,y2,(unsigned char*)v0);
			// Move current X,Y according to the string
			while(b=((unsigned char*)v0)[0]){
				v0++;
				if (b==0x0d) {
					x1=0;
					y1+=8;
				} else {
					x1+=8;
				}
			}
			g_prev_x=x1;
			g_prev_y=y1;
			break;
		case FUNC_PUTBMP2:// X1,Y1,M,N,BMP(label)
			// Search CDATA
			// It starts from either 0x00000020,0x00000021,0x00000022, or 0x00000023.
			while((((unsigned int*)v0)[0]&0xfffffffc)!=0x00000020) v0+=4;
			// CDATA starts from next word.
			// MLB 3 bytes show skip byte(s).
			v0+=4+(((unsigned int*)v0)[0]&0x03);
			// Contunue to FUNC_PUTBMP.
		case FUNC_PUTBMP:// X1,Y1,M,N,BMP(pointer)
			g_putbmpmn(x1,y1,x2,y2,(const unsigned char*)v0);
			g_prev_x=x1;
			g_prev_y=y1;
			break;
		case FUNC_GCOLOR:// (X1,Y1)
			v0=g_color(x1,y1);
			break;
		default:
			break;
	}
	return v0;
}

void lib_var_push(int a0, int a1, int* sp){
	// Note that sp[1] is used for string return address
	// sp[2] can be used to store flags
	// sp[3] etc can be used to store variable values
	int i,params;
	unsigned char varnum;
	unsigned int strflags=0;
	int stack=3;
	for(i=0;i<8;i++){
		// Prepare parameter
		switch(i){
			case 0:
				params=a0;
				break;
			case 4:
				params=a1;
				break;
			default:
				break;
		}
		// Get variable number
		varnum=params&0xff;
		params>>=8;
		if (varnum==0) break; // No more variable. End the loop.
		varnum--;
		sp[stack++]=g_var_mem[varnum];
		if (g_var_size[varnum] && g_var_mem[varnum]==(int)(&g_var_pointer[varnum])) {
			// strflags change using varnum
			strflags|=1<<i;
			// Copy to VAR_BLOCK
			move_to_perm_block(varnum);
		}
		// Clear variable
		g_var_mem[varnum]=0;
	}
	// Store string flags
	sp[2]=strflags;
}

void lib_var_pop(int a0, int a1, int* sp){
	// Note that sp is 4 bytes larger than that in lib_var_push
	// sp[1] was used to store flags
	// sp[2] etc can be used to store variable values
	int i,params;
	unsigned char varnum;
	int stack=2;
	unsigned int strflags=sp[1];
	for(i=0;i<8;i++){
		// Prepare parameter
		switch(i){
			case 0:
				params=a0;
				break;
			case 4:
				params=a1;
				break;
			default:
				break;
		}
		// Get variable number
		varnum=params&0xff;
		params>>=8;
		if (varnum==0) break; // No more variable. End the loop.
		varnum--;
		g_var_mem[varnum]=sp[stack++];
		if (strflags&(1<<i)) {
			// Restore from VAR_BLOCK
			move_from_perm_block(varnum);
		}
	}
}


char* lib_sprintf(char* format, int data){
	char* str;
	int i;
	char temp[4];
	if (!format) format="%g";
	i=snprintf((char*)(&temp[0]),4,format,data)+1;
	str=alloc_memory((i+3)/4,-1);
	snprintf(str,i,format,data);
	return str;
}

int lib_floatfuncs(int ia0,int iv0,enum functions a1){
	volatile float a0,v0;
	((int*)(&a0))[0]=ia0;
	((int*)(&v0))[0]=iv0;
	switch(a1){
		case FUNC_FLOAT:
			v0=(float)iv0;
			break;
		case FUNC_INT:
			return (int)v0;
		case FUNC_VALSHARP:
			v0=strtof((const char*)iv0,0);
			break;
		case FUNC_SIN:
			v0=sinf(v0);
			break;
		case FUNC_COS:
			v0=cosf(v0);
			break;
		case FUNC_TAN:
			v0=tanf(v0);
			break;
		case FUNC_ASIN:
			v0=asinf(v0);
			break;
		case FUNC_ACOS:
			v0=acosf(v0);
			break;
		case FUNC_ATAN:
			v0=atanf(v0);
			break;
		case FUNC_ATAN2:
			v0=atan2f(v0,a0);
			break;
		case FUNC_SINH:
			v0=sinhf(v0);
			break;
		case FUNC_COSH:
			v0=coshf(v0);
			break;
		case FUNC_TANH:
			v0=tanhf(v0);
			break;
		case FUNC_EXP:
			v0=expf(v0);
			break;
		case FUNC_LOG:
			v0=logf(v0);
			break;
		case FUNC_LOG10:
			v0=log10f(v0);
			break;
		case FUNC_POW:
			v0=powf(v0,a0);
			break;
		case FUNC_SQRT:
			v0=sqrtf(v0);
			break;
		case FUNC_CEIL:
			v0=ceilf(v0);
			break;
		case FUNC_FLOOR:
			v0=floorf(v0);
			break;
		case FUNC_FABS:
			v0=fabsf(v0);
			break;
		case FUNC_MODF:
			v0=modff(v0,(void*)&a0);
			break;
		case FUNC_FMOD:
			v0=fmodf(v0,a0);
			break;
		default:
			err_unknown();
			break;
	}
	return ((int*)(&v0))[0];
};

int* lib_dim(int varnum, int argsnum, int* sp){
	int i,j;
	static int* heap;
	// Calculate total length.
	int len=0;  // Total length
	int size=1; // Size of current block
	for(i=1;i<=argsnum;i++){
		size*=sp[i]+1;
		len+=size;
	}
	// Allocate memory
	heap=calloc_memory(len,varnum);
	// Construct pointers
	len=0;
	size=1;
	for(i=1;i<argsnum;i++){
		size*=sp[i]+1;
		for(j=0;j<size;j++){
			heap[len+j]=(int)&heap[len+size+(sp[i+1]+1)*j];
		}
		len+=size;
	}
	return heap;
};

int lib_file_textlen(FSFILE* fhandle){
	char buff[128];
	int i,textlen,len,seek;
	seek=FSftell(fhandle);
	len=FSfread(&buff[0],1,128,fhandle);
	textlen=0;
	for(i=0;i<len-1;i++){ // Read 127 bytes for supporting CRLF
		if (buff[i]==0x0d) {
			if (i<len && buff[i+1]==0x0a) i++;
			break;
		} else if (buff[i]==0x0a) {
			break;
		}
		if (i==len-2) {
			// reached the end of buffer. Read next 127 bytes
			textlen+=127;
			buff[0]=buff[127];
			len=FSfread(&buff[1],1,127,fhandle);
			// Continue with i=0
			i=-1;
		}
	}
	// The last return code must be included to caluclate total length.
	textlen+=i+1;
	// Return to original position
	FSfseek(fhandle,seek,SEEK_SET);
	return textlen;
}

int lib_file(enum functions func, int a0, int a1, int v0){
	static FSFILE* s_fhandle[2]={0,0};
	static char activefhandle=0;
	static int numinline=0;
	FSFILE* fhandle=0;
	int i;
	int buff[1];
	char* str;
	if (activefhandle) fhandle=s_fhandle[activefhandle-1];
	switch(func){
		case FUNC_FINIT:
			// This function is not BASIC statement/function but used from
			// running routine. 
			for(i=0;i<2;i++){
				if (s_fhandle[i]) FSfclose(s_fhandle[i]);
				s_fhandle[i]=0; 
			}
			activefhandle=0;
			numinline=0;
			break;
		case FUNC_FOPEN:   // Return 0 when called as a function.
		case FUNC_FOPENST: // Stop with error when called as a statement.
			activefhandle=0;
			// Check if file handle is free to use, first.
			switch(v0){
				case 0:
					// File handle was not designated
					// Force handle=1 and continue to following cases.
					v0=1;
				case 1:
				case 2:
					// File handle was designated
					// Check if not used yet.
					if (s_fhandle[v0-1]) {
						// This file handle has been occupied.
						err_file();
						return 0;
					}
					// OK. This file handle can be asigned for new file opened.
					break;
				default:
					err_invalid_param();
					return 0;
			}
			// Open a file
			fhandle=FSfopen ((const char*) a0, (const char*) a1);
			if (!fhandle) {
				if (func==FUNC_FOPENST) err_file();
				return 0;
			}
			// The file is succesfully opened. Asign file handle.
			s_fhandle[v0-1]=fhandle;
			activefhandle=v0;
			return v0;
		case FUNC_FILE:
			switch(v0){
				case 1:
				case 2:
					if (s_fhandle[v0]) {
						activefhandle=v0;
						break;
					}
				default:
					err_invalid_param();
			}
			break;
		case FUNC_FCLOSE:
			switch(v0){
				case 0:
					break;
				case 1:
				case 2:
					if (s_fhandle[v0-1]) activefhandle=v0;
					if (activefhandle) fhandle=s_fhandle[activefhandle-1];
					break;
				default:
					err_invalid_param();
			}
			if (fhandle) {
				FSfclose(fhandle);
				s_fhandle[activefhandle-1]=0;
			}
			activefhandle=0;
			break;	
		case FUNC_FINPUT:
			if (fhandle) {
				// Determine text length if called without parameter
				if (v0==0) v0=lib_file_textlen(fhandle);
				// Allocate temporary area for string
				str=alloc_memory((v0+1+3)/4,-1);
				// Read from SD card
				v0=FSfread(str,1,v0,fhandle);
				// Null string at the end.
				str[v0]=0;
				return (int)str;
			} else {
				err_file();
				return (int)"";
			}
		case FUNC_FPRINTSTR:
			// Like lib_printstr()
			for(i=0;((char*)v0)[i];i++);
			if (fhandle) {
				if (!FSfwrite((char*)v0,1,i,fhandle)) err_file();
			} else err_file();
			numinline+=i;
			break;
		case FUNC_FSTRING:
			// Like lib_string()
			switch(v0){
				case 0:
					// CR
					lib_file(FUNC_FPRINTSTR,a0,a1,(int)"\r\n");
					numinline=0;
					break;
				case 1:
					// ,
					i=rem10_32(numinline);
					lib_file(FUNC_FPRINTSTR,a0,a1,(int)("          "+i));
					break;
				default:
					break;
			}
			break;
		case FUNC_FGET:
			if (fhandle) return FSfread((void*)a0,1,v0,fhandle);
			err_file();
			break;
		case FUNC_FPUT:
			if (fhandle) return FSfwrite((void*)a0,1,v0,fhandle);
			err_file();
			break;
		case FUNC_FGETC:
			if (fhandle) {
				// Note: Little endian.
				if (FSfread((void*)&buff[0],1,1,fhandle)) return buff[0]&0xff;
				else return -1;
			}
			err_file();
			break;
		case FUNC_FPUTC:
			if (fhandle) {
				// Note: Little endian.
				buff[0]=v0;
				return FSfwrite((void*)&buff[0],1,1,fhandle);
			}
			err_file();
			break;
		case FUNC_FSEEK:
			if (fhandle) return FSfseek(fhandle,v0,SEEK_SET);
			err_file();
			break;
		case FUNC_FTELL:
			if (fhandle) return FSftell(fhandle);
			err_file();
			break;
		case FUNC_FLEN:
			if (fhandle) return fhandle->size;
			err_file();
			break;
		case FUNC_FEOF:
			if (fhandle) return (fhandle->size<=FSftell(fhandle)) ? 1:0;
			err_file();
			break;
		case FUNC_FREMOVE:
			return FSremove((const char *)v0);
		default:
			err_unknown();
	}
	return v0;
}

int lib_readkey(){
	int ret=ps2readkey();
	return ret|(vkey<<8);
}

int _call_library(int a0,int a1,int a2,enum libs a3);

void call_library(void){
	// Store s6 in g_s6
	asm volatile("la $a2,%0"::"i"(&g_s6));
	asm volatile("sw $s6,0($a2)");
	// Copy $v0 to $a2 as 3rd argument of function
	asm volatile("addu $a2,$v0,$zero");
	// Store sp in g_libparams
	asm volatile("la $v0,%0"::"i"(&g_libparams));
	asm volatile("sw $sp,0($v0)");
	// Jump to main routine
	asm volatile("j _call_library");
}

int _call_library(int a0,int a1,int v0,enum libs a3){
	// usage: call_lib_code(LIB_XXXX);
	// Above code takes 2 words.
	check_break();
	switch(a3 & LIB_MASK){
		case LIB_FLOAT:
			return lib_float(a0,v0,(enum operator)(a3 & OP_MASK)); // see operator.c
		case LIB_FLOATFUNCS:
			return lib_floatfuncs(a0,v0,(enum functions)(a3 & FUNC_MASK));
		case LIB_STRNCMP:
			return strncmp((char*)g_libparams[1],(char*)g_libparams[2],v0);
		case LIB_MIDSTR:
			return (int)lib_midstr(a1,v0,a0);
		case LIB_RND:
			return (int)lib_rnd();
		case LIB_DEC:
			return (int)lib_dec(v0);
		case LIB_HEX:
			return (int)lib_hex(v0,a0);
		case LIB_CHR:
			return (int)lib_chr(v0);
		case LIB_VAL:
			return lib_val((char*)v0);
		case LIB_LETSTR:
			lib_let_str((char*)v0,a0);
			return;
		case LIB_CONNECT_STRING:
			return (int)lib_connect_string((char*)a0, (char*)v0);
		case LIB_STRING:
			lib_string(v0);
			return v0;
		case LIB_PRINTSTR:
			printstr((char*)v0);
			return v0;
		case LIB_GRAPHIC:
			return lib_graphic(v0, (enum functions)(a3 & FUNC_MASK));
		case LIB_SPRINTF:
			return (int)lib_sprintf((char*)v0,a0);
		case LIB_VAR_PUSH:
			lib_var_push(a0,a1,g_libparams);
			return v0;
		case LIB_VAR_POP:
			lib_var_pop(a0,a1,g_libparams);
			return v0;
		case LIB_SCROLL:
			scroll(g_libparams[1],v0);
			return v0;
		case LIB_FILE:
			return lib_file((enum functions)(a3 & FUNC_MASK),g_libparams[1],g_libparams[2],v0);
		case LIB_KEYS:
			return lib_keys(v0);
		case LIB_INKEY:
			return (int)lib_inkey(v0);
		case LIB_READKEY:
			return lib_readkey();
		case LIB_CURSOR:
			setcursor(g_libparams[1],v0,cursorcolor);
			return v0;
		case LIB_SOUND:
			set_sound((unsigned long*)v0,a0);
			return v0;
		case LIB_MUSICFUNC:
			return musicRemaining(a0);
		case LIB_MUSIC:
			set_music((char*)v0,a0);
			return v0;
		case LIB_PLAYWAVE:
			play_wave((char*)g_libparams[1],v0);
			return v0;
		case LIB_PLAYWAVEFUNC:
			return waveRemaining(v0);
		case LIB_SETDRAWCOUNT:
			drawcount=(v0&0x0000FFFF);
			return v0;
		case LIB_GETDIR:
			return lib_getdir();
		case LIB_SETDIRFUNC:
		case LIB_SETDIR:
			return lib_setdir(a3,(char*)v0);
		case LIB_DRAWCOUNT:
			return drawcount;
		case LIB_SYSTEM:
			return lib_system(a0, a1 ,v0, a3, g_gcolor, g_prev_x, g_prev_y);
		case LIB_RESTORE:
			return lib_read(0,v0);
		case LIB_RESTORE2:
			return lib_read(1,v0);
		case LIB_READ:
			return lib_read(0,0);
		case LIB_CREAD:
			return lib_read(1,0);
		case LIB_LABEL:
			return (int)lib_label(v0);
		case LIB_INPUT:
			return (int)lib_input();
		case LIB_USEGRAPHIC:
			lib_usegraphic(v0);
			return v0;
		case LIB_USEPCG:
			lib_usepcg(v0);
			return v0;
		case LIB_PCG:
			lib_pcg(g_libparams[1],g_libparams[2],v0);
			return v0;
		case LIB_BGCOLOR: // BGCOLOR R,G,B
			set_bgcolor(v0,g_libparams[1],g_libparams[2]); //set_bgcolor(b,r,g);
			return v0;
		case LIB_PALETTE: // PALETTE N,R,G,B
			set_palette(g_libparams[1],v0,g_libparams[2],g_libparams[3]); // set_palette(n,b,r,g);
			return v0;
		case LIB_GPALETTE:// GPALETTE N,R,G,B
			if (g_graphic_area) g_set_palette(g_libparams[1],v0,g_libparams[2],g_libparams[3]); // g_set_palette(n,b,r,g);
			return v0;
		case LIB_CLS:
			clearscreen();
			return v0;
		case LIB_GCLS:
			if (g_graphic_area) g_clearscreen();
			g_prev_x=g_prev_y=0;
			return v0;
		case LIB_WIDTH:
			videowidth(v0);
			return v0;
		case LIB_COLOR:
			setcursorcolor(v0);
			return v0;
		case LIB_GCOLOR:
			g_gcolor=v0;
			return v0;
		case LIB_WAIT:
			lib_wait(v0);
			return v0;
		case LIB_CLEAR:
			lib_clear();
			return v0;
		case LIB_DIM:
			return (int)lib_dim(a0,a1,(int*)v0);
#ifdef __DEBUG
		case LIB_DEBUG:
			asm volatile("nop");
			return v0;
#endif
		case LIB_DIV0:
			err_div_zero();
			return v0;
		default:
			err_unknown();
			return v0;
	}
}