#include "mad.h"
#include "fillplugins.h"
#include "audiodecoder.h"
#include "plugin.h"
#include "utils.h"
#include "avm_fourcc.h"
#include "avm_output.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

AVM_BEGIN_NAMESPACE;

PLUGIN_TEMP(mad_audiodec);

#define _
static char const *error_str(enum mad_error error)
{
    static char str[17];

    switch (error) {
    case MAD_ERROR_BUFLEN:
    case MAD_ERROR_BUFPTR:
	// some backward compatible
#ifdef MAD_ERROR_NONE
    case MAD_ERROR_NONE:
#endif // MAD_ERROR_NONE
	/* these errors are handled specially and/or should not occur */
	break;

    case MAD_ERROR_NOMEM:		return _("not enough memory");
    case MAD_ERROR_LOSTSYNC:		return _("lost synchronization");
    case MAD_ERROR_BADLAYER:		return _("reserved header layer value");
    case MAD_ERROR_BADBITRATE:		return _("forbidden bitrate value");
    case MAD_ERROR_BADSAMPLERATE:	return _("reserved sample frequency value");
    case MAD_ERROR_BADEMPHASIS:		return _("reserved emphasis value");
    case MAD_ERROR_BADCRC:		return _("CRC check failed");
    case MAD_ERROR_BADBITALLOC:		return _("forbidden bit allocation value");
    case MAD_ERROR_BADSCALEFACTOR:	return _("bad scalefactor index");
    case MAD_ERROR_BADFRAMELEN:		return _("bad frame length");
    case MAD_ERROR_BADBIGVALUES:	return _("bad big_values count");
    case MAD_ERROR_BADBLOCKTYPE:	return _("reserved block_type");
    case MAD_ERROR_BADSCFSI:		return _("bad scalefactor selection info");
    case MAD_ERROR_BADDATAPTR:		return _("bad main_data_begin pointer");
    case MAD_ERROR_BADPART3LEN:		return _("bad audio data length");
    case MAD_ERROR_BADHUFFTABLE:	return _("bad Huffman table select");
    case MAD_ERROR_BADHUFFDATA:		return _("Huffman data overrun");
    case MAD_ERROR_BADSTEREO:		return _("incompatible block_type for JS");
    default:;
    }

    sprintf(str, "error 0x%04x", error);
    return str;
}

