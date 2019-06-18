/*
   This file is provided under the LGPL license ver 2.1.
   Written by Katsumi.
   http://hp.vector.co.jp/authors/VA016157/
   kmorimatsu@users.sourceforge.jp
*/

#include <xc.h>
#include "compiler.h"
#include "api.h"

/*
	Timer3: 1/32 prescaler, toggle mode :750000 Hz
	PR3=1704 <-> 440 Hz
	Shift tone for 2048 <--> ~440 Hz.
		x 1705 / 2048 = 213 / 256 + 1 / 2048

*/

/*
	c:  1434
	B:  1519
	A#: 1609
	A:  1705
	G#: 1806
	G:  1914
	F#: 2028
	F:  2148
	E:  2276
	D#: 2411
	D:  2555
	C#: 2707
	C:  2867
	Cb:	3038
*/

const static int g_keys[]={
	1609,1434,2707,2411,2148,2028,1806,//  0 7# C# A#m
	1609,1519,2707,2411,2148,2028,1806,//  7 6# F# D#m
	1609,1519,2707,2411,2276,2028,1806,// 14 5# B  G#m
	1705,1519,2707,2411,2276,2028,1806,// 21 4# E  C#m
	1705,1519,2707,2555,2276,2028,1806,// 28 3# A  F#m
	1705,1519,2707,2555,2276,2028,1914,// 35 2# D  Bm
	1705,1519,2867,2555,2276,2028,1914,// 42 1# G  Em
	1705,1519,2867,2555,2276,2148,1914,// 49 0  C  Am
	1705,1609,2867,2555,2276,2148,1914,// 56 1b F  Dm
	1705,1609,2867,2555,2411,2148,1914,// 63 2b Bb Gm
	1806,1609,2867,2555,2411,2148,1914,// 70 3b Eb Cm
	1806,1609,2867,2707,2411,2148,1914,// 77 4b Ab Fm
	1806,1609,2867,2707,2411,2148,2028,// 84 5b Db Bbm
	1806,1609,3038,2707,2411,2148,2028,// 91 6b Gb Ebm
	1806,1609,3038,2707,2411,2276,2028 // 98 7b Cb Abm
};

/*
	2^(1/12)    ~= 69433/(2^16)
	1/(2^(1/12) ~= 1933/(2^11)
*/

#define toneFlat(x) ((((unsigned long)(x))*69433)>>16)
#define toneSharp(x) ((((unsigned long)(x))*1933)>>11)

/* local global vars */
static int* g_tones;
static int g_qvalue;
static int g_lvalue;
static int g_mpoint;
static char* g_mstr;
static int g_mspos;
static unsigned short g_music[32];
static unsigned short g_sound[32];
static unsigned short g_musiclen[32];
static unsigned char g_soundlen[32];
static int g_musicstart;
static int g_musicend;
static int g_musicwait;
static int g_soundstart;
static int g_soundend;
static int g_soundwait;
static int g_soundrepeat;

static char g_sound_mode=0;
static FSFILE* g_fhandle=0;
static char* g_wavtable=0;

#define SOUND_MODE_MUSIC 1
#define SOUND_MODE_WAVE 2

#define start_dma() T4CONSET=0x8000; DCH2CONSET=0x00000080; g_sound_mode=SOUND_MODE_WAVE
#define stop_dma()  T4CONCLR=0x8000; DCH2CONCLR=0x00000080; g_sound_mode=SOUND_MODE_MUSIC

int waveRemaining(int mode){
	if (!g_fhandle) return 0;
	switch(mode){
		case 1: // current position (header excluded)
			return g_fhandle->seek-0x2c;
			break;
		case 2: // file size (header excluded)
			return g_fhandle->size-0x2c;
			break;
		case 0: // remaining
		default:
			return g_fhandle->size-g_fhandle->seek;
			break;
	}
}
int musicRemaining(int flagsLR){
	// flagsLR is ignored
	return (g_musicend-g_musicstart)&31;
}

