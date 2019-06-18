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

unsigned int g_label;

char* get_label(void){
	unsigned int i;
	char b1;
	int prevpos;
	next_position();
	prevpos=g_srcpos;
	i=0;
	b1=g_source[g_srcpos];
	if ('0'<= b1 && b1<='9') {
		// May be line number
		do {
			i*=10;
			i+=b1-'0';
			g_srcpos++;
			b1=g_source[g_srcpos];
		} while ('0'<= b1 && b1<='9');
		// Check if end of the statement.
		if (i==0 || 65535<i) {
			// Line number 0 or more than 65535 is not available
			g_srcpos=prevpos;
			return ERR_SYNTAX;
		} else if (get_operator()) {
			// Oparator not found.
			g_label=i;
			return 0;
		} else {
			// This is not constant line number.
			g_srcpos=prevpos;
			g_label=0;
			return 0;
		}
	} else if ('A'<=b1 && b1<='Z' || b1=='_') {
		// May be label
		// When changing here, see also chech_var_name()
		do {
			// First character must be A-Z or _
			// From second, A-Z, _, and 0-9 can be used.
			i*=37;
			if (b1=='_') b1='Z'+1;
			if ('0'<=b1 && b1<='9') {
				i+=b1-'0';
			} else if (g_srcpos==prevpos) {
				// First character must be A-Z or _.
				// Subtract 9, resulting in 1-27 but not 10-36.
				// This subtraction is required to maintain
				// final number being <0x80000000.
				i+=b1-'A'+1;
			} else {
				i+=b1-'A'+10;
			}
			g_srcpos++;
			b1=g_source[g_srcpos];
		} while ('0'<= b1 && b1<='9' || 'A'<=b1 && b1<='Z' || b1=='_');
		// Length of the label must be between 2 and 6.
		if (g_srcpos-prevpos<2 || 6<g_srcpos-prevpos) {
			g_srcpos=prevpos;
			return ERR_LABEL_LONG;
		}
		// Must not be a function
		next_position();
		if (g_source[g_srcpos]=='(') {
			g_srcpos=prevpos;
			g_label=0;
			return 0;
		}
		g_label=i+65536;
		return 0;
	} else {
		g_label=0;
		return 0;
	}
}

void* search_label(unsigned int label){
	unsigned int i,code,search1,search2;
	if (label&0xFFFF0000) {
		// Label
		search1=0x3C160000|((label>>16)&0x0000FFFF); //lui s6,yyyy;
		search2=0x36D60000|(label&0x0000FFFF);       //ori s6,s6,zzzz;
		for(i=0;i<g_objpos;i++){
			code=g_object[i];
			if (code==search1) {
				if (g_object[i+1]==search2) {
					// Label number found
					return &(g_object[i]);
				}
			}
			if (code&0xFFFF0000==0x04110000) {
				// "bgezal zero," assembly found. Skip following block (strig).
				i+=code&0x0000FFFF;
			}
		}
		// Line number not found.
		return 0;
	} else {
		// Line number
		search1=0x34160000|label; //ori         s6,zero,xxxx;
		for(i=0;i<g_objpos;i++){
			code=g_object[i];
			if (code==search1) {
				// Line number found
				return &(g_object[i]);
			}
			if (code&0xFFFF0000==0x04110000) {
				// "bgezal zero," assembly found. Skip following block (strig).
				i+=code&0x0000FFFF;
			}
		}
		// Line number not found.
		return 0;
	}
}

void* search_breakout(unsigned int start, int* prevcode){
	unsigned int pos,code1,depth;
	// Start search from start point where BREAK statement is used.
	depth=0;
	for(pos=start;pos<g_objpos;pos++){
		code1=g_object[pos];
		switch(code1>>16){
			case 0x0411:
				// "bgezal zero," assembly found. Skip following block (strig).
				pos+=code1&0x0000FFFF;
				break;
			case 0x0820: // FOR
			case 0x0821: // WHILE
			case 0x0822: // DO
				depth++;
				break;
			case 0x0830: // NEXT
			case 0x0831: // WEND
			case 0x0832: // LOOP
				if (0<depth) {
					depth--;
					break;
				}
				// Destination found.
				// Previous code will be also set if required for CONTINUE statement.
				if (prevcode) prevcode[0]=g_object[pos-1];
				return (void*)&g_object[pos];
			default:
				break;
		}
	}
	return 0;
}

