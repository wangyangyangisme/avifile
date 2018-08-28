/* MPEG/WAVE Sound library

   (C) 1997 by Jung woo-jae */

// Mpegtoraw.cc
// Server which get mpeg format and put raw format.

#include "fillplugins.h"
#include "mpegsound.h"
#include "avm_output.h"
#include "plugin.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

AVM_BEGIN_NAMESPACE;

PLUGIN_TEMP(mpeg_audiodec);

#define MY_PI 3.14159265358979323846

#ifndef WORDS_BIGENDIAN
#define _KEY 0
#else
#define _KEY 3
#endif

int Mpegtoraw::getbits(int bits)
{
  int current;
  int bi;

  if(!bits)return 0;

  current=0;
  bi=(bitindex&7);

  current = buffer[bitindex>>3]<<bi & 0xff;
  bi=8-bi;
  bitindex+=bi;

  while(bits)
  {
    if(!bi)
    {
      current |= buffer[bitindex>>3] & 0xff;
      bitindex+=8;
      bi=8;
    }

    if(bits>=bi)
    {
      current<<=bi;
      bits-=bi;
      bi=0;
    }
    else
    {
      current<<=bits;
      bi-=bits;
      bits=0;
    }
  }
  bitindex-=bi;

  return (current>>8);
}

void Mpegtoraw::setforcetomono(bool flag)
{
    forcetomonoflag=flag;
}

void Mpegtoraw::setdownfrequency(int value)
{
    downfrequency=0;
    if(value)downfrequency=1;
}

bool Mpegtoraw::getforcetomono(void)
{
    return forcetomonoflag;
}

int Mpegtoraw::getdownfrequency(void)
{
    return downfrequency;
}

int Mpegtoraw::getpcmperframe(void)
{
    int s;

    s=32;
    if(layer==3)
    {
	s*=18;
	if(version==0)s*=2;
    }
    else
    {
	s*=SCALEBLOCK;
	if(layer==2)s*=3;
    }

    return s;
}

int Mpegtoraw::flushrawdata(void)
{
    //  player->putblock((char *)rawdata,rawdataoffset<<1);

    if (rawdataoffset<<1 == 0) {
	return -2;
    }

    if(local_dest_buffer_size<(rawdataoffset<<1))
	return -1;
    memcpy(local_dest_buffer, rawdata, rawdataoffset<<1);
    local_dest_buffer+=rawdataoffset<<1;
    local_dest_buffer_size-=rawdataoffset<<1;
    currentframe++;
    rawdataoffset=0;
    return 0;
};

typedef struct
{
    char *songname;
    char *musican;
    char *type;
    char *year;
    char *etc;
}ID3;

static void strman(char *str,int max)
{
    int i;

    str[max]=0;

    for(i=max-1;i>=0;i--)
	if(((unsigned char)str[i])<26 || str[i]==' ')str[i]=0; else break;
}
/*
inline void parseID3(Soundinputstream *fp,ID3 *data)
{
  int c,flag=0;

  fp->setposition(fp->getsize()-1000);

  data->songname[0]=0;
  data->musican [0]=0;
  data->type    [0]=0;
  data->year    [0]=0;
  data->etc     [0]=0;

  for(;;)
  {
    if((c=fp->getbytedirect())<0)break;
    else if(c==0xFF)flag++;
    else if(c==0x54 && flag>=28)
    {
      if(fp->getbytedirect()==0x41)
      {
        if(fp->getbytedirect()==0x47)
        {
	  fp->_readbuffer(data->songname,30);strman(data->songname,30);
	  fp->_readbuffer(data->musican ,30);strman(data->musican, 30);
	  fp->_readbuffer(data->type    ,30);strman(data->type,    30);
	  fp->_readbuffer(data->year    , 7);strman(data->year,     7);
	  fp->_readbuffer(data->etc     ,25);strman(data->etc,     25);
          break;
        }
      }
      flag=0;
    }
    else flag=0;
  }

  fp->setposition(0);
}
*/

// Convert mpeg to raw
// Mpeg headder class
Mpegtoraw::Mpegtoraw(const CodecInfo& info, const WAVEFORMATEX* fmt)
    :IAudioDecoder(info, fmt), gain_add(8), flushed(true), lastfrequency(0)
{
    static bool initialized=false;

    __errorcode=SOUND_ERROR_OK;
    //  frameoffsets=NULL;

    forcetomonoflag=false;
    downfrequency=0;

    int i;
    REAL *s1,*s2;
    REAL *s3,*s4;

    scalefactor=SCALE;
    calcbufferoffset=15;
    currentcalcbuffer=0;

    s1=calcbufferL[0];s2=calcbufferR[0];
    s3=calcbufferL[1];s4=calcbufferR[1];
    for(i=CALCBUFFERSIZE-1;i>=0;i--)
	calcbufferL[0][i]=calcbufferL[1][i]=
	    calcbufferR[0][i]=calcbufferR[1][i]=0.0;

    if(!initialized)
    {
	for(i=0;i<16;i++)hcos_64[i]=1.0/(2.0*cos(MY_PI*double(i*2+1)/64.0));
	for(i=0;i< 8;i++)hcos_32[i]=1.0/(2.0*cos(MY_PI*double(i*2+1)/32.0));
	for(i=0;i< 4;i++)hcos_16[i]=1.0/(2.0*cos(MY_PI*double(i*2+1)/16.0));
	for(i=0;i< 2;i++)hcos_8 [i]=1.0/(2.0*cos(MY_PI*double(i*2+1)/ 8.0));
	hcos_4=1.0/(2.0*cos(MY_PI*1.0/4.0));
	initialized=true;
    }

    currentframe=decodeframe=0;

    /*
     if(loadheader())
     {
     totalframe=(loader->getsize()+framesize-1)/framesize;
     loader->setposition(0);
     }
     else totalframe=0;

     if(frameoffsets)delete [] frameoffsets;

     song.name[0]=0;
     if(totalframe>0)
     {
     frameoffsets=new int[totalframe];
     for(i=totalframe-1;i>=0;i--)
     frameoffsets[i]=0;

     {
     ID3 data;

     data.songname=song.name;
     data.musican =song.musican;
     data.type    =song.type;
     data.year    =song.year;
     data.etc     =song.etc;
     parseID3(loader,&data);
     }
     }
     else frameoffsets=NULL;
     */
};

Mpegtoraw::~Mpegtoraw()
{
    //  if(frameoffsets)delete [] frameoffsets;
}



/*
 void Mpegtoraw::setframe(int framenumber)
 {
 int pos=0;

 if(frameoffsets==NULL)return;
 if(framenumber==0)pos=frameoffsets[0];
 else
 {
 if(framenumber>=totalframe)framenumber=totalframe-1;
 pos=frameoffsets[framenumber];
 if(pos==0)
 {
 int i;

 for(i=framenumber-1;i>0;i--)
 if(frameoffsets[i]!=0)break;

 loader->setposition(frameoffsets[i]);

 while(i<framenumber)
 {
 loadheader();
 i++;
 frameoffsets[i]=loader->getposition();
 }
 pos=frameoffsets[framenumber];
 }
 }

 clearbuffer();
 loader->setposition(pos);
 decodeframe=currentframe=framenumber;
 }
 */
/*
 void Mpegtoraw::clearbuffer(void)
 {
 player->abort();
 player->resetsoundtype();
 }*/

int Mpegtoraw::Convert(const void* input_buffer, uint_t in_size,
		       void* output_buffer, uint_t out_size,
		       uint_t* size_read, uint_t* size_written)
{
    int ret;

    local_src_buffer=(uint8_t*)input_buffer;
    local_dest_buffer=(uint8_t*)output_buffer;
    local_src_buffer_size=in_size;
    local_dest_buffer_size=out_size;

    if (flushed)
    {
	layer3initialize();
	clearrawdata();
	flushrawdata();
    }

    // prevent deadlock in this loop!!!
    int maxloop;
    for (maxloop = 0; maxloop < 20; maxloop++)
    {
	//printf("Maxloop  %d\n", maxloop);
	const unsigned char* rest_lb=local_src_buffer;
	unsigned rest_lc=local_src_buffer_size;
	unsigned rest_dc=local_dest_buffer_size;

	if (loadheader()==false)
	{
	    local_src_buffer=rest_lb;
	    local_src_buffer_size=rest_lc;
	    local_dest_buffer_size=rest_dc;
	    break;
	}

	if (frequency!=lastfrequency)
	{
	    if(lastfrequency>0)seterrorcode(SOUND_ERROR_BAD);
	    lastfrequency=frequency;
	}

	decodeframe++;

	if     (layer==3)extractlayer3();
	else if(layer==2)extractlayer2();
	else if(layer==1)extractlayer1();
	ret = flushrawdata();
	//printf("WRIT out: %d  in: %d  %d\n", out_size-local_dest_buffer_size, in_size - local_src_buffer_size, ret);
	if(ret==-1) /* OutputBuffer Is Full!! */
	{
	    //printf("flushrawdata() failed: restoring %d/%d\n", rest_lc, rest_dc);
	    //        local_src_buffer=rest_lb;
	    //	local_src_buffer_size=rest_lc;
	    //	local_dest_buffer_size=rest_dc;
	    //	declined=*(int*)local_src_buffer;
	    /*        FILE* zz=fopen("./log.mp3", "ab");
	     fwrite(input_buffer, in_size-local_src_buffer_size, 1, zz);
	     fclose(zz);
	     zz=fopen("./log.raw", "ab");
	     fwrite(output_buffer, out_size-local_dest_buffer_size, 1, zz);
	     fclose(zz);*/
	    break;
	}
	else if (ret == -2)
	{
	    /* Convert Error? Clear All OutputData */
	    local_src_buffer=(uint8_t*)input_buffer;
	    local_src_buffer_size=in_size;
	    local_dest_buffer=(uint8_t*)output_buffer;
	    local_dest_buffer_size=out_size;
	    clearrawdata();
            continue;
	}
        break;
    }

    //printf("done  l:%d %d   %d\n", maxloop, in_size - local_src_buffer_size, out_size-local_dest_buffer_size );
    flushed = false;

    if (size_read)
	*size_read = in_size - local_src_buffer_size;
    int written = out_size - local_dest_buffer_size;
    if (size_written)
	*size_written = written;

    //printf(" JO %d  %d\n ", *size_written, *size_read);
    return (written) ? 0 : -1;
}