void musicint(){
	unsigned int i;
	static unsigned short wavtable_pos;
	// This function is called every 1/60 sec.
	switch(g_sound_mode){
		case SOUND_MODE_MUSIC:
			if (g_soundstart!=g_soundend){
				// Start timer & OC4
				i=g_sound[g_soundstart];
				if (i<0xffff) {
					T3CONSET=0x8000;
					PR3=i;
					if (i<TMR3) TMR3=0;
				} else {
					T3CONCLR=0x8000;
				}
				if ((--g_soundwait)<=0) {
					g_soundstart++;
					if (g_soundstart==g_soundend || 31<g_soundstart) {
						g_soundstart=0;
						g_soundrepeat--;
						if (0<g_soundrepeat) {
							g_soundwait=g_soundlen[g_soundstart];
						} else {
							g_soundend=g_soundrepeat=g_soundwait=0;
						}
					} else {
						g_soundwait=g_soundlen[g_soundstart];
					}
				}
				// Shift music data even though without output.
				if (g_musicstart!=g_musicend) {
					if ((--g_musicwait)<=0) {
						g_musicstart++;
						g_musicstart&=31;
						g_musicwait=g_musiclen[g_musicstart];
					}
				}
			} else if (g_musicstart!=g_musicend) {
				// Start timer & OC4
				i=g_music[g_musicstart];
				if (i<0xffff) {
					T3CONSET=0x8000;
					PR3=i;
					if (i<TMR3) TMR3=0;
				} else {
					T3CONCLR=0x8000;
				}
				if ((--g_musicwait)<=0) {
					g_musicstart++;
					g_musicstart&=31;
					g_musicwait=g_musiclen[g_musicstart];
					if (((g_musicstart+1)&31)==g_musicend) {
						// Raise MUSIC interrupt flag
						raise_interrupt_flag(INTERRUPT_MUSIC);
					}
				}
			} else {
				// Stop timer
				T3CONCLR=0x8000;
			}
			break;
		case SOUND_MODE_WAVE:
			// Initialize parameters
			if (!T4CONbits.ON){
				start_dma();
				wavtable_pos=0;
			}
			wavtable_pos=262-wavtable_pos;
			// Read from file
			if (0 == FSfread((void*)&g_wavtable[wavtable_pos],1,262,g_fhandle)) {
				// End of file.
				stop_dma();
				FSfclose(g_fhandle);
				g_fhandle=0;
				g_sound_mode=SOUND_MODE_MUSIC;
				stop_music();
				// Raise WAVE interrupt flag
				raise_interrupt_flag(INTERRUPT_WAVE);
				break;
			}
			break;
		default:
			break;
	}
}

int musicGetNum(){
	int i, ret;
	char b;
	// Skip non number character
	for(i=0;(b=g_mstr[g_mspos+i])<'0' && '9'<g_mstr[g_mspos+i];i++);
	// Determine the number
	ret=0;
	while('0'<=b && b<='9'){
		ret*=10;
		ret+=b-'0';
		i++;
		b=g_mstr[g_mspos+i];
	}
	g_mspos+=i;
	return ret;
}

void stop_music(){
	// Initialize normal music mode.

	// Use Timer3 and OC4 for sound.
	RPA2R=5;         //Use RPA2 for OC4
	OC4R=0;
	OC4CON=0x000b;   // Timer3, toggle mode
	OC4CONSET=0x8000;// Start OC4
	T3CON=0x0050;    // Prescaller: 1:32 (1.5 MHz), not yet started

	// Initializations for music/sound.
	g_musicstart=g_musicend=g_musicwait=g_soundstart=g_soundend=g_soundwait=g_soundrepeat=0;
	g_sound_mode=SOUND_MODE_MUSIC;

	// Initialize DMA (see also init_dma_music())

	// Stop Timer4
	T4CON=0x0000; // Not start yet

	// Enable DMA, stop DMA2
	DMACONSET=0x8000;
	DCH2CONCLR=0x0080;

	// Close handle if open.
	if (g_fhandle) FSfclose(g_fhandle);
	g_fhandle=0;

	// Inactive music first
	g_music_active=0;
}

void init_music(){
	// Initilize music system
	stop_music();
	
	// Initializations for music/sound.
	g_qvalue=160; // Q: 1/4=90
	g_lvalue=20;   // L: 1/8
	g_tones=(int*)&(g_keys[49]); // C major
}

void musicSetL(){
	// Set length of a character.
	// Syntax: L:n/m, where n and m are numbers.
	int n,m;
	n=musicGetNum();
	g_mspos++;
	m=musicGetNum();
	g_lvalue=g_qvalue*n/m;
}