class MAD_Decoder : public IAudioDecoder, public IRtConfig
{
    struct mad_stream stream;
    struct mad_frame frame;
    struct mad_synth synth;
    int m_iGain;
    bool m_bInitialized;

public:
    MAD_Decoder(const CodecInfo& info, const WAVEFORMATEX* wf)
	:IAudioDecoder(info, wf), m_iGain(8), m_bInitialized(false)
    {
	mad_stream_init(&stream);
	mad_frame_init(&frame);
        Flush();
    }
    ~MAD_Decoder()
    {
	mad_synth_finish(&synth);
	mad_frame_finish(&frame);
	mad_stream_finish(&stream);
    }
    virtual int Convert(const void* in_data, uint_t in_size,
			void* out_data, uint_t out_size,
			uint_t* size_read, uint_t* size_written)
    {
	mad_stream_buffer(&stream, (const unsigned char*) in_data, in_size);
	if (mad_frame_decode(&frame, &stream) == -1)
	{
	    //printf("mad_frame_decode: stream error: %s (harmless after seeking)\n", error_str(stream.error));
            Flush();
	}
	else
	{
	    if (!m_bInitialized)
	    {
		AVM_WRITE("MAD decoder", "MAD header MPEG Layer-%d %dHz %ldkbps\n",
			  frame.header.layer, frame.header.samplerate, frame.header.bitrate / 1000);
		m_bInitialized = true;
	    }
	    mad_synth_frame(&synth, &frame);
	    // Convert mad_fixed to int16_t
	    int16_t* samples = (int16_t*) out_data;
#if 0
	    printf("Synth %d  ch:%d  l:%d\n", synth.pcm.samplerate, synth.pcm.channels,
		   synth.pcm.length);

	    printf("Timer %d %f   mode: %d  \n",
		   frame.header.duration.seconds, 1/(double)frame.header.duration.fraction,
		   frame.header.mode);
#endif
	    for(int i = 0; i < synth.pcm.channels; i++)
	    {
		if (m_iGain != 8)
		{
		    for(int j = 0; j < synth.pcm.length; j++)
		    {
			mad_fixed_t sample = (int64_t) ((synth.pcm.samples[i][j] >> 6) * m_iGain) >> (MAD_F_FRACBITS - 16 - 2);
			//printf("%x  %d  ---  %x  %d\n", s1, s1, sample, sample);
			if (sample > 32767)
			{
			    //printf("SAMPLEBIG %d\n", sample);
			    sample = 32767;
			}
			else if (sample < -32768)
			{
			    //printf("SAMPLESMALL %d\n", sample);
			    sample = -32768;
			}
			samples[j * synth.pcm.channels + i] = (int16_t)(sample);
		    }
		}
		else
		{
		    for(int j = 0; j < synth.pcm.length; j++)
		    {
			int sample = synth.pcm.samples[i][j] >> (MAD_F_FRACBITS + 1 - 16);
#if 1
			if (sample > 32767)
			    sample = 32767;
			else if (sample < -32768)
			    sample = -32768;
#endif
			samples[j * synth.pcm.channels + i] = (int16_t)(sample);
		    }
		}
	    }
	}
	if (size_read)
	    *size_read = stream.next_frame - (const unsigned char*)in_data;
	if (size_written)
	    *size_written = 2 * synth.pcm.channels * synth.pcm.length;

	return 0;
    }
    virtual void Flush()
    {
	mad_frame_mute(&frame);
	mad_synth_init(&synth);
    }
    IRtConfig* GetRtConfig() { return this; }
    const avm::vector<AttributeInfo>& GetAttrs() const { return m_Info.decoder_info; }
    int GetValue(const char* name, int* value) const
    {
	if (strcmp(name, madstr_gain) == 0)
	{
	    *value = m_iGain;
	    return 0;
	}
	return -1;
    }
    int SetValue(const char* name, int value)
    {
	if (strcmp(name, madstr_gain) == 0)
	{
	    m_iGain = value;
	    return 0;
	}
	return -1;
    }

#if 0
    int GetOutputFormat(WAVEFORMATEX* destfmt)
    {
	if (!destfmt)
	    return -1;
	*destfmt = in_fmt;

	destfmt->wBitsPerSample = 16;
	destfmt->wFormatTag = 0x2000;
	destfmt->nAvgBytesPerSec = 192000;  // after conversion
	destfmt->nBlockAlign = MAD_BLOCK_SIZE;
	destfmt->nSamplesPerSec = destfmt->nAvgBytesPerSec / destfmt->nChannels
	    / (destfmt->wBitsPerSample / 8);

	/*
	 destfmt->nBlockAlign = destfmt->nChannels * destfmt->wBitsPerSample / 8;
	 destfmt->nAvgBytesPerSec = destfmt->nSamplesPerSec * destfmt->nBlockAlign;
	 destfmt->cbSize = 0;
	 */
	char b[200];
	avm_wave_format(b, sizeof(b), &in_fmt);
	printf("src %s\n", b);
	avm_wave_format(b, sizeof(b), destfmt);
	printf("dst %s\n", b);

	return 0;
    }
#endif
};

static IAudioDecoder* mad_CreateAudioDecoder(const CodecInfo& info, const WAVEFORMATEX* format)
{
    return new MAD_Decoder(info, format);
}

AVM_END_NAMESPACE;

extern "C" avm::codec_plugin_t avm_codec_plugin_mad_audiodec;

avm::codec_plugin_t avm_codec_plugin_mad_audiodec =
{
    PLUGIN_API_VERSION,

    0, // err
    0, 0,
    avm::PluginGetAttrInt,
    avm::PluginSetAttrInt,
    0, 0, // attrs
    avm::mad_FillPlugins,
    avm::mad_CreateAudioDecoder,
    0,
    0,
    0,
};
