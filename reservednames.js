/*
   This file is provided under the LGPL license ver 2.1.
   Written by Katsumi.
   http://hp.vector.co.jp/authors/VA016157/
   kmorimatsu@users.sourceforge.jp
*/

/*
	This script is to make lines of reserved words for long var names.
*/

function name2int(name){
	var g_source=name+'$';
	var g_srcpos=0;
	var prevpos=0;
	var i=0;
	var b1=g_source.charCodeAt(0);
	do {
		// First character must be A-Z or _
		// From second, A-Z, _, and 0-9 can be used.
		i*=37;
		if (b1=='_'.charCodeAt(0)) b1='Z'.charCodeAt(0)+1;
		if ('0'.charCodeAt(0)<=b1 && b1<='9'.charCodeAt(0)) {
			i+=b1-'0'.charCodeAt(0);
		} else if (g_srcpos==prevpos) {
			// First character must be A-Z or _.
			// Subtract 9, resulting in 1-27 but not 10-36.
			// This subtraction is required to maintain
			// final number being <0x80000000.
			i+=b1-'A'.charCodeAt(0)+1;
		} else {
			i+=b1-'A'.charCodeAt(0)+10;
		}
		g_srcpos++;
		b1=g_source.charCodeAt(g_srcpos);
	} while ('0'.charCodeAt(0)<= b1 && b1<='9'.charCodeAt(0) || b1=='_'.charCodeAt(0) ||
		'A'.charCodeAt(0)<=b1 && b1<='Z'.charCodeAt(0));
	return i+65536;
}

function int2name(s6){
	var rem37_31=function(s6){
		return s6 % 37;
	};
	var div37_31=function(s6){
		return (s6-(s6 % 37))/37;
	};
	var str=new Array();
	var i,res;
	s6-=65536;
	str[6]=0;
	for(i=5;0<=i;i--){
		if (s6<37) {
			// First character must be A-Z or _, corresponding to 1-27 but not 10-36.
			// See get_label() for the detail.
			if (s6==27) str[i]='_'.charCodeAt(0);
			else str[i]=s6-1+'A'.charCodeAt(0);
			break;
		} else {
			// From second, 0-9 corresponds to 0-9 and A-Z corresponds to 10-36.
			str[i]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_".charCodeAt(rem37_31(s6));
			s6=div37_31(s6);
		}
	}
	//return (char*)(str+i);
	for(res='';str[i];i++) {
		res+=String.fromCharCode(str[i]);
	}
	return res;
}

// Show max number by this system.
//WScript.Echo(name2int('______').toString(16));

// Test functions for 10000 times with randomized var names
for(i=0;i<10000;i++){
	t="ABCDEFGHIJKLMNOPQRSTUVWXYZ_".charAt(Math.random()*27);
	for(j=0;j<1+Math.random()*4;j++){
		t+="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_".charAt(Math.random()*37);
	}
	j=name2int(t);
	if (int2name(j)!=t || j<0 || 0x7fffffff<j) {
		WScript.Echo('ERROR: '+t+' ('+j+'; 0x'+j.toString(16)+') '+i);
		WScript.Quit();
	}
}

var namearray=[
	'ABS',
	'ACOS',
	'ARGS',
	'ASC',
	'ASIN',
	'ATAN',
	'ATAN2',
	'BREAK',
	'CALL',
	'CDATA',
	'CEIL',
	'CHR',
	'CIRCLE',
	'CLEAR',
	'CLS',
	'COLOR',
	'COS',
	'COSH',
	'CREAD',
	'CURSOR',
	'DATA',
	'DEC',
	'DELETE',
	'DIM',
	'DO',
	'ELSE',
	'ELSEIF',
	'END',
	'ENDIF',
	'EXEC',
	'EXP',
	'FABS',
	'FCLOSE',
	'FEOF',
	'FGET',
	'FGETC',
	'FIELD',
	'FILE',
	'FINPUT',
	'FLEN',
	'FLOAT',
	'FLOOR',
	'FMOD',
	'FOPEN',
	'FOR',
	'FPRINT',
	'FPUT',
	'FPUTC',
	'FSEEK',
	'GCLS',
	'GCOLOR',
	'GETDIR',
	'GOSUB',
	'GOTO',
	'GPRINT',
	'HEX',
	'IDLE',
	'IF',
	'INKEY',
	'INPUT',
	'INT',
	'KEYS',
	'LABEL',
	'LEN',
	'LET',
	'LINE',
	'LOG',
	'LOG10',
	'LOOP',
	'MODF',
	'MUSIC',
	'NEXT',
	'NEW',
	'NOT',
	'OPTION',
	'PCG',
	'PEEK',
	'PEEK16',
	'PEEK32',
	'PI',
	'POINT',
	'POKE',
	'POKE16',
	'POKE32',
	'POW',
	'PRINT',
	'PSET',
	'PUBLIC',
	'PUTBMP',
	'READ',
	'REM',
	'RETURN',
	'RND',
	'SCROLL',
	'SETDIR',
	'SGN',
	'SIN',
	'SINH',
	'SOUND',
	'SQRT',
	'SYSTEM',
	'TAN',
	'TANH',
	'TIMER',
	'TVRAM',
	'UNTIL',
	'USEPCG',
	'USEVAR',
	'VAL',
	'VAR',
	'WAIT',
	'WEND',
	'WHILE',
	'WIDTH',
	// For megalopa
	'// For Megalopa',
	'OUT',
	'OUT8H',
	'OUT8L',
	'OUT16',
	'IN',
	'IN8H',
	'IN8L',
	'IN16',
	'ANALOG',
	'PWM',
	'SERIAL',
	'SPI',
	'I2C',
];
var result='';
var commented=0;
for(key in namearray){
	var nameint='0000'+name2int(namearray[key]).toString(16);
	nameint=nameint.substr(nameint.length-8);
	if (namearray[key].substr(0,2)=='//') {
		// This is a comment
		result+=namearray[key]+"\r\n";
		commented=1;
	} else {
		result+="\t0x"+nameint+", /*"+namearray[key]+"*/"+(commented ? " \\\r\n" : "\r\n");
	}
}

var fso = new ActiveXObject("Scripting.FileSystemObject");
var file = fso.CreateTextFile("result.txt");
file.Write(result);
file.Close();

WScript.Echo('Done!');

