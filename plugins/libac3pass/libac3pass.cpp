#include "fillplugins.h"
#include "audiodecoder.h"
#include "utils.h"
#include "plugin.h"
#include "ac3-iec958.h"
#include "avm_output.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

AVM_BEGIN_NAMESPACE;

PLUGIN_TEMP(ac3pass);

class AC3_PassDecoder : public IAudioDecoder
{
public:
    // Note:  AudioQueue has this value hardcoded and it is used
    //        as block size when ac3 passthrough stream is played!
    static const uint_t AC3_BLOCK_SIZE = 6144;

    AC3_PassDecoder(const CodecInfo& info, const WAVEFORMATEX* wf)
	:IAudioDecoder(info, wf)
    {
    }
    int Convert(const void* in_data, uint_t in_size,
		void* out_data, uint_t out_size,
		uint_t* size_read, uint_t* size_written)
    {
	if (!in_data || !out_data)
	    return -1;

	int skipped;
	struct ac3info ai;

	int data_type = 1;

	int i = ac3_iec958_parse_syncinfo((unsigned char*)in_data, in_size, &ai, &skipped);
	if (i < 0) {
	    AVM_WRITE("AC3_PassThrough", "AC3 stream not valid.\n");
	    return -1;
	}
	if (ai.samplerate != 48000) {
	    AVM_WRITE("AC3_PassThrough", "Only 48000 Hz streams supported.\n");
	    return -1;
	}

	// we are reading just one frame - easier to synchronize
	// speed is not ours biggest problem :) ...
	// so I don't care we call write more offten then play-ac3
	ac3_iec958_build_burst(ai.framesize, data_type, 1,
			       (unsigned char*)in_data + skipped,
			       (unsigned char*)out_data);

	if (size_read)
	    *size_read = skipped + ai.framesize;
	if (size_written)
	    *size_written = AC3_BLOCK_SIZE;

	return 0;
    }
    int GetOutputFormat(WAVEFORMATEX* destfmt) const
    {
	if (!destfmt)
	    return -1;
	*destfmt = *m_pFormat;

	destfmt->wBitsPerSample = 16;
	destfmt->wFormatTag = 0x2000;
	destfmt->nAvgBytesPerSec = 192000;  // after conversion
	destfmt->nBlockAlign = AC3_BLOCK_SIZE;
	destfmt->nSamplesPerSec = destfmt->nAvgBytesPerSec / destfmt->nChannels
	    / (destfmt->wBitsPerSample / 8);
	destfmt->cbSize = 0;
	/*
	 destfmt->nBlockAlign = destfmt->nChannels * destfmt->wBitsPerSample / 8;
	 destfmt->nAvgBytesPerSec = destfmt->nSamplesPerSec * destfmt->nBlockAlign;
	 */
	return 0;
    }
};

// PLUGIN loading part
static IAudioDecoder* ac3pass_CreateAudioDecoder(const CodecInfo& info, const WAVEFORMATEX* format)
{
    if (info.fourcc == 0x2000) //AC3
	return new AC3_PassDecoder(info, format);
    ac3pass_error_set("format unsupported");
    return 0;
}

AVM_END_NAMESPACE;

extern "C" avm::codec_plugin_t avm_codec_plugin_ac3pass;

avm::codec_plugin_t avm_codec_plugin_ac3pass =
{
    PLUGIN_API_VERSION,

    0, // err
    0,0, 0, 0, 0, 0, // attrs
    avm::ac3pass_FillPlugins,
    avm::ac3pass_CreateAudioDecoder,
    0,
    0,
    0,
};
