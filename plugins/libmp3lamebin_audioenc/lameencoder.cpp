/*
 *  Plugin for lame encoding
 *
 *  this version loads the library in runtime
 *  might look a bit complicated but has few advantages - you do not have
 *  to have libmp3lame installed on your system while you still could build
 *  this working plugin
 */

#include "audioencoder.h"
#include "fillplugins.h"
#include "plugin.h"
#include "utils.h"
#include "avm_output.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lame.h"

AVM_BEGIN_NAMESPACE;

PLUGIN_TEMP(mp3lamebin_audioenc)

static const char* mp3lamename = "libmp3lame.so.0";

class LameEncoder: public IAudioEncoder
{
    void* handle;
    lame_global_flags* gf;
    WAVEFORMATEX in_fmt;
    int brate;
    char m_Error[128];

    /* lame calls */
    lame_global_flags* CDECL (*p_lame_init)(void);
    int CDECL (*p_lame_init_params)(lame_global_flags *);
    int CDECL (*p_lame_encode_buffer)(lame_global_flags*  gfp,
				      const short int     buffer_l[],
				      const short int     buffer_r[],
				      const int           nsamples,
				      unsigned char*      mp3buf,
				      const int           mp3buf_size);
    int CDECL (*p_lame_encode_buffer_interleaved)(lame_global_flags*  gfp,
						  short int           pcm[],
						  int                 num_samples,
						  unsigned char*      mp3buf,
						  int                 mp3buf_size);
    int CDECL (*p_lame_encode_finish)(lame_global_flags*  gfp,
				      unsigned char*      mp3buf,
				      int                 size);
    int CDECL (*p_lame_get_framesize)(const lame_global_flags *);
    int CDECL (*p_lame_get_size_mp3buffer)( const lame_global_flags*);
    int CDECL (*p_lame_get_brate)(const lame_global_flags *);
    int CDECL (*p_lame_get_VBR_mean_bitrate_kbps)( const lame_global_flags*);
    void CDECL (*p_lame_print_config)(const lame_global_flags*);
    void CDECL (*p_lame_print_internals)(const lame_global_flags *);
    int CDECL (*p_lame_set_bWriteVbrTag)(lame_global_flags *, int);
    int CDECL (*p_lame_set_padding_type)(lame_global_flags *, Padding_type);
    int CDECL (*p_lame_set_VBR)(lame_global_flags *, vbr_mode);
    int CDECL (*p_lame_set_VBR_q)(lame_global_flags *, int);
    int CDECL (*p_lame_set_VBR_mean_bitrate_kbps)(lame_global_flags *, int);
    int CDECL (*p_lame_set_VBR_min_bitrate_kbps)(lame_global_flags *, int);
    int CDECL (*p_lame_set_VBR_max_bitrate_kbps)(lame_global_flags *, int);
    int CDECL (*p_lame_set_in_samplerate)(lame_global_flags *, int);
    int CDECL (*p_lame_set_num_channels)(lame_global_flags *, int);
    int CDECL (*p_lame_set_mode)(lame_global_flags *, MPEG_mode);
    int CDECL (*p_lame_set_brate)(lame_global_flags *, int);
    int CDECL (*p_lame_set_quality)(lame_global_flags *, int);

public:
    LameEncoder(const CodecInfo& info, const WAVEFORMATEX* format)
	:IAudioEncoder(info), handle(0)
    {
        m_Error[0] = 0;
	in_fmt = *format;
    }

    int init()
    {
	handle = dlopen(mp3lamename, RTLD_LAZY);

	if (!handle)
	{
	    sprintf(m_Error, "Lame library %s  could not be opened: %s\n"
		    "If you want to use this plugin - install lame library\n"
		    "on your system -  see README for more details\n",
		    mp3lamename, dlerror());
            return -1;
	}

	// resolve all needed function calls
#define d(a, b)		p_lame_ ## a = b dlsymm( "lame_" #a )
	d(init, (lame_global_flags * CDECL (*)(void)));
	d(init_params, (int CDECL (*)(lame_global_flags *)));
	d(print_config, (void CDECL (*)(const lame_global_flags *)));
	d(print_internals, (void CDECL (*)(const lame_global_flags *)));
	d(set_bWriteVbrTag, (int CDECL (*)(lame_global_flags *, int)));
	d(set_padding_type, (int CDECL (*)(lame_global_flags *, Padding_type)));
	d(set_VBR, (int CDECL (*)(lame_global_flags *, vbr_mode)));
	d(set_VBR_q, (int CDECL (*)(lame_global_flags *, int)));
	d(set_VBR_mean_bitrate_kbps, (int CDECL (*)(lame_global_flags *, int)));
	d(set_VBR_min_bitrate_kbps, (int CDECL (*)(lame_global_flags *, int)));
	d(set_VBR_max_bitrate_kbps, (int CDECL (*)(lame_global_flags *, int)));
	d(set_in_samplerate, (int CDECL (*)(lame_global_flags *, int)));
	d(set_num_channels, (int CDECL (*)(lame_global_flags *, int)));
	d(set_mode, (int CDECL (*)(lame_global_flags *, MPEG_mode)));
	d(set_brate, (int CDECL (*)(lame_global_flags *, int)));
	d(set_quality, (int CDECL (*)(lame_global_flags *, int)));
	d(get_framesize, (int CDECL (*)(const lame_global_flags *)));
	d(get_size_mp3buffer, (int CDECL (*)(const lame_global_flags *)));
	d(get_brate, (int CDECL (*)(const lame_global_flags *)));
	d(get_VBR_mean_bitrate_kbps, (int CDECL (*)(const lame_global_flags *)));
	d(encode_buffer_interleaved, (int CDECL (*)(lame_global_flags*,
						    short int pcm[],
						    int, unsigned char*, int)));
	d(encode_finish, (int CDECL (*)(lame_global_flags*, unsigned char*, int)));
	d(encode_buffer, (int CDECL (*)(lame_global_flags*, const short int buffer_l[],
					const short int buffer_r[],
					const int, unsigned char*, const int)));
#undef d
	if (m_Error[0] != 0)
            return -1;

	gf = p_lame_init();
	p_lame_set_bWriteVbrTag(gf, 0);
	p_lame_set_padding_type(gf, PAD_ADJUST);
	p_lame_set_in_samplerate(gf, in_fmt.nSamplesPerSec);
	p_lame_set_num_channels(gf, in_fmt.nChannels);

#if 1
	p_lame_set_VBR(gf, vbr_off);
#else
	p_lame_set_VBR(gf, vbr_mtrh);
	p_lame_set_VBR_q(gf, 7);
	p_lame_set_bWriteVbrTag(gf, 1);
	p_lame_set_VBR_max_bitrate_kbps(gf, 64);
	p_lame_set_quality(gf, 9);
#endif

	if (in_fmt.nChannels == 1)
	    p_lame_set_mode(gf, MONO);
	else
	    p_lame_set_mode(gf, JOINT_STEREO);
	/* 0,1,2,3 stereo,jstereo,dual channel,mono */

	p_lame_init_params(gf);

	//p_lame_print_config(gf);
	//p_lame_print_internals(gf);

	AVM_WRITE("Lame MP3 encoder", "LameEncoder initialized\n");
        return 0;
    }

    ~LameEncoder()
    {
        if (handle)
	    dlclose(handle);
    }

    virtual int Convert(const void* in_data, uint_t in_size,
			void* out_data, uint_t out_size,
			uint_t* size_read, uint_t* size_written)
    {
#warning FIXME 8-bit?
	int result;

	//printf("LAMEGETBUG %d\n", p_lame_get_size_mp3buffer(gf));
	if (in_fmt.nChannels == 1)
	    result = p_lame_encode_buffer(gf, (short*)in_data,
					  (short*)in_data, in_size,
					  (unsigned char*)out_data, out_size);
	else
	    result = p_lame_encode_buffer_interleaved(gf, (short*)in_data, in_size,
						      (unsigned char*)out_data, out_size);
	//printf("LAMEGETBUG2 b:%d  i:%d  s:%d\n", p_lame_get_size_mp3buffer(gf), in_size, result);
	if (result < 0)
	    result = 0;
	if (size_read)
	    *size_read = in_size;
	if (size_written)
	    *size_written = result;
	return 0;
    }
    virtual uint_t GetFormat(void* extension = 0, uint_t size = 0) const
    {
	struct __attribute__((__packed__)) strf_mp3
	{
	    uint16_t wID;
	    uint32_t fdwFlags;
	    uint16_t nBlockSize;
	    uint16_t nFramesPerBlock;
	    uint16_t nCodecDelay;
	};

	if (!extension)
	    return 30;
	if (size < 30)
	    return 0;

	int ibrate = p_lame_get_brate(gf);
	memset(extension, 0, size);
	WAVEFORMATEX wf;
	strf_mp3 mp3extra;
	memcpy(&wf, &in_fmt, sizeof(wf));
	wf.wFormatTag = 0x55;
	wf.nAvgBytesPerSec = ibrate * 125;
	wf.nBlockAlign = 1;
	wf.wBitsPerSample = 0;
	wf.cbSize = 12;
	memcpy(extension, &wf, sizeof(wf));

	//p_lame_init_params(gf);
	avm_set_le16(&mp3extra.wID, 1);			//
	avm_set_le32(&mp3extra.fdwFlags, 2);		// These values based
	avm_set_le16(&mp3extra.nBlockSize, p_lame_get_framesize(gf));
	avm_set_le16(&mp3extra.nFramesPerBlock, 1);	// on an old Usenet post!!
	avm_set_le16(&mp3extra.nCodecDelay, 1393);	// 0x571 for F-IIS MP3 Codec

	memcpy((uint8_t*)extension + sizeof(wf), &mp3extra, 12);

	AVM_WRITE("Lame MP3 encoder", "LameEncoder::GetFormat  %d\n", ibrate);
	return 30;
    }
    virtual int Start()
    {
	//p_lame_init_params(gf);
	AVM_WRITE("Lame MP3 encoder", "LameEncoder::Start()  ch: %d  freq: %d\n",
		  in_fmt.nChannels, in_fmt.nSamplesPerSec);
	p_lame_init_params(gf);
	return 0;
    }
    virtual int SetBitrate(int bitrate)
    {
	brate = bitrate / 125;
	AVM_WRITE("Lame MP3 encoder", "LameEncoder::SetBitrate(%d) %dkbps\n", bitrate, brate);
	return p_lame_set_brate(gf, brate);
    }
    virtual int SetQuality(int quality)
    {
	AVM_WRITE("Lame MP3 encoder", "LameEncoder::SetQuality(%d)\n", quality);
	return p_lame_set_quality(gf, quality);
    }
    //virtual int SetVBR(int min, int max);
    const char* getError() { return m_Error; }
private:
    int Close(void* out_data, uint_t out_size, uint_t* size_read)
    {
	uint8_t buffer[7200];
	uint_t bytes = p_lame_encode_finish(gf, buffer, sizeof(buffer));
	if (out_size < bytes)
	    bytes = out_size;
	if (out_data)
	    memcpy(out_data, buffer, bytes);
	if (out_data && size_read)
	    *size_read = bytes;

	AVM_WRITE("Lame MP3 encoder", "average %d kbps", p_lame_get_VBR_mean_bitrate_kbps(gf));
	return 0;
    }
    void* dlsymm(const char* symbol, bool fatal = true)
    {
	if (m_Error[0] != 0)
	    return 0;

	void* f = dlsym(handle, symbol);
	if (!f && fatal)
	    sprintf(m_Error, "function '%s' can't be resolved\n", symbol);
	return f;
    }
};

/* here are some flags pertaining to the above structure */
#define MPEGLAYER3_ID_UNKNOWN            0
#define MPEGLAYER3_ID_MPEG               1
#define MPEGLAYER3_ID_CONSTANTFRAMESIZE  2
#define MPEGLAYER3_FLAG_PADDING_ISO      0x00000000
#define MPEGLAYER3_FLAG_PADDING_ON       0x00000001
#define MPEGLAYER3_FLAG_PADDING_OFF      0x00000002

// plugin part
static IAudioEncoder* mp3lamebin_CreateAudioEncoder(const CodecInfo& info, fourcc_t fourcc, const WAVEFORMATEX* fmt)
{
    LameEncoder* e = new LameEncoder(info, fmt);
    if (e->init() == 0)
        return e;

    mp3lamebin_audioenc_error_set(e->getError());
    delete e;
    return 0;
}

AVM_END_NAMESPACE;

extern "C" avm::codec_plugin_t avm_codec_plugin_mp3lamebin_audioenc;

avm::codec_plugin_t avm_codec_plugin_mp3lamebin_audioenc =
{
    PLUGIN_API_VERSION,

    0, // err
    0, 0, 0, 0, 0, 0, // attrs
    avm::mp3lamebin_FillPlugins,
    0,
    avm::mp3lamebin_CreateAudioEncoder,
    0,
    0,
};
