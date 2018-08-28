/***

    Ogg Vorbis audio decoder
    libvorbis interface is based on work by Felix 'Atmosfear' Buenemann
    ( atmosfear@users.sourceforge.net )
    and taken from 'mplayer' ( http://mplayer.sourceforge.net ).

    some other parts are taken directly from Ogg/Vorbis source

***/

#include "fillplugins.h"
#include "audiodecoder.h"
#include "utils.h"
#include "avm_output.h"
#include "plugin.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

AVM_BEGIN_NAMESPACE;

PLUGIN_TEMP(vorbis_audio)

class VorbisDecoder : public IAudioDecoder
{
    FILE* f;

    float m_fScaleFactor;

    ogg_sync_state   oy; /* sync and verify incoming physical bitstream */
    ogg_stream_state os; /* take physical pages, weld into a logical
   			    stream of packets */
    ogg_page         og; /* one Ogg bitstream page.  Vorbis packets are inside */
    ogg_packet       op; /* one raw packet of data for decode */

    vorbis_info      vi; /* struct that stores all the static vorbis bitstream
			    settings */
    vorbis_comment   vc; /* struct that stores all the bitstream user comments */
    vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
    vorbis_block     vb; /* local working space for packet->PCM decode */

    uint32_t hdrsizes[3]; /* learned from mplayer */
    int m_iSetNo;
    bool m_bInitialized;
    bool m_bInRead;


public:
    VorbisDecoder(const CodecInfo& info, const WAVEFORMATEX* wf)
	:IAudioDecoder(info, wf)
    {
    }
    ~VorbisDecoder()
    {
	if (m_bInitialized)
	{
	    ogg_stream_clear(&os);
	    ogg_sync_clear(&oy);

	    vorbis_block_clear(&vb);
	    vorbis_dsp_clear(&vd);
            vorbis_comment_clear(&vc);
	    vorbis_info_clear(&vi);
	}
    }

    int init()
    {
	m_bInitialized = false;
	m_fScaleFactor = 40000; // boost volume - do as an option

	//cout << "Init vorbis decoder " << wf->cbSize << "  " << sizeof(WAVEFORMATEXTENSIBLE) << endl;
	uint_t ogg_header_size;

	ogg_header_size = m_pFormat->cbSize;

	const char* vorbishdr = (const char*) m_pFormat + sizeof(WAVEFORMATEXTENSIBLE);
	//const char* ogg_header = vorbishdr;

	/*
	 *  inspired from mplayer's header parsing
	 */

	memcpy(hdrsizes, vorbishdr, 3 * sizeof(uint32_t));
	vorbishdr += 3 * sizeof(uint32_t);

	//cout << "hdr sizes: " << hdrsizes[0] << ", " << hdrsizes[1] << ",  " << hdrsizes[2] << endl;

	vorbis_info_init(&vi);
	vorbis_comment_init(&vc);

	op.packet = NULL;
	op.b_o_s  = 1; /* beginning of stream for first packet */
	op.bytes  = hdrsizes[0];
	op.packet = (unsigned char*) vorbishdr;
	vorbishdr += op.bytes;
	if (vorbis_synthesis_headerin(&vi, &vc, &op) < 0)
	{
	    vorbis_audio_error_set("initial (identification) header broken!");
            return -1;
	}

	op.b_o_s  = 0;
	op.bytes  = hdrsizes[1];
	op.packet = (unsigned char*) vorbishdr;
	vorbishdr += op.bytes;
	if (vorbis_synthesis_headerin(&vi, &vc, &op) < 0)
	{
	    vorbis_audio_error_set("comment header broken!");
            return -1;
	}

	op.bytes  = hdrsizes[2];
	op.packet = (unsigned char*) vorbishdr;
	vorbishdr += op.bytes;
	if (vorbis_synthesis_headerin(&vi, &vc, &op) < 0)
	{
	    vorbis_audio_error_set("codebook header broken!");
            return -1;
	}
	{
	    char **ptr = vc.user_comments;
	    while (*ptr)
	    {
		AVM_WRITE("Ogg Vorbis decoder", "OggVorbisComment: %s\n", *ptr);
		++ptr;
	    }
	    AVM_WRITE("Ogg Vorbis decoder", "Bitstream is %d channel, %ldHz, %ldkbit/s %cBR\n",
		      vi.channels, vi.rate, vi.bitrate_nominal/1000,
		      (vi.bitrate_lower != vi.bitrate_nominal)
		      ||(vi.bitrate_upper != vi.bitrate_nominal) ? 'V' : 'C');
	    AVM_WRITE("Ogg Vorbis decoder", "Encoded by: %s\n", vc.vendor);
	}
	m_uiBytesPerSec = vi.channels * vi.rate * 2; //16bit
	//printf("HDR SZE %d  diff %d\n", ogg_header_size, vorbishdr - ogg_header);

	/* OK, got and parsed all three headers. Initialize the Vorbis
	 packet->PCM decoder. */
	vorbis_synthesis_init(&vd, &vi); /* central decode state */
	vorbis_block_init(&vd, &vb);     /* local state for most of the decode
	so multiple block decodes can
	proceed in parallel.  We could init
	multiple vorbis_block structures
	for vd here */
	//printf("OggVorbis: synthesis and block init done.\n");
	ogg_sync_init(&oy);		     /* Now we can read pages */
	ogg_stream_reset(&os);

	m_bInRead = true;
        return 0;
    }

