/*
   This file is provided under the LGPL license ver 2.1.
   Written by K.Tanaka & Katsumi
   http://www.ze.em-net.ne.jp/~kenken/index.html
   http://hp.vector.co.jp/authors/VA016157/
*/

/*
	This file is shared by Megalopa and Zoea
*/

#include <xc.h>
#include "api.h"
#include "compiler.h"

static FSFILE* g_fhandle;
static char* g_fbuff;
static int g_size;
static int g_filepoint;

char* init_file(char* buff,char* appname){
	// Open file
	g_fhandle=FSfopen(appname,"r");
	if (!g_fhandle) {
		return ERR_UNKNOWN;
	}
	// Initialize parameters
	g_fbuff=buff;
	g_line=0;
	g_fileline=0;
	g_source=buff;
	g_srcpos=0;
	g_filepoint=0;
	return 0;
}

void close_file(){
	FSfclose(g_fhandle);
}

int filepoint(){
	return g_filepoint+g_srcpos;
}

void read_file(int blocklen){
	int i;
	static char in_string, escape;
	// blocklen is either 512 or 256.
	if (blocklen==512) {
		// This is first read. Initialize parameter(s).
		in_string=0;
		escape=0;
	} else if (g_size<512) {
		// Already reached the end of file.
		return;
	} else {
		// Shift buffer and source position 256 bytes.
		for(i=0;i<256;i++) g_fbuff[i]=g_fbuff[i+256];
		g_srcpos-=256;
		g_filepoint+=256;
	}
	// Read 512 or 256 bytes from SD card.
	g_size=512-blocklen+FSfread((void*)&g_fbuff[512-blocklen],1,blocklen,g_fhandle);
	// Some modifications of text for easy compiling.
	for(i=512-blocklen;i<512;i++){
		if (in_string) {
			if (g_fbuff[i]=='\\' && !escape) {
				escape=1;
			} else {
				escape=0;
				if (g_fbuff[i]=='"') in_string=0;
			}
		} else {
			// If not in string, all upper cases.
			if (g_fbuff[i]=='"') in_string=1;
			else if ('a'<=g_fbuff[i] && g_fbuff[i]<='z') g_fbuff[i]+='A'-'a';
			// If not in string, tabs will be spaces.
			else if ('\t'==g_fbuff[i]) g_fbuff[i]=' ';
		}
		if (g_fbuff[i]==0x0a || g_fbuff[i]==0x0d) in_string=escape=0;
	}
	return;
}

char* compile_file(){
	int i;
	char* err;
	// Read first 512 bytes
	read_file(512);
	// Compile line by line
	while (g_size==512) {
		err=compile_line();
		if (err==ERR_OPTION_CLASSCODE) {
			g_size=g_srcpos=0;
			break;
		} else if (err) {
			return err;
		}
		// Maintain at least 256 characters in cache.
		if (256<=g_srcpos) read_file(256);
	}
	// Return code at the end
	g_source[g_size]=0x0d;
	// Compile last few lines.
	while(g_srcpos<g_size-1){
		err=compile_line();
		if (err==ERR_OPTION_CLASSCODE) {
			break;
		} else if (err) {
			return err;
		}
	}
	// Add "DATA 0" and "END" statements.
	if (g_compiling_class) {
		g_source="END\n";
	} else {
		g_source="DATA 0:END\n";
	}
	g_srcpos=0;
	err=compile_line();
	if (err) return err;
	g_srcpos=-1;
	// No error occured
	return 0;
}

int compile_and_link_file(char* buff,char* appname){
	int i,j;
	char* err;

	while(1){
		// Initialize SD card file system
		err=init_file(buff,appname);
		if (err) {
			//setcursorcolor(COLOR_ERRORTEXT);
			printstr("Can't Open ");
			printstr(appname);
			printchar('\n');
			return -1;
		}

		// Option initialization(s)
		g_option_nolinenum=0;
		g_option_fastfield=0;

		// Compile the file
		err=compile_file();
		close_file();

		// If compiling a class file is required, do it.
		if (err==ERR_COMPILE_CLASS) {
			j=g_compiling_class;
			i=compile_and_link_class(buff, g_class);
			g_compiling_class=j;
			if (i) return i;
			// Continue compiling current file from the beginning.
			continue;
		}
		break;
	}

	if (err) {
		// Compile error
		printstr(err);
		printstr("\nAround: '");
		for(i=0;i<5;i++){
			printchar(g_source[g_srcpos-2+i]);
		}
		printstr("' in line ");
		printdec(g_line);
		printstr("\n");
		for(i=g_srcpos;0x20<=g_source[i];i++);
		g_source[i]=0x00;
		for(i=g_srcpos;0x20<=g_source[i];i--);
		printstr(g_source+i);
		return g_fileline;
	}

	// Link
	err=link();
	if (err) {
		// Link error
		printstr(err);
		printstr(resolve_label(g_label));
		return -2;
	}
	
	// All done
	return 0;
}

