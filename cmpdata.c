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
	This file provide functions for handling data used when compiling.
	The data is inserted between g_objmax and file cache.
	Data format (32 bit):
		MSB                LSB
		 +----+----+--------+
		 |type|len | data16 |
		 +----+----+--------+
		where,
			type:   data type number (unsigned char)
			len:    length of data area in number of words (unsigned char)
			data16: general 16 bit data (short)
*/

/*
	CMPDATA_TEMP structure
		type:      CMPDATA_TEMP (6)
		len:       n+1
		data16:    id
		record[1]: any data
		record[2]: any data
		...
		record[n]: any data
*/

#define g_cmpdata g_objmax

static int* g_cmpdata_end;
static int* g_cmpdata_point;
static unsigned short g_cmpdata_id;

/*
	Initialize routine must be called when starting compiler.
*/
void cmpdata_init(){
	g_cmpdata_end=g_objmax;
	g_cmpdata_point=g_objmax;
	g_cmpdata_id=1;
}

/*
	Returns ID as 16 bit indivisual number
*/
unsigned short cmpdata_get_id(){
	if ((++g_cmpdata_id)==0) printstr("CMPDATA: no more ID!\n");
	return g_cmpdata_id;
}

/*
	Function to insert a data. The data must be defined by a pointer to int array.
		unsigned char type: Data type number (0-255)
		short data16:       16 bit data. If not required, set 0.
		int* data:          Pointer to data array. If not requird, set 0.
		unsigned char num:  Length of above data array. If not required, set 0.
*/
char* cmpdata_insert(unsigned char type, short data16, int* data, unsigned char num){
	unsigned char i;
	g_cmpdata-=num+1;
	if (g_cmpdata<g_object+g_objpos) return ERR_NE_BINARY;
	g_cmpdata[0]=(type<<24)|(num+1)<<16|data16;
	for(i=0;i<num;i++){
		g_cmpdata[i+1]=data[i];
	}
	return 0;
}

/*
	Reset data point. Next search will be from the beginning.
*/

void cmpdata_reset(){
	g_cmpdata_point=g_cmpdata;
}

/*
	Find the next record with defined type. Return the pointer to the record.
*/
int* cmpdata_find(unsigned char type){
	int* ret;
	while(g_cmpdata_point<g_cmpdata_end){
		// Remember return value
		ret=g_cmpdata_point;
		// Move the point to next
		g_cmpdata_point+=(ret[0]&0x00ff0000)>>16;
		// Check if type is the same. If the same, return.
		if ((ret[0]>>24)==type) return ret;
	}
	return 0;
}

/*
	Find the record from beginning.
*/

int* cmpdata_findfirst(unsigned char type){
	cmpdata_reset();
	return cmpdata_find(type);
}

/*
	Delete a record.
*/
void cmpdata_delete(int* record){
	int delnum;
	int* data;
	// Ignore if invalid record.
	if (record<g_cmpdata || g_cmpdata_end<record) return;
	// Get number of word to delete.
	delnum=(record[0]&0x00ff0000)>>16;
	// Delete record by shifting data.
	for(data=record-1;g_cmpdata<=data;data--){
	     data[delnum]=data[0];
	}
	g_cmpdata+=delnum;
	// Reset
	cmpdata_reset();
}
