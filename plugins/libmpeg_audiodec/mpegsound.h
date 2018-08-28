/* MPEG/WAVE Sound library

(C) 1997 by Woo-jae Jung */

// Mpegsound.h
//   This is typeset for functions in MPEG/WAVE Sound library.
//   Now, it's for only linux-pc-?86

/************************************/
/* Include default library packages */
/************************************/

#ifndef _L__SOUND__
#define _L__SOUND__

#include "audiodecoder.h"
#include <stdio.h>
#include <sys/types.h>

AVM_BEGIN_NAMESPACE;

/****************/
/* Sound Errors */
/****************/
// General error
#define SOUND_ERROR_OK                0
#define SOUND_ERROR_FINISH           -1

// Device error (for player)
#define SOUND_ERROR_DEVOPENFAIL       1
#define SOUND_ERROR_DEVBUSY           2
#define SOUND_ERROR_DEVBADBUFFERSIZE  3
#define SOUND_ERROR_DEVCTRLERROR      4

// Sound file (for reader)
#define SOUND_ERROR_FILEOPENFAIL      5
#define SOUND_ERROR_FILEREADFAIL      6

// Network
#define SOUND_ERROR_UNKNOWNPROXY      7
#define SOUND_ERROR_UNKNOWNHOST       8
#define SOUND_ERROR_SOCKET            9
#define SOUND_ERROR_CONNECT          10
#define SOUND_ERROR_FDOPEN           11
#define SOUND_ERROR_HTTPFAIL         12
#define SOUND_ERROR_HTTPWRITEFAIL    13
#define SOUND_ERROR_TOOMANYRELOC     14

// Miscellneous (for translater)
#define SOUND_ERROR_MEMORYNOTENOUGH  15
#define SOUND_ERROR_EOF              16
#define SOUND_ERROR_BAD              17

#define SOUND_ERROR_THREADFAIL       18

#define SOUND_ERROR_UNKNOWN          19


/**************************/
/* Define values for MPEG */
/**************************/
#define SCALEBLOCK     12
#define CALCBUFFERSIZE 512
#define MAXSUBBAND     32
#define MAXCHANNEL     2
#define MAXTABLE       2
#define SCALE          32768
#define MAXSCALE       (SCALE-1)
#define MINSCALE       (-SCALE)
#define RAWDATASIZE    (2*2*32*SSLIMIT)

enum { LS = 0, RS = 1 };

#define SSLIMIT      18
#define SBLIMIT      32

#define WINDOWSIZE    4096

// Huffmancode
#define HTN 34


/*******************************************/
/* Define values for Microsoft WAVE format */
/*******************************************/
#define RIFF		0x46464952
#define WAVE		0x45564157
#define FMT		0x20746D66
#define DATA		0x61746164
#define PCM_CODE	1
#define WAVE_MONO	1
#define WAVE_STEREO	2

#define MODE_MONO   0
#define MODE_STEREO 1

/********************/
/* Type definitions */
/********************/
typedef float REAL;
#if defined(__sun__) && defined(__svr4__)
typedef unsigned int u_int32_t;
typedef unsigned short u_int16_t;
#endif

typedef struct _waveheader {
    u_int32_t     main_chunk;  // 'RIFF'
    u_int32_t     length;      // filelen
    u_int32_t     chunk_type;  // 'WAVE'

    u_int32_t     sub_chunk;   // 'fmt '
    u_int32_t     sc_len;      // length of sub_chunk, =16
    u_int16_t     format;      // should be 1 for PCM-code
    u_int16_t     modus;       // 1 Mono, 2 Stereo
    u_int32_t     sample_fq;   // frequence of sample
    u_int32_t     byte_p_sec;
    u_int16_t     byte_p_spl;  // samplesize; 1 or 2 bytes
    u_int16_t     bit_p_spl;   // 8, 12 or 16 bit

    u_int32_t     data_chunk;  // 'data'
    u_int32_t     data_length; // samplecount
}WAVEHEADER;

typedef struct
{
    bool         generalflag;
    unsigned int part2_3_length;
    unsigned int big_values;
    unsigned int global_gain;
    unsigned int scalefac_compress;
    unsigned int window_switching_flag;
    unsigned int block_type;
    unsigned int mixed_block_flag;
    unsigned int table_select[3];
    unsigned int subblock_gain[3];
    unsigned int region0_count;
    unsigned int region1_count;
    unsigned int preflag;
    unsigned int scalefac_scale;
    unsigned int count1table_select;
}layer3grinfo;