void* search_ifout(unsigned int start){
	unsigned int pos,code1,depth;
	// Start search from start point where BREAK statement is used.
	depth=0;
	for(pos=start;pos<g_objpos;pos++){
		code1=g_object[pos];
		switch(code1>>16){
			case 0x0411:
				// "bgezal zero," assembly found. Skip following block (strig).
				pos+=code1&0x0000FFFF;
				break;
			case 0x3000: // Block marker
				if (code1==0x30008000) {
					// end block
					if (0<depth) {
						depth--;
						break;
					}
					// Destination found.
					return (void*)&g_object[pos];
				} else if (code1==0x30000000) {
					// begin block
					depth++;
					break;
				} else {
					break;
				}
			default:
				break;
		}
	}
	return 0;
}

/*
	Following codes are dedicated to specific use:
	0x0411xxxx: String/data block
		Use 0x0413xxxx (bzegall zero,xxxx) for other cases to get PC to $ra
	0x0810xxxx, 0x0811xxxx: GOTO statement
	0x0812xxxx, 0x0813xxxx: GOSUB statement
	0x0814xxxx, 0x0815xxxx: SOUND etc, for setting v0 as pointer to DATA array.
	0x0816xxxx: BREAK statemant and relatives
		0x08160000: BREAK
		0x08160008: CONTINUE
		0x08160100: Jump to next ELSE, ELSEIF or ENDIF
	0x082xyyyy: Begin block (FOR/DO/WHILE)
	0x083xyyyy: End block (NEXT/LOOP/WEND)
	0x00000020, 0x00000021,
	0x00000022, 0x00000023: Marker for begining the DATA region.
	                        MLB 2 bits show skip byte length in DATA.
	0x30000000: Begin block (IF-THEN-ELSEIF-ELSE-ENDIF)
	0x30008000: End block (IF-THEN-ELSEIF-ELSE-ENDIF)
	0x3000Fxxx: General purpose NOP with value 0x0000-0x0FFF.

	IF-THEN-ELSEIF-ELSE-ENDIF is written as follows:
		IF-THEN: 0x30000000 0x10400000 0x30000000
		ELSEIF-THEN: 0x08160100 0x30008000 (conditional expression) 0x10400000 0x30000000
		ELSE: 0x08160100 0x30008000 0x30000000
		ENDIF: 0x30008000 0x30008000 
	, where "0x10400000 0x30000000" and "0x08160100 0x30008000" will be replaced by
	codes jumping to next 0x30008000. The 0x30000000 - 0x30008000 blocks will be skipped.
*/