void musicSetQ(){
	int i;
	// Syntax: Q:1/4=n, where n is number.
	// Skip "1/4="
	for(i=0;g_mstr[g_mspos+i]!='=';i++);
	g_mspos+=i+1;
	i=musicGetNum();
	if      (i<48)  { g_qvalue=320; /* 1/4=45  */ }
	else if (i<53)  { g_qvalue=288; /* 1/4=50  */ }
	else if (i<60)  { g_qvalue=256; /* 1/4=56  */ }
	else if (i<70)  { g_qvalue=224; /* 1/4=64  */ }
	else if (i<83)  { g_qvalue=192; /* 1/4=75  */ }
	else if (i<102) { g_qvalue=160; /* 1/4=90  */ }
	else if (i<132) { g_qvalue=128; /* 1/4=113 */ }
	else if (i<188) { g_qvalue=96;  /* 1/4=150 */ }
	else            { g_qvalue=64;  /* 1/4=225 */ }
	g_lvalue=g_qvalue>>3;
}

void musicSetK(){
	// Syntax: K:xxx
	if (!strncmp((char*)&(g_mstr[g_mspos]),"A#m",3)) {
			g_mspos+=3;
			g_tones=(int*)&(g_keys[0]);
			return;
	} else if (!strncmp((char*)&(g_mstr[g_mspos]),"D#m",3)) {
			g_mspos+=3;
			g_tones=(int*)&(g_keys[7]);
			return;
	} else if (!strncmp((char*)&(g_mstr[g_mspos]),"G#m",3)) {
			g_mspos+=3;
			g_tones=(int*)&(g_keys[14]);
			return;
	} else if (!strncmp((char*)&(g_mstr[g_mspos]),"C#m",3)) {
			g_mspos+=3;
			g_tones=(int*)&(g_keys[21]);
			return;
	} else if (!strncmp((char*)&(g_mstr[g_mspos]),"F#m",3)) {
			g_mspos+=3;
			g_tones=(int*)&(g_keys[28]);
			return;
	} else if (!strncmp((char*)&(g_mstr[g_mspos]),"Bbm",3)) {
			g_mspos+=3;
			g_tones=(int*)&(g_keys[84]);
			return;
	} else if (!strncmp((char*)&(g_mstr[g_mspos]),"Ebm",3)) {
			g_mspos+=3;
			g_tones=(int*)&(g_keys[91]);
			return;
	} else if (!strncmp((char*)&(g_mstr[g_mspos]),"Abm",3)) {
			g_mspos+=3;
			g_tones=(int*)&(g_keys[98]);
			return;
	}
	if (!strncmp((char*)&(g_mstr[g_mspos]),"C#",2)) {
			g_mspos+=2;
			g_tones=(int*)&(g_keys[0]);
			return;
	} else if (!strncmp((char*)&(g_mstr[g_mspos]),"F#",2)) {
			g_mspos+=2;
			g_tones=(int*)&(g_keys[7]);
			return;
	} else if (!strncmp((char*)&(g_mstr[g_mspos]),"Bm",2)) {
			g_mspos+=2;
			g_tones=(int*)&(g_keys[35]);
			return;
	} else if (!strncmp((char*)&(g_mstr[g_mspos]),"Em",2)) {
			g_mspos+=2;
			g_tones=(int*)&(g_keys[42]);
			return;
	} else if (!strncmp((char*)&(g_mstr[g_mspos]),"Am",2)) {
			g_mspos+=2;
			g_tones=(int*)&(g_keys[49]);
			return;
	} else if (!strncmp((char*)&(g_mstr[g_mspos]),"Dm",2)) {
			g_mspos+=2;
			g_tones=(int*)&(g_keys[56]);
			return;
	} else if (!strncmp((char*)&(g_mstr[g_mspos]),"Gm",2) || !strncmp((char*)&(g_mstr[g_mspos]),"Bb",2)) {
			g_mspos+=2;
			g_tones=(int*)&(g_keys[63]);
			return;
	} else if (!strncmp((char*)&(g_mstr[g_mspos]),"Cm",2) || !strncmp((char*)&(g_mstr[g_mspos]),"Eb",2)) {
			g_mspos+=2;
			g_tones=(int*)&(g_keys[70]);
			return;
	} else if (!strncmp((char*)&(g_mstr[g_mspos]),"Fm",2) || !strncmp((char*)&(g_mstr[g_mspos]),"Ab",2)) {
			g_mspos+=2;
			g_tones=(int*)&(g_keys[77]);
			return;
	} else if (!strncmp((char*)&(g_mstr[g_mspos]),"Db",2)) {
			g_mspos+=2;
			g_tones=(int*)&(g_keys[84]);
			return;
	} else if (!strncmp((char*)&(g_mstr[g_mspos]),"Gb",2)) {
			g_mspos+=2;
			g_tones=(int*)&(g_keys[91]);
			return;
	} else if (!strncmp((char*)&(g_mstr[g_mspos]),"Cb",2)) {
			g_mspos+=2;
			g_tones=(int*)&(g_keys[98]);
			return;
	}
	switch(g_mstr[g_mspos]){
		case 'B':
			g_mspos++;
			g_tones=(int*)&(g_keys[14]);
			return;
		case 'E':
			g_mspos++;
			g_tones=(int*)&(g_keys[21]);
			return;
		case 'A':
			g_mspos++;
			g_tones=(int*)&(g_keys[28]);
			return;
		case 'D':
			g_mspos++;
			g_tones=(int*)&(g_keys[35]);
			return;
		case 'G':
			g_mspos++;
			g_tones=(int*)&(g_keys[42]);
			return;
		case 'C':
			g_mspos++;
			g_tones=(int*)&(g_keys[49]);
			return;
		case 'F':
			g_mspos++;
			g_tones=(int*)&(g_keys[56]);
			return;
		default:
			err_music(g_mstr);
			break;
	}
}
void musicSetM(){
	// Currently do nothing
	musicGetNum();
	musicGetNum();
}

