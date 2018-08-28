#include "fillplugins.h"
#include "audiodecoder.h"
#include "plugin.h"
#include "utils.h"

#include <stdio.h>

#define A52BIN
#ifdef A52BIN
#include <dlfcn.h>

/**
 * copy of A52 defines here
 *
 * have to match with the a52 library
 * hopefully they will not change often
 */
#define A52_CHANNEL 0
#define A52_MONO 1
#define A52_STEREO 2
#define A52_3F 3
#define A52_2F1R 4
#define A52_3F1R 5
#define A52_2F2R 6
#define A52_3F2R 7
#define A52_CHANNEL1 8
#define A52_CHANNEL2 9
#define A52_DOLBY 10
#define A52_CHANNEL_MASK 15

#define A52_LFE 16
#define A52_ADJUST_LEVEL 32
/* generic accelerations */
#define MM_ACCEL_DJBFFT		0x00000001

/* x86 accelerations */
#define MM_ACCEL_X86_MMX	0x80000000
#define MM_ACCEL_X86_3DNOW	0x40000000
#define MM_ACCEL_X86_MMXEXT	0x20000000

typedef struct a52_state_s a52_state_t;
typedef float sample_t;

/** end of a52.h and mm_accel.h **/
#endif

AVM_BEGIN_NAMESPACE;

// library name
static const char* a52name = "liba52.so.0";

class A52_Decoder : public IAudioDecoder
{
public:

    A52_Decoder(const CodecInfo& info, const WAVEFORMATEX* wf)
	:IAudioDecoder(info, wf)
    {
        m_Error[0] = 0;
    }

    int init()
    {
#ifdef A52BIN
	m_pHandle = dlopen(a52name, RTLD_LAZY);
	if (!m_pHandle)
	{
	    sprintf(m_Error, "library '%s'  could not be opened: %s\n",
		    a52name, dlerror());
            return -1;
	}

	// resolve all needed function calls
	p_a52_init = (a52_state_t* (*)(uint32_t)) dlsymm("a52_init");
	p_a52_samples = (sample_t* (*)(a52_state_t*)) dlsymm("a52_samples");
	p_a52_syncinfo = (int (*)(uint8_t*, int*, int*, int*)) dlsymm("a52_syncinfo");
	p_a52_frame = (int (*)(a52_state_t*, uint8_t*, int*, sample_t*, sample_t)) dlsymm("a52_frame");
	p_a52_block = (int (*)(a52_state_t*)) dlsymm("a52_block");
	p_a52_free = (void (*)(a52_state_t*)) dlsymm("a52_free");
#else
	p_a52_init = a52_init;
	p_a52_samples = a52_samples;
	p_a52_syncinfo = a52_syncinfo;
	p_a52_frame = a52_frame;
	p_a52_block = a52_block;
	p_a52_free = a52_free;
#endif

	m_pState = p_a52_init(0);
	if (!m_pState)
	{
	    sprintf(m_Error, "initialization failed");
            return -1;
	}

	m_pSamples = p_a52_samples(m_pState);
        return 0;
    }
    virtual ~A52_Decoder()
    {
        if (m_pState)
	    p_a52_free(m_pState);
#ifdef A52BIN
        if (m_pHandle)
	    dlclose(m_pHandle);
#endif
    }
    virtual int Convert(const void* in_data, uint_t in_size,
			void* out_data, uint_t out_size,
			uint_t* size_read, uint_t* size_written)
    {
	unsigned written = 0;
	unsigned bread = 0;

	//while ((written + (6 * 2 * 256 * 2)) < out_size && (bread + 2000) < in_size)
	for (;;)
	{
	    int len = p_a52_syncinfo((unsigned char*)in_data + bread, &flags, &sample_rate, &bit_rate);
	    //printf("syncinfo  %d %d %d   insz: %d\n", sample_rate, bit_rate, flags,   in_size);
	    if (len <= 0) {
		//printf("AC3 stream not valid.\n");
		if (bread + 128 < in_size)
		{
		    bread++;
		    continue;
		}
		break;
	    }

	    if (flags != A52_MONO)
		flags = A52_STEREO;

	    sample_t level = 1;
	    if (p_a52_frame(m_pState, (unsigned char*)in_data + bread, &flags, &level, 384))
		break;
	    bread += len;

	    for (int i = 0; i < 6; i++)
	    {
		if (p_a52_block(m_pState))
		    break;
		float_to_int(m_pSamples, (int16_t*)out_data + i * 256 * 2/*channels*/, 2);
		written += 2 * 2 * 256;
	    }
	    break;
	}

	//printf("READ %d   WR %d\n", bread, written);
	// we are reading just one frame - easier to synchronize
	// speed is not ours biggest problem :) ...
	// so I don't care we call write more offten then play-ac3
	if (size_read)
	    *size_read = bread;
	if (size_written)
	    *size_written = written;

	return 0;
    }
#if 0
    virtual int GetOutputFormat(WAVEFORMATEX* destfmt) const
    {
	if (!destfmt)
	    return -1;
	*destfmt = *m_pFormat;

	destfmt->wBitsPerSample = 16;
	destfmt->wFormatTag = 0x2000;
	destfmt->nAvgBytesPerSec = 192000;  // after conversion
	destfmt->nChannels = 0
	    destfmt->nBlockAlign = A52_BLOCK_SIZE;
	destfmt->nSamplesPerSec = destfmt->nAvgBytesPerSec / destfmt->nChannels
	    / (destfmt->wBitsPerSample / 8);
	destfmt->cbSize = 0;
	/*
	 destfmt->nBlockAlign = destfmt->nChannels * destfmt->wBitsPerSample / 8;
	 destfmt->nAvgBytesPerSec = destfmt->nSamplesPerSec * destfmt->nBlockAlign;
	 */
	return 0;
    }
#endif
    const char* getError() const { return m_Error; }
protected:
    /**** the following two functions comes from a52dec */
    int blah(int32_t i)
    {
	if (i > 0x43c07fff)
	    return 32767;
	else if (i < 0x43bf8000)
	    return -32768;
	return i - 0x43c00000;
    }

    void float_to_int(float* _f, int16_t* s16, int nchannels)
    {
	int i, j, c;
	int32_t * f = (int32_t *) _f;	// XXX assumes IEEE float format

	j = 0;
	nchannels *= 256;
	for (i = 0; i < 256; i++) {
	    for (c = 0; c < nchannels; c += 256)
		s16[j++] = blah (f[i + c]);
	}
    }
#ifdef A52BIN
    void* m_pHandle;
    void* dlsymm(const char* symbol, bool fatal = true)
    {
	if (m_Error[0] != 0)
            return 0;
	void* f = dlsym(m_pHandle, symbol);
	if (!f && fatal)
	    sprintf(m_Error, "function '%s' can't be resolved", symbol);
	return f;
    }
#endif

    a52_state_t* m_pState;
    sample_t* m_pSamples;
    int flags, sample_rate, bit_rate;

    a52_state_t* (*p_a52_init)(uint32_t mm_accel);
    sample_t* (*p_a52_samples)(a52_state_t * state);
    int (*p_a52_syncinfo)(uint8_t * buf, int * flags,
			  int * sample_rate, int * bit_rate);
    int (*p_a52_frame)(a52_state_t * state, uint8_t * buf, int * flags,
		       sample_t * level, sample_t bias);
    void (*p_a52_dynrng)(a52_state_t * state,
			 sample_t (* call) (sample_t, void *), void * data);
    int (*p_a52_block)(a52_state_t * state);
    void (*p_a52_free)(a52_state_t * state);

    char m_Error[128];
};

IAudioDecoder* CreateA52_Decoder(const CodecInfo& info, const WAVEFORMATEX* format)
{
    A52_Decoder* d = new A52_Decoder(info, format);
    if (d->init() == 0)
	return d;

    delete d;
    return 0;
}

AVM_END_NAMESPACE;