char* link(void){
	int pos;
	unsigned int code1,code2,label;
	g_fileline=0;
	for(pos=0;pos<g_objpos;pos++){
		code1=g_object[pos];
		switch(code1>>16){
			case 0x0411:
				// "bgezal zero," assembly found. Skip following block (strig).
				pos+=code1&0x0000FFFF;
				break;
			case 0x3416:
				// "ori s6,zero,xxxx" found this is the first word in a line.
				g_fileline++;
				g_line=code1&0x0000FFFF;
				break;
			case 0x0810:
				// GOTO
				code2=g_object[pos+1];
				if ((code2&0xFFFF0000)!=0x08110000) continue;
				code1&=0x0000FFFF;
				code2&=0x0000FFFF;
				label=(code1<<16)|code2;
				code1=(int)search_label(label);
				g_label=label;
				if (!code1) return ERR_LABEL_NF;
				code1&=0x0FFFFFFF;
				code1>>=2;
				code1|=0x08000000; // j xxxx
				g_object[pos++]=code1;
				g_object[pos]=0x00000000; // nop
				break;
			case 0x0812:
				// GOSUB
				code2=g_object[pos+1];
				if ((code2&0xFFFF0000)!=0x08130000) continue;
				code1&=0x0000FFFF;
				code2&=0x0000FFFF;
				label=(code1<<16)|code2;
				code2=(int)search_label(label);
				g_label=label;
				if (!code2) return ERR_LABEL_NF;
				code2&=0x0FFFFFFF;
				code2>>=2;
				code2|=0x08000000; // j xxxx
				g_object[pos++]=0x00000000; // nop
				g_object[pos]=code2;
				break;
			case 0x0814:
				// SOUND etc, for setting v0 as pointer to label/line
				code2=g_object[pos+1];
				if ((code2&0xFFFF0000)!=0x08150000) continue;
				code1&=0x0000FFFF;
				code2&=0x0000FFFF;
				label=(code1<<16)|code2;
				code1=(int)search_label(label);
				g_label=label;
				if (!code1) return ERR_LABEL_NF;
				g_object[pos++]=0x3C020000|((code1>>16)&0x0000FFFF); // lui   v0,xxxx
				g_object[pos]  =0x34420000|(code1&0x0000FFFF);       // ori v0,v0,xxxx
				break;
			case 0x0816:
				switch(code1&0xFFFF) {
					case 0x0000:
						// BREAK statement
						// Find next the NEXT or WHILE statement and insert jump code after this.
						g_label=g_line;
						code1=(int)search_breakout(pos,0);
						if (!code1) return ERR_INVALID_BREAK;
						code1&=0x0FFFFFFF;
						code1>>=2;
						code1|=0x08000000; // j xxxx
						g_object[pos]=code1;
						break;
					case 0x0008:
						// CONTINUE statement
						// Find next the NEXT or WHILE statement and insert jump code after this.
						g_label=g_line;
						code1=(int)search_breakout(pos,&g_temp);
						if (!code1) return ERR_INVALID_BREAK;
						if (0x3000F000 == (g_temp&0xFFFFF000)) {
							// WEND or LOOP statement found
							code1-=(g_temp&0x0FFF)<<2;
						} else {
							// NEXT statement found
							code1-=3<<2;
						}
						code1&=0x0FFFFFFF;
						code1>>=2;
						code1|=0x08000000; // j xxxx
						g_object[pos]=code1;
						break;
					case 0x0100:
						// Jump to next ENDIF
						g_label=g_line;
						// "pos+2" is for skipping next code (must be 0x30008000)
						code1=(int)search_ifout(pos+2);
						if (!code1) return ERR_INVALID_ELSEIF;
						code1&=0x0FFFFFFF;
						code1>>=2;
						code1|=0x08000000; // j xxxx
						g_object[pos]=code1;
						break;
					default:
						break;
				}
				break;
			case 0x3000:
				// Block marker
				switch(code1&0xFFFF) {
					case 0x0000:
						// Begin if block
						if (g_object[pos-1]==0x10400000) { // beq v0,zero,xxxx
							// IF-THEN or ELSEIF-THEN
							// Jump to next ELSE, ELSEIF or ENDIF
							g_label=g_line;
							// "pos+1" is for skipping current code (0x30000000)
							code1=(int)search_ifout(pos+1);
							if (!code1) return ERR_INVALID_ELSEIF;
							code1-=(int)(&g_object[pos]);
							code1>>=2;
							code1&=0x0000FFFF;
							code1|=0x10400000; // beq v0,zero,xxxx
							g_object[pos-1]=code1;
							break;
						}
						break;
					default:
						break;
				}
				break;
			case 0x0820: // FOR
			case 0x0830: // NEXT
			case 0x0821: // WHILE
			case 0x0831: // WEND
			case 0x0822: // DO
			case 0x0832: // WHILE
				// These are used for detecing the depth of structures.
				// Change them to stack increase/decrease commands.
				g_object[pos]=0x27BD0000|(code1&0x0000FFFF); //// addiu       sp,sp,xx
				break;
			case 0x2407:                                // addiu       a3,zero,xxxx
				if (g_object[pos-1]!=0x02E0F809) break; // jalr        ra,s7
				// call_lib_code(x)
				switch(code1&0x0000FFFF){
					case LIB_RESTORE:
						// Convert label data to pointer if not dynamic
						code1=g_object[pos-3];
						code2=g_object[pos-2];
						if ((code1>>16)!=0x3C02) break; // lui         v0,xxxx
						if ((code2>>16)!=0x3442) break; // ori         v0,v0,xxxx
						label=(code1<<16)|(code2&0x0000FFFF);
						code1=(int)search_label(label);
						g_label=label;
						if (!code1) return ERR_LABEL_NF;
						code2=code1&0x0000FFFF;
						code1=code1>>16;
						g_object[pos-3]=0x3C020000|code1;      // lui         v0,xxxx
						g_object[pos-2]=0x34420000|code2;      // ori         v0,v0,xxxx
						g_object[pos]=0x24070000|LIB_RESTORE2; // addiu       a3,zero,xxxx
						break;
					default:
						break;
				}
				break;
			default:
				break;
		}
	}
	return 0;
}