//
// Class which should minimalize effect of
// changed sound signal by interpolating through the zero
//

#include "AudioCleaner.h"
#include <stdio.h>
AVM_BEGIN_NAMESPACE;

template <class T>
uint_t AudioCleanerStereo<T>::soundOff(void* out, const void* in)
{
#if 0
    T* OUT = (T*) out;
    const T* IN = (const T*) in;

    unsigned i = 1;
    while (i < CLEARED_SAMPLES)
    {
	float f = (CLEARED_SAMPLES - i) / (float)CLEARED_SAMPLES;
	*OUT++ = (T) (f * *IN++);
	*OUT++ = (T) (f * *IN++);
        i++;
    }

    while (i < 2 * CLEARED_SAMPLES)
    {
	*OUT++ = (T) 0;
	*OUT++ = (T) 0;
	i++;
    }
#endif
    m_uiRemains = m_uiClearSize;
    return 2 * CLEARED_SAMPLES;
}

template <class T>
uint_t AudioCleanerStereo<T>::soundOn(void* out, uint_t n)
{
    if (m_uiRemains > 0)
    {
	T* OUT = (T*) out;
	uint_t clear = (n < m_uiRemains && (n > 0)) ? n : m_uiRemains;
	m_uiRemains -= clear;
	if (m_uiRemains < 128)
	    m_uiRemains = 0;

	uint_t sub = (m_uiRemains) ? 0 : CLEARED_SAMPLES * 2 * sizeof(T);
	if (clear < sub)
	    sub = clear;

	memset(OUT, (sizeof(T) == 1) ? 0x80 : 0, clear - sub + 4);

	OUT += clear / sizeof(T);
	T* IN = OUT;

	if (sub > 0)
	{
	    for (unsigned i = 1; i <= sub / 2 / sizeof(T); i++)
	    {
		float f = (CLEARED_SAMPLES - i) / (float)CLEARED_SAMPLES;
		f *= f;
		OUT--;
		*OUT = (T) (f * *OUT);
		OUT--;
		*OUT = (T) (f * *OUT);
	    }
	}
    }

    return m_uiRemains;
}

template <class T>
uint_t AudioCleanerMono<T>::soundOff(void* out, const void* in)
{
#if 0
    T* OUT = (T*) out;
    const T* IN = (const T*) in;

    for (unsigned i = 0; i < CLEARED_SAMPLES; i++)
    {
	float f = (CLEARED_SAMPLES - i) / (double)CLEARED_SAMPLES;
	*OUT++ = (T) (IN[0] * f);
    }
#endif
    m_uiRemains = m_uiClearSize;
    return 0;
}

template <class T>
uint_t AudioCleanerMono<T>::soundOn(void* out, uint_t n)
{
    if (m_uiRemains > 0)
    {
	T* OUT = (T*) out;
	uint_t clear = (n < m_uiRemains && (n > 0)) ? n : m_uiRemains;
	m_uiRemains -= clear;
	if (m_uiRemains < 128)
	    m_uiRemains = 0;

	uint_t sub = (m_uiRemains) ? 0 : CLEARED_SAMPLES * sizeof(T);
	if (clear < sub)
	    sub = clear;

	memset(OUT, (sizeof(T) == 1) ? 0x80 : 0, clear - sub + 4);

	OUT += clear / sizeof(T);
	T* IN = OUT;

	if (sub > 0)
	{
	    for (unsigned i = 1; i <= sub / sizeof(T); i++)
	    {
		float f = (CLEARED_SAMPLES - i) / (float)CLEARED_SAMPLES;
		f *= f;
		OUT--;
		*OUT = (T) (f * *OUT);
	    }
	}
    }
    return m_uiRemains;
}

IAudioCleaner* CreateAudioCleaner(uint_t channels, uint_t bitsPerSample,
				  uint_t clearsz)
{
    IAudioCleaner* r = 0;

    switch (channels)
    {
    case 1:
	if (bitsPerSample <= 8)
	    r = new AudioCleanerMono<unsigned char>(clearsz);
        else if (bitsPerSample <= 16)
	    r = new AudioCleanerMono<short>(clearsz);
        else if (bitsPerSample <= 32)
	    r = new AudioCleanerMono<int>(clearsz);
        break;
    case 2:
	if (bitsPerSample <= 8)
	    r = new AudioCleanerStereo<unsigned char>(clearsz);
        else if (bitsPerSample <= 16)
	    r = new AudioCleanerStereo<short>(clearsz);
        else if (bitsPerSample <= 32)
	    r = new AudioCleanerStereo<int>(clearsz);
        break;
    }

    return r;
}

AVM_END_NAMESPACE;