    int Convert(const void* in_data, uint_t in_size,
		void* out_data, uint_t out_size,
		uint_t* size_read, uint_t* size_written)
    {
	const char* in_data_base = (const char*) in_data;
	int r = 0;
	uint_t outwrt = 0;

	//fwrite(in_data, 1, in_size, f);
	//fflush(f);

	// Note - m_bInRead checks if we haven't been reading new data
	// previously in that case we will not call packetout
        // OGG stream parsing
	while (m_bInRead || (r = ogg_stream_packetout(&os, &op)) != 1)
	{
	    //printf("enter loop   %d   %d\n", r, m_bInRead);
	    if (r == 0)
	    {
		//while((r = ogg_sync_pageseek(&oy, &og)) <= 0)
		while ((r = ogg_sync_pageout(&oy, &og)) != 1 && in_size > 0)
		{
		    /* submit in_sized block to libvorbis' Ogg layer */
		    //printf("OggVorbis: Pageout: need more data to verify page, reading more data. %d\n", in_size);
		    char* buffer = ogg_sync_buffer(&oy, in_size);
		    memcpy(buffer, in_data, in_size);
		    ogg_sync_wrote(&oy, in_size);
		    in_data = (const char*)in_data + in_size;
                    in_size = 0;
		}

		if (r != 1)
		{
		    m_bInRead = true;
		    break;
		}

		m_bInRead = false;
		r = ogg_stream_pagein(&os, &og);

		if (r < 0 && m_bInitialized)
		{
		    AVM_WRITE("Ogg Vorbis decoder", "Pagein failed!\n");
		    break;
		}
		else if (!m_bInitialized)
		{
		    /* Get the serial number and set up the rest of decode. */
		    /* serialno first; use it to set up a logical stream */
		    m_iSetNo = ogg_page_serialno(&og);
		    ogg_stream_init(&os, m_iSetNo);

		    AVM_WRITE("Ogg Vorbis decoder", "Init OK! (%d)\n", m_iSetNo);
		    m_bInitialized = true;
		}
	    }
	}

        // Vorbis decoding
	if (r == 1)
	{
	    //printf("valid loop %d\n", in_size);
	    // we have valid packet
	    if (vorbis_synthesis(&vb, &op) == 0)
	    {
		/* test for success! */
		vorbis_synthesis_blockin(&vd, &vb);

		float** pcm;
		int convsize = (out_size - outwrt) / vi.channels / sizeof(int16_t);
		int clipflag = 0;
		int samples;

		while ((samples = vorbis_synthesis_pcmout(&vd, &pcm)) > 0)
		{
		    int bout = (samples < convsize) ? samples : convsize;

		    if (bout <= 0)
			break;

		    int16_t *ptr = 0;

		    for (int i = 0; i < vi.channels; i++)
		    {
			ptr = (int16_t *)out_data + i;
			float  *mono=pcm[i];
			for (int j = 0; j < bout; j++)
			{
			    int val=(int)(mono[j] * m_fScaleFactor);
			    /* might as well guard against clipping */
			    if (val > 32767)
			    {
				val = 32767;
				clipflag = 1;
			    }
			    else if (val < -32768)
			    {
				val = -32768;
				clipflag = 1;
			    }
			    *ptr = val;
			    ptr += vi.channels;
			}
		    }
		    /* tell libvorbis about consumed samples */
		    vorbis_synthesis_read(&vd, bout);

		    //printf("CONVERT %d  %d\n", outwrt, bout);
		    convsize -= bout;
		    outwrt += bout;
		    out_data = ptr;
		}
		if (clipflag)
		{
		    if (m_fScaleFactor > 32768.0)
		    {
			m_fScaleFactor *= 0.9;
			if (m_fScaleFactor < 32768.0)
			    m_fScaleFactor = 32768.0;
		    }
		    AVM_WRITE("Ogg Vorbis decoder", "OggVorbis: clipping -> %f\n", m_fScaleFactor);
		}
	    }
	}

	if (size_read)
	    *size_read = (const char*) in_data - in_data_base;
	if (size_written)
	    *size_written = outwrt * vi.channels * sizeof(int16_t);
	//printf("DATA Read: %d  Written: %d\n", *size_read, *size_written);
	return 0;
    }