int compile_and_link_class(char* buff,int class){
	int i,j;
	char* err;
	char* classname;
	char classfile[13];
	char classdir[11];
	int data[2];
	unsigned short cwd_id;
	int* record;
	g_num_classes++;
	while(1){
		// Begin compiling class
		err=begin_compiling_class(class);
		if (err) break;
		// Determine class file name
		classname=resolve_label(class);
		for(i=0;classfile[i]=classname[i];i++);
		classfile[i++]='.';
		classfile[i++]='B';
		classfile[i++]='A';
		classfile[i++]='S';
		classfile[i]=0;
		// Check if file exists in current directory
		err=init_file(buff,&classfile[0]);
		if (!err) {
			// Class file found in current directory
			close_file();
			// Compile it
			i=compile_and_link_file(buff,&classfile[0]);
			if (i) break;
		} else {
			// Class file not found in current directory.
			// Try library directory, for example, \LIB\CLASS1\CLASS1.BAS
			// Store current directory, first
			if (!FSgetcwd(buff,256)) break;
			for(i=0;buff[i];i++);
			cwd_id=cmpdata_get_id();
			if (!cwd_id) break;
			err=cmpdata_insert(CMPDATA_TEMP,cwd_id,(int*)(&buff[0]),(i+1+3)>>2);
			if (err) break;
			// Change current directory to class library directory
			for(i=0;classdir[i]="\\LIB\\"[i];i++);
			for(j=0;classdir[i++]=classname[j];j++);
			classdir[i]=0;
			FSchdir(classdir);
			// Compile class file
			i=compile_and_link_file(buff,&classfile[0]);
			// Restore current dirctory
			cmpdata_reset();
			while(record=cmpdata_find(CMPDATA_TEMP)){
				if (cwd_id=(record[0]&0xffff)) break;
			}
			if (!record) break;
			FSchdir((char*)(&record[1]));
			cmpdata_delete(record);
			if (i) break;
		}
		// End compiling class
		err=end_compiling_class(class);
		if (err) break;
		// Initial assembly is a jump statement to jump to the end of class file
		// Note that there is at least a code (set line # to $s6) before reaching here
		g_object[0]=0x08000000 | ((((int)(&g_object[g_objpos]))&0x0FFFFFFF)>>2); // j xxxxxxxx
		// In the next link, current region of object is ignored.
		g_object+=g_objpos;
		g_objpos=0;
		// All done
		return 0;
	}
	// Error occured
	printstr("/nError in class: ");
	printstr((char*)&classfile[0]);
	printchar('\n');
	if (err) printstr(err);
	return -2;
}

int compile_and_link_main_file(char* buff,char* appname){
	int i;
	// Reset parameters
	g_compiling_class=0;
	g_num_classes=0;
	// Compile the file
	i=compile_and_link_file(buff,appname);
	if (i) return i;
	return 0;
	/*
		After compiling class code, g_object is set to the beginning of next code.
		Therefore, after the all, g_object is set toe the beginnig of main code,
		and class code(s) is/are excluded. This will affect following features when running:
			READ/DATA/RESTORE function/statements
		The linker also works withing the g_object dimension. Therefore, the label only works withing the file,
		but not in the other file. This feature allows using the same label name in different files without
		causing error/misjumping

		After compiling class code, following cmpdata are destroyed (see delete_cmpdata_for_class() function):
			CMPDATA_FIELD  : object field and method information
			CMPDATA_USEVAR : long var name information
		but following cmpdata remains:
			CMPDATA_CLASS  : class name and address of class structure
		This feature allows compiler to use class information (name and structure) for "NEW" function,
		to use the same long var name in different files (note that g_long_name_var_num is not reseted after
		compiling each class code).
	*/
}