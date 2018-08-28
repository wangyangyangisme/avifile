#include "audioencoder.h"
#include "avm_output.h"
#include "plugin.h"
#include "fillplugins.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern "C"
{
#include "lame3.70/lame.h"
}

AVM_BEGIN_NAMESPACE;

PLUGIN_TEMP(mp3lame_audioenc)

class MP3Encoder: public IAudioEncoder
{
    lame_global_flags gf;
    WAVEFORMATEX in_fmt;

    struct __attribute__((__packed__)) strf_mp3
    {
	short wID;
	int fdwFlags;
	short  nBlockSize;
	short  nFramesPerBlock;
	short  nCodecDelay;
    };
public:
    MP3Encoder::MP3Encoder(const CodecInfo& info, const WAVEFORMATEX* format)
	:IAudioEncoder(info)
    {
	in_fmt=*format;
	lame_init(&gf);
	gf.silent=1;
	gf.padding_type=2;
	gf.VBR=0;
	gf.in_samplerate=format->nSamplesPerSec;
	gf.num_channels=format->nChannels;
	if (format->nChannels==1)
	    gf.mode=3;
	else
	    gf.mode=1;
	lame_init_params(&gf);
	//int mode;   /* 0,1,2,3 stereo,jstereo,dual channel,mono */
    }
    virtual int SetBitrate(int bitrate)
    {
	gf.brate=bitrate/125;
	lame_init_params(&gf);
	AVM_WRITE("Lame MP3 encoder", "Setting bit rate to %d (%dkbps)\n", bitrate, gf.brate);
	return 0;
    }
    virtual int SetQuality(int quality)
    {
	gf.quality=quality;
	lame_init_params(&gf);
	return 0;
    }
    virtual uint_t GetFormat(void* extension = 0, uint_t size = 0) const
    {
	if (!extension)
	    return 30;
	if (size < 30)
	    return 0;
	WAVEFORMATEX wf;
	strf_mp3 mp3extra;
	memcpy(&wf, &in_fmt, 18);
	wf.wFormatTag=0x55;
	wf.nAvgBytesPerSec=gf.brate*125;
	wf.nBlockAlign=1;
	wf.wBitsPerSample=0;
	wf.cbSize=12;
	memcpy(extension, &wf, 18);

	mp3extra.wID             = 1;        //
	mp3extra.fdwFlags        = 2;        // These values
	mp3extra.nBlockSize      = gf.framesize;
	// based on an
	mp3extra.nFramesPerBlock = 1;        // old Usenet post!!
	mp3extra.nCodecDelay     = 1393;     //
	memcpy((char*)extension+18, &mp3extra, 12);

	return 30;
    }
    virtual int Start()
    {
	lame_init_params(&gf);
	return 0;
    }
    virtual int Close(void* out_data, uint_t out_size, uint_t* size_read)
    {
	char buffer[7200];
	//    printf("MP3Encoder::Close()\n");
	uint_t bytes=lame_encode_finish(&gf, buffer, sizeof(buffer));
	if (out_size<bytes)
	    bytes = out_size;
	if (out_data)
	    memcpy(out_data, buffer, bytes);
	if (out_data && size_read)
	    *size_read = bytes;
	return 0;
    }
    virtual int Convert(const void* in_data, uint_t in_size,
			void* out_data, uint_t out_size,
			uint_t* size_read, uint_t* size_written)
    {
#warning FIXME 8-bit?
	int result;
	if(in_fmt.nChannels==1)
	    result=lame_encode_buffer(&gf, (short*)in_data, (short*)in_data,
				      in_size, (char*)out_data, out_size);
	else
	    result=lame_encode_buffer_interleaved(&gf, (short*)in_data,
						  in_size, (char*)out_data, out_size);
	if(result<0)result=0;
	if(size_read)*size_read=in_size;
	if(size_written)*size_written=result;
	return 0;
    }
};

static IAudioEncoder* mp3lame_CreateAudioEncoder(const CodecInfo& info, fourcc_t fourcc, const WAVEFORMATEX* fmt)
{
    return new MP3Encoder(info, fmt);
}

AVM_END_NAMESPACE;

extern "C" avm::codec_plugin_t avm_codec_plugin_mp3lame_audioenc;

avm::codec_plugin_t avm_codec_plugin_mp3lame_audioenc =
{
    PLUGIN_API_VERSION,

    0, // err
    0, 0, 0, 0, 0, 0, // attrs
    avm::mp3lame_FillPlugins,
    0,
    avm::mp3lame_CreateAudioEncoder,
    0,
    0,
};