    virtual void Flush()
    {
	// FIXME: for proper usage this would have to be fixed somehow
	//ogg_stream_flush(&os, &og);
    }

    virtual int GetOutputFormat(WAVEFORMATEX* destfmt) const
    {
	if (!destfmt)
	    return -1;
	*destfmt = *m_pFormat;

	destfmt->wFormatTag = 1;//PCM
	destfmt->wBitsPerSample = 16;
	destfmt->nAvgBytesPerSec = vi.rate * vi.channels * 2;  // after conversion
	destfmt->nBlockAlign = destfmt->nChannels * destfmt->wBitsPerSample / 8;
	destfmt->nSamplesPerSec = destfmt->nAvgBytesPerSec / destfmt->nChannels
	    / (destfmt->wBitsPerSample / 8);

	destfmt->cbSize = 0;

	return 0;
    }

    virtual uint_t GetMinSize() const
    {
        return 128000;
    }

    virtual uint_t GetSrcSize(uint_t dest_size) const
    {
	//printf("SR %d  %ld  b:%d   %ld\n", dest_size, vi.bitrate_nominal, m_uiBytesPerSec, (dest_size * vi.bitrate_nominal / 8) / m_uiBytesPerSec);
        return  (dest_size * vi.bitrate_nominal / 8) / m_uiBytesPerSec;
    }
};


// PLUGIN loading part
static IAudioDecoder* vorbis_CreateAudioDecoder(const CodecInfo& info, const WAVEFORMATEX* format)
{
    if (info.fourcc == WAVE_FORMAT_EXTENSIBLE)
    {
	VorbisDecoder* d = new VorbisDecoder(info, format);
	if (d->init() == 0)
	    return d;
        delete d;
    }
    else
	vorbis_audio_error_set("format unsupported");
    return 0;
}

AVM_END_NAMESPACE;

extern "C" avm::codec_plugin_t avm_codec_plugin_vorbis_audio;

avm::codec_plugin_t avm_codec_plugin_vorbis_audio =
{
    PLUGIN_API_VERSION,

    0, // err
    0, 0, 0, 0, 0, 0, // attrs
    avm::vorbis_FillPlugins,
    avm::vorbis_CreateAudioDecoder,
    0,
    0,
    0,
};

// some useless junk
#if 0
{
    {
	int i;
	int sum = 0;
	for (i = 0; i < rs; i++)
	    sum += buffer[i];
	printf("checksum %d\n", sum);
    }

    avm_usleep(100000);
}
#endif

#if 0
    char* bb = ogg_header;
    for (int i = 0; i < ogg_header_size; i++)
    {
	if (!(i % 8))
	    printf("\n-- %04x  ", i);
	printf("0x%02x %c  ", (unsigned char)bb[i], isprint(bb[i]) ? bb[i] : '.');
    }
#endif

#if 0
    f = fopen("/tmp/o", "w+");
    cout << "opened" << endl;
    fwrite(ogg_header, 1, ogg_header_size, f);
    fflush(f);
    fclose(f);
#endif
#if 0
    // example of vorbis headers
static const char fakehd[] =
{
    'O','g','g','S',  0,2,
    0,0,0,0, 0,0,0,0,
    0xf0,0x0d,0,0,  //ogg_page_serialno  14..17b
    0,0,0,0,        //ogg_page_pageno    18..21b
    0x8d,0x4b,0xef,0x42,
    1, // subpackets ???
    0x1e,

    1,'v','o','r','b','i','s',0, 0,0,0,2,
    0x80,0xbb,0,0,  0xff,0xff,0xff,0xff,
    0,0x71,2,0,   0xff,0xff,0xff,0xff,  0xb8,1, //1e

    'O','g','g','S',  0,0,
    0,0,0,0, 0,0,0,0,
    0xf0,0x0d,0,0,
    1,0,0,0,    // second packet
    0xa4,0x7d,0xe5,0x92,
    0x11,  // 17
    0x30,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    3
};

static const char fakehd1[] =
{
    'O','g','g','S',  0,1,
    0,0,0,0, 0,0,0,0,
    0xf0,0xd,0,0,
    2,0,0,0,
    0,0xe0,0x33,0x63,
    2,0xff,0x2b
};

#endif