typedef struct
{
    unsigned main_data_begin;
    unsigned private_bits;
    struct
    {
	unsigned scfsi[4];
	layer3grinfo gr[2];
    }ch[2];
}layer3sideinfo;

typedef struct
{
    int l[23];            /* [cb] */
    int s[3][13];         /* [window][cb] */
}layer3scalefactor;     /* [ch] */

typedef struct
{
    int tablename;
    unsigned int xlen,ylen;
    unsigned int linbits;
    unsigned int treelen;
    const unsigned int (*val)[2];
}HUFFMANCODETABLE;

/*********************************/
/* Sound input interface classes */
/*********************************/
// Superclass for Inputbitstream // Yet, Temporary




// Class for Mpeg layer3
class Mpegbitwindow
{
public:
    Mpegbitwindow(){bitindex=point=0;};

    void initialize(void)  {bitindex=point=0;};
    int  gettotalbit(void) const {return bitindex;};
    void putbyte(int c)    {buffer[point&(WINDOWSIZE-1)]=c;point++;};
    void wrap(void) {
	int p=bitindex>>3;
	point&=(WINDOWSIZE-1);

	if(p>=point)
	{
	    for(register int i=4;i<point;i++)
		buffer[WINDOWSIZE+i]=buffer[i];
	}
	*((int *)(buffer+WINDOWSIZE))=*((int *)buffer);
    }
    void rewind(int bits)  {bitindex-=bits;};
    void forward(int bits) {bitindex+=bits;};
    int getbits(int bits);

    int getbit(void)
    {
	//  register int r=(buffer[(bitindex>>3)&(WINDOWSIZE-1)]>>(7-(bitindex&7)))&1;
	register int r=(buffer[bitindex>>3]>>(7-(bitindex&7)))&1;
	bitindex++;
	return r;
    }
    int getbits9(int bits)
    {
	register unsigned short a;

	//#ifndef WORDS_BIGENDIAN
	{
	    //    int offset=(bitindex>>3)&(WINDOWSIZE-1);
	    int offset=bitindex>>3;

	    a = (unsigned char)buffer[offset] << 8;
	    a |= (unsigned char)buffer[offset+1];
	}
	//#else
	//wrong  a=((unsigned short *)(buffer+((bixindex>>3)&(WINDOWSIZE-1))));
	//  a=*((unsigned short *)(buffer+((bitindex>>3))));
	//#endif

	a<<=(bitindex&7);
	bitindex+=bits;
	return (int)((unsigned int)(a>>(16-bits)));
    }



private:
    int  point,bitindex;
    char buffer[2*WINDOWSIZE];
};
// Class for converting mpeg format to raw format
class Mpegtoraw : public IAudioDecoder, public IRtConfig
{
private:
    /*********************************/
    /* this was added for AVI player */
    /*********************************/
    const unsigned char* local_src_buffer;
    int local_src_buffer_size;
    unsigned char* local_dest_buffer;
    int local_dest_buffer_size;
    int gain_add;
    bool flushed;
public:
    /*  returns number of bytes written to output_buffer */
    /*  input_size contains number of bytes used in input_buffer */
    /*  if format!=0, writes info about audio stream into it */
    /*  output_buffer MUST be large enough !!! */
    int Convert(const void* in_data, uint_t in_size,
		void* out_data, uint_t out_size,
		uint_t* size_read, uint_t* size_written);
    void Flush() { flushed = true; }
    IRtConfig* GetRtConfig() { return this; }
    const avm::vector<AttributeInfo>& GetAttrs() const;
    int GetValue(const char* name, int* value) const;
    int SetValue(const char* name, int value);

    // this mpeg lib seems to have problems whitout this
    //uint_t GetMinSize() const { return IAudioDecoder::GetMinSize() * 2; }

    /*****************************/
    /* Constant tables for layer */
    /*****************************/

private:
    static const int bitrate[2][3][15],frequencies[3][3];
    static const REAL scalefactorstable[64];
    static const HUFFMANCODETABLE ht[HTN];
    static const REAL filter[512];
    static REAL hcos_64[16],hcos_32[8],hcos_16[4],hcos_8[2],hcos_4;

    /*************************/
    /* MPEG header variables */
    /*************************/
private:
    int layer,protection,bitrateindex,padding,extendedmode;
    enum _mpegversion  {mpeg1,mpeg2}                               version;
    enum _mode    {fullstereo,joint,dual,single}                   mode;
    enum _frequency {frequency44100,frequency48000,frequency32000} frequency;