void set_sound(unsigned long* data, int flagsLR){
	// flagsLR is ignored
	int sound;
	int len;
	int pos;
	int datalen;
	IEC0bits.T4IE=0; // Stop interruption, first.
	// Initialize
	g_soundrepeat=g_soundstart=g_soundend=0;
	pos=0;
	do {
		while(data[1]!=0x00000020) data++; // Seek DATA statement
		datalen=(data[0]&0x00007FFF)-1;    // Use bgezal statement containing data length.
		data+=2;
		while(0<datalen){
			datalen--;
			len=data[0]>>16;
			sound=data[0]&0x0000FFFF;
			data++;
			if (len) {
				// Shift tone for 2048 <--> ~440 Hz.
				// 2048 must be changed to 1705
				//    x 1705 / 2048 = 213 / 256 + 1 / 2048
				sound=213*(sound>>8)+(sound>>11);
				g_sound[pos]=sound-1;
				g_soundlen[pos]=len;
				pos++;
				if (32<pos) {
					err_music("Sound data too long.");
					return;
				}
			} else {
				g_soundrepeat=sound;
				break;
			}
		}
	} while(len);
	g_soundend=pos;
	g_soundwait=g_soundlen[0];
	IEC0bits.T4IE=1; // Restart interrupt.
	g_music_active=1;// Activate music system
}

void set_music(char* str, int flagsLR){
	// flagsLR is ignored
	char b;
	unsigned long tone,tonenatural;
	int len;
	g_mstr=str;
	g_mspos=0;
	while(0<(b=g_mstr[g_mspos])){
		if (g_mstr[g_mspos+1]==':') {
			// Set property
			g_mspos+=2;
			switch(b){
				case 'L':
					musicSetL();
					break;
				case 'Q':
					musicSetQ();
					break;
				case 'K':
					musicSetK();
					break;
				case 'M':
					musicSetM();
					break;
				default:
					err_music(str);
					break;
			}	
		} else if ('A'<=b && b<='G' || 'a'<=b && b<='g' || b=='z') {
			g_mspos++;
			if (b=='z') {
				tone=0;
			} else if (b<='G') {
				tone=g_tones[b-'A'];
				tonenatural=g_keys[b-'A'+49];
			} else {
				tone=g_tones[b-'a']>>1;
				tonenatural=g_keys[b-'a'+49]>>1;
			}
			// Check "'"s
			while(g_mstr[g_mspos]=='\''){
				g_mspos++;
				tone>>=1;
			}
			// Check ","s
			while(g_mstr[g_mspos]==','){
				g_mspos++;
				tone<<=1;
				tonenatural<<=1;
			}
			// Check "^","=","_"
			switch(g_mstr[g_mspos]){
				case '^':
					g_mspos++;
					tone=toneSharp(tone);
					break;
				case '_':
					g_mspos++;
					tone=toneFlat(tone);
					break;
				case '=':
					g_mspos++;
					tone=tonenatural;
					break;
				default:
					break;
			}
			// Check number for length
			b=g_mstr[g_mspos];
			if ('0'<=b && b<='9') {
				len=g_lvalue*musicGetNum();
			} else {
				len=g_lvalue;
			}
			if (g_mstr[g_mspos]=='/') {
				g_mspos++;
				len=len/musicGetNum();
			}
			// Shift tone if graphic is used
			if (g_use_graphic) {
				// MUL_15 instead of MUL_16 for CPU clock.
				// tone=tone*15/16
				tone-=(tone+7)>>4;
			}
			// Update music value array
			IEC0bits.T4IE=0; // Stop interruption, first.
			if (g_musicstart==g_musicend) {
				g_musicwait=len;
			}
			g_music[g_musicend]=(tone-1)&0x0000FFFF;
			g_musiclen[g_musicend]=len;
			g_musicend++;
			g_musicend&=31;
			IEC0bits.T4IE=1; // Restart interruption.
		} else {
			err_music(str);
		}
		// Go to next character
		while(0<g_mstr[g_mspos] && g_mstr[g_mspos]<=0x20 || g_mstr[g_mspos]=='|') g_mspos++;
	}
	g_music_active=1;// Activate music system
}