bool Mpegtoraw::loadheader(void)
{
    //    unsigned char* rest_lb=local_src_buffer;
    //    unsigned rest_lc=local_src_buffer_size;
    register int c;
    bool flag;
    unsigned int freqs;

    sync();

again:

    // Synchronize
    flag=false;
    do
    {
	if((c=getbytedirect())<0)break;

	if(c==0xff)
	    while(!flag)
	    {
		if((c=getbytedirect())<0)
		{
		    flag=true;
		    break;
		}
		if((c&0xe0)==0xe0)
		{
		    flag=true;
		    break;
		}
		else if(c!=0xff)break;
	    }
    }while(!flag);

    //c=getbytedirect();
    if(c<0)
    {
	return false;
    }
    // Analyzing
    if (c&0x10) {
	mpeg25=false;
    }
    else {
	mpeg25=true;
	c=c+0x10;
    }

    if ((c & 0xf0) != 0xf0) {
	goto again;
    }

    c&=0xf;
    protection=c&1;
    layer=4-((c>>1)&3);
    version=(_mpegversion)((c>>3)^1);

    //if ((m_pFormat->wFormatTag == 0x55) && (layer!=3)) goto again;

    c=((getbytedirect()))>>1;
    padding=(c&1);             c>>=1;
    frequency=(_frequency)(c&3); c>>=2;
    bitrateindex=(int)c;
    if(bitrateindex==15) {
	goto again;
    }

    if (mpeg25) freqs=frequencies[2][frequency];
    else        freqs=frequencies[version][frequency];

    if (freqs != m_pFormat->nSamplesPerSec) {
	goto again;
    }

    c=((unsigned int)(getbytedirect()))>>4;
    extendedmode=c&3;
    mode=(_mode)(c>>2);


    // Making information
    inputstereo= (mode==single)?0:1;
    if(forcetomonoflag)outputstereo=0; else outputstereo=inputstereo;

    /*  if(layer==2)
     if((bitrateindex>=1 && bitrateindex<=3) || (bitrateindex==5)) {
     if(inputstereo)return seterrorcode(SOUND_ERROR_BAD); }
     else if(bitrateindex==11 && mode==single)
     return seterrorcode(SOUND_ERROR_BAD); */

    channelbitrate=bitrateindex;
    if(inputstereo)
	if(channelbitrate==4)channelbitrate=1;
	else channelbitrate-=4;

    if(channelbitrate==1 || channelbitrate==2)tableindex=0; else tableindex=1;

    if(layer==1)subbandnumber=MAXSUBBAND;
    else
    {
	if(!tableindex)
	    if(frequency==frequency32000)subbandnumber=12; else subbandnumber=8;
	else if(frequency==frequency48000||
		(channelbitrate>=3 && channelbitrate<=5))
	    subbandnumber=27;
	else subbandnumber=30;
    }

    if(mode==single)stereobound=0;
    else if(mode==joint)stereobound=(extendedmode+1)<<2;
    else stereobound=subbandnumber;
    if(frequency==3) {
	goto again;
    }
    if(stereobound>subbandnumber)stereobound=subbandnumber;
    //    printf("Layer %d, mode %d, frequency %d\n", layer, mode, frequency);
    // framesize & slots
    if(layer==1)
    {
	framesize=(12000*bitrate[version][0][bitrateindex])/
	    frequencies[version][frequency];
	if(frequency==frequency44100 && padding)framesize++;
	framesize<<=2;
    }
    else
    {
	if(mpeg25)
	{
	    framesize=(144000*bitrate[version][layer-1][bitrateindex])/
		(frequencies[2][frequency]<<version);
	}
	else
	{
	    framesize=(144000*bitrate[version][layer-1][bitrateindex])/
		(frequencies[version][frequency]<<version);
	}
	if(padding)framesize++;
	if(layer==3)
	{
	    if(version)
		layer3slots=framesize-((mode==single)?9:17)
		    -(protection?0:2)
		    -4;
	    else
		layer3slots=framesize-((mode==single)?17:32)
		    -(protection?0:2)
		    -4;
	}
    }
#ifdef DEBUG
    fprintf(stderr, "MPEG %d audio layer %d (%d kbps), at %d Hz %s [%d]\n", version+1, layer,
	    bitrate[version][layer-1][bitrateindex],
	    (mpeg25) ? frequencies[2][frequency] : frequencies[version][frequency],
	    (mode == single) ? "mono" : "stereo", framesize);
#endif

    if(!fillbuffer(framesize-4))
    {
	seterrorcode(SOUND_ERROR_FILEREADFAIL);
	return false;
    }

    if(!protection)
    {
	getbyte();                      // CRC, Not check!!
	getbyte();
    }

    if(eof())
	return false;
    return true;
}

const avm::vector<AttributeInfo>& Mpegtoraw::GetAttrs() const
{
    return m_Info.decoder_info;
}
int Mpegtoraw::GetValue(const char* name, int* value) const
{
    if (strcmp(name, mpegadstr_gain) == 0)
    {
	*value = gain_add;
	return 0;
    }
    return -1;
}
int Mpegtoraw::SetValue(const char* name, int value)
{
    if (strcmp(name, mpegadstr_gain) == 0)
    {
	gain_add = value;
        return 0;
    }
    return -1;
}

static IAudioDecoder* mpeg_audiodec_CreateAudioDecoder(const CodecInfo& info, const WAVEFORMATEX* format)
{
    return new Mpegtoraw(info, format);
}

AVM_END_NAMESPACE;

extern "C" avm::codec_plugin_t avm_codec_plugin_mpeg_audiodec;

avm::codec_plugin_t avm_codec_plugin_mpeg_audiodec =
{
    PLUGIN_API_VERSION,

    0, // err
    0, 0,
    avm::PluginGetAttrInt,
    avm::PluginSetAttrInt,
    0, 0, // attrs
    avm::mpeg_audiodec_FillPlugins,
    avm::mpeg_audiodec_CreateAudioDecoder,
    0,
    0,
    0,
};