    /*******************************************/
    /* Functions getting MPEG header variables */
    /*******************************************/
public:
    // General
    int  getversion(void)   const {return version;};
    int  getlayer(void)     const {return layer;};
    bool getcrccheck(void)  const {return (!protection);};
    // Stereo or not
    int  getmode(void)      const {return mode;};
    bool isstereo(void)     const {return (getmode()!=single);};
    // Frequency and bitrate
    int  getfrequency(void) const {return frequencies[version][frequency];};
    int  getbitrate(void)   const {return bitrate[version][layer-1][bitrateindex];};

    /***************************************/
    /* Interface for setting music quality */
    /***************************************/
private:
    bool forcetomonoflag;
    int  downfrequency;

public:
    void setforcetomono(bool flag);
    void setdownfrequency(int value);

    /******************************************/
    /* Functions getting other MPEG variables */
    /******************************************/
public:
    bool getforcetomono(void);
    int  getdownfrequency(void);
    int  getpcmperframe(void);

    /******************************/
    /* Frame management variables */
    /******************************/
private:
    int currentframe,totalframe;
    int decodeframe;
    //  int *frameoffsets;

    /******************************/
    /* Frame management functions */
    /******************************/
public:
    int  getcurrentframe(void) const {return currentframe;};
    int  gettotalframe(void)   const {return totalframe;};
    void setframe(int framenumber);

    /***************************************/
    /* Variables made by MPEG-Audio header */
    /***************************************/
private:
    int tableindex,channelbitrate;
    int stereobound,subbandnumber,inputstereo,outputstereo;
    REAL scalefactor;
    int framesize;
    bool mpeg25;

    /********************/
    /* Song information */
    /********************/
    /*
     private:
     struct
     {
     char name   [30+1];
     char musican[30+1];
     char type   [30+1];
     char year   [ 7+1];
     char etc    [25+1];
     }song;
     */
    /* Song information functions */
    /*
     const char *getsongname(void) const { return (const char *)song.name;   };
     const char *getmusican (void) const { return (const char *)song.musican;};
     const char *getsongtype(void) const { return (const char *)song.type;   };
     const char *getsongyear(void) const { return (const char *)song.year;   };
     const char *getsongetc (void) const { return (const char *)song.etc;    };
     */
    /*******************/
    /* Mpegtoraw class */
    /*******************/
public:
    Mpegtoraw(const CodecInfo&, const WAVEFORMATEX*);
    ~Mpegtoraw();
    bool run(int frames);
    int  geterrorcode(void) {return __errorcode;};
    void clearbuffer(void);

private:
    int __errorcode;
    bool seterrorcode(int errorno){__errorcode=errorno;return false;};

    /*****************************/
    /* Loading MPEG-Audio stream */
    /*****************************/
private:
    //  Soundinputstream *loader;   // Interface
    union
    {
	unsigned char store[4];
	unsigned int  current;
    }u;
    //  char buffer[4096];
    const unsigned char* buffer;
    int  bitindex;
    bool fillbuffer(unsigned size)
    {
	bitindex=0;
	if(local_src_buffer_size<(int)size)
	    return false;
	else
	{
	    buffer=local_src_buffer;
	    local_src_buffer+=size;
	    local_src_buffer_size-=size;
	    return true;
	}
    };
    void unwind(int z)
    {
	local_src_buffer_size+=z;
	local_src_buffer-=z;
    }
    void sync(void)  {bitindex=(bitindex+7)&0xFFFFFFF8;};
    bool issync(void){return (bitindex&7);};
    int getbits(int bits);
    int getbyte(void)
    {
	int r=(unsigned char)buffer[bitindex>>3];

	bitindex+=8;
	return r;
    }
    int getbits9(int bits)
    {
	register unsigned short a;
	//#ifndef WORDS_BIGENDIAN
	{
	    int offset=bitindex>>3;

	    a = (unsigned char)buffer[offset] << 8;
	    a |= (unsigned char)buffer[offset+1];
	}
	//#else
	//  a=*((unsigned short *)(buffer+((bitindex>>3))));
	//#endif

	a<<=(bitindex&7);
	bitindex+=bits;
	return (int)((unsigned int)(a>>(16-bits)));
    }
    int getbits8(void)
    {
	register unsigned short a;

	//#ifndef WORDS_BIGENDIAN
	{
	    int offset=bitindex>>3;

	    a = (unsigned char)buffer[offset] << 8;
	    a |= (unsigned char)buffer[offset+1];
	}
	//#else
	//  a=*((unsigned short *)(buffer+((bitindex>>3))));
	//#endif

	a<<=(bitindex&7);
	bitindex+=8;
	return (int)((unsigned int)(a>>8));
    }
    int getbit(void)
    {
	register int r=(buffer[bitindex>>3]>>(7-(bitindex&7)))&1;

	bitindex++;
	return r;
    }