/*
	PLAYWAVE routines follow
*/

int checkChars(char* str1, char* str2, int num){
	int i;
	for(i=0;i<num;i++){
		if (str1[i]!=str2[i]) return 1;
	}
	return 0;
}

void init_dma_music(){
	// Timer4 for 15700 Hz
	T4CON=0x0000; // Not start yet
	if (g_use_graphic) {
		PR4=3405-1;
	} else {
		PR4=3632-1;
	}
	TMR4=PR4-1;

	// Timer3 for PWM
	TMR3=0;
	PR3=0x100;
	T3CON=0x8000;

	// OC4 setting
	RPA2R=5;         //Use RPA2 for OC4
	OC4RS=0x80;
	OC4CON=0x000e;
	OC4CONSET=0x8000;

	//DMA2 settings for OC4
	DMACONSET=0x8000;
	DCH2CON=0x00000012;  // CHBUSY=0, CHCHNS=0, CHEN=0, CHAED=0, CHCHN=0, CHAEN=1, CHEDET=0, CHPRI=b10
	DCH2ECON=0x1310;     // CHAIRQ=0, CHSIRQ=19, CFORCE=0, CABRT=0, PATEN=0, SIRQEN=1, AIRQEN=0
	                     // CHSIRQ=19: Timer4 interrupt
	DCH2SSA=((unsigned int)&(g_wavtable[0]))&0x1fffffff;
	DCH2DSA=0x1F803620; // OC4RS
	DCH2SSIZ=524;
	DCH2DSIZ=1;
	DCH2CSIZ=1;
	DCH2INTCLR=0x00FF00FF;
	DCH2CONSET=0x00000080;
}

void play_wave(char* filename, int start){
	int i;
	// Stop the previous play
	stop_music();
	// Exit function if null filename
	if (filename[0]==0x00) {
		return;
	}
	// Alocate 524 byte buffer if not assigned
	if (g_var_size[ALLOC_WAVE_BLOCK]==0) {
		g_wavtable=(char*)alloc_memory(524/4,ALLOC_WAVE_BLOCK);
	}
	// Open file
	if (g_fhandle) FSfclose(g_fhandle);
	g_fhandle=FSfopen(filename,"r");
	if (!g_fhandle) err_file();
	// Read header and check if monaural 8 bit 16000 Hz.
	if (0x2c != FSfread((void*)&g_wavtable[0],1,0x2c,g_fhandle)) err_file();
	i=0;
	i+=checkChars((char*)&g_wavtable[0],"RIFF",4);                      // Check RIFF
	i+=checkChars((char*)&g_wavtable[8],"WAVEfmt ",8);                  // Check WAVE and fmt
	i+=checkChars((char*)&g_wavtable[16],"\x10\x00\x00\x00\x01\x00",6); // Check if liear PCM
	if (!checkChars((char*)&g_wavtable[22],"\x01\x00\x80\x3e\x00\x00\x80\x3e\x00\x00\x01\x00",12)) {
		// Monaural 16000 Hz
	} else if (!checkChars((char*)&g_wavtable[22],"\x01\x00\x54\x3d\x00\x00\x54\x3d\x00\x00\x01\x00",12)) {
		// Monaural 15700 Hz
	} else {
		i=1;
	}
	i+=checkChars((char*)&g_wavtable[34],"\x08\x00\x64\x61\x74\x61",6); // Check bit # and data
	if (i) err_wave();
	// Support defined start position here to skip file pointer here.
	FSfseek(g_fhandle, start, SEEK_CUR);
	// Read first 262 bytes.
	if (262 != FSfread((void*)&g_wavtable[0],1,262,g_fhandle)) err_file();

	// Initialize DMA
	init_dma_music();	
	g_sound_mode=SOUND_MODE_WAVE;

	g_music_active=1;// Activate music system
}