    /********************/
    /* Global variables */
    /********************/
private:
    int lastfrequency,laststereo;

    // for Layer3
    int layer3slots,layer3framestart,layer3part2start;
    REAL prevblck[2][2][SBLIMIT][SSLIMIT];
    int currentprevblock;
    layer3sideinfo sideinfo;
    layer3scalefactor scalefactors[2];
    //#define MPEGNOINSTRUMENTFUNCTION  __attribute__ ((no_instrument_function))
#define MPEGNOINSTRUMENTFUNCTION
    Mpegbitwindow bitwindow;

    int wgetbit(void) MPEGNOINSTRUMENTFUNCTION   { return bitwindow.getbit(); }
    int wgetbits9(int bits) MPEGNOINSTRUMENTFUNCTION { return bitwindow.getbits9(bits); }
    int wgetbits (int bits) MPEGNOINSTRUMENTFUNCTION { return bitwindow.getbits (bits); }

    /*************************************/
    /* Decoding functions for each layer */
    /*************************************/
private:
    bool loadheader(void);

    //
    // Subbandsynthesis
    //
    REAL calcbufferL[2][CALCBUFFERSIZE],calcbufferR[2][CALCBUFFERSIZE];
    int  currentcalcbuffer,calcbufferoffset;

    void computebuffer(REAL *fraction,REAL buffer[2][CALCBUFFERSIZE]);
    void generatesingle(void);
    void generate(void) MPEGNOINSTRUMENTFUNCTION;
#undef MPEGNOINSTRUMENTFUNCTION
    void subbandsynthesis(REAL *fractionL,REAL *fractionR);

    void computebuffer_2(REAL *fraction,REAL buffer[2][CALCBUFFERSIZE]);
    void generatesingle_2(void);
    void generate_2(void);
    void subbandsynthesis_2(REAL *fractionL,REAL *fractionR);

    // Extractor
    void extractlayer1(void);    // MPEG-1
    void extractlayer2(void);
    void extractlayer3(void);
    void extractlayer3_2(void);  // MPEG-2


    // Functions for layer 3
    void layer3initialize(void);
    bool layer3getsideinfo(void);
    bool layer3getsideinfo_2(void);
    void layer3getscalefactors(int ch,int gr);
    void layer3getscalefactors_2(int ch);
    void layer3huffmandecode(int ch,int gr,int out[SBLIMIT][SSLIMIT]);
    void layer3dequantizesample(int ch,int gr,int   in[SBLIMIT][SSLIMIT],
				REAL out[SBLIMIT][SSLIMIT]);
    void layer3fixtostereo(int gr,REAL  in[2][SBLIMIT][SSLIMIT]);
    void layer3reorderandantialias(int ch,int gr,REAL  in[SBLIMIT][SSLIMIT],
				   REAL out[SBLIMIT][SSLIMIT]);

    void layer3hybrid(int ch,int gr,REAL in[SBLIMIT][SSLIMIT],
		      REAL out[SSLIMIT][SBLIMIT]);

    void huffmandecoder_1(const HUFFMANCODETABLE *h,int *x,int *y);
    void huffmandecoder_2(const HUFFMANCODETABLE *h,int *x,int *y,int *v,int *w);

    /********************/
    /* Playing raw data */
    /********************/
private:
    int getbytedirect()
    {
	local_src_buffer_size--;
	return (local_src_buffer_size>=0)?(*local_src_buffer++):(-1);
    }
    int eof() { return (local_src_buffer_size<0); }

    void clearrawdata(void)    { rawdataoffset=0; }
    void putraw(short int pcm) { rawdata[rawdataoffset++]=pcm; }
    int flushrawdata(void);

    int rawdataoffset;
    short int rawdata[RAWDATASIZE];
};

AVM_END_NAMESPACE;

#endif // _L__SOUND__
