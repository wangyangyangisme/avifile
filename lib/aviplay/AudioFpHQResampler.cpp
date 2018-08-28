//
//  RealTime High Precision resampling with antialising of the signal
//
//  based on some algorithm from
//  Digital Image Warping by George Wolberg - page 159
//
//

// this code might be a bit more optimized in the future
// but its already quite fast for its purpose

#include "AudioFpHQResampler.h"
#include <string.h>

/**
 * Real-arithmetic based resampling - giving very precise results - but its rather slow
 * few lines bellow we have now integer version of this -
 * this version is used for scaling of more then 16bit precise samples
 * and is not that much optimized as it serves as an illustration
 * how is this alghorithm working as the optimized integer version
 * is probably quite mystique
 *
 * NOTE:
 *  these methods are accesing one member behind array
 *  this is not a bug and it it intencial to fill also the last
 *  member in the output field
 **/

AVM_BEGIN_NAMESPACE;

#define PRECTYPE double
//#define PRECTYPE float

#define PREC (15)		// use 14bits for floating point part
#define PRECSIZE  (1L<<PREC)
#define PRECINT  (-1L<<PREC)

// implementation of the AudioResampler Interface
// floating point high quality version
template <class T> class AudioFpHQResamplerStereo : public IAudioResampler
{
public:
    virtual uint_t getBitsPerSample() const;
    virtual void resample(void* out, const void* in, uint_t out_count, uint_t int_count);
};

template <class T> class AudioFpHQResamplerMono : public IAudioResampler
{
public:
    virtual uint_t getBitsPerSample() const;
    virtual void resample(void* out, const void* in, uint_t out_count, uint_t in_count);
};

// to be used for 8 & 16 bit samples

template <class T> class AudioIntHQResamplerStereo : public IAudioResampler
{
public:
    virtual uint_t getBitsPerSample() const;
    virtual void resample(void* out, const void* in, uint_t out_count, uint_t int_count);
};

template <class T> class AudioIntHQResamplerMono : public  IAudioResampler
{
public:
    virtual uint_t getBitsPerSample() const;
    virtual void resample(void* out, const void* in, uint_t out_count, uint_t in_count);
};

template <class T>
void AudioFpHQResamplerMono<T>::resample(void* out, const void* in, uint_t OUTlen, uint_t INlen)
{
    const T* IN = (const T*) in;
    T* OUT = (T*) out;

    memset(OUT, 0, OUTlen * sizeof(T));

    PRECTYPE step = OUTlen / (PRECTYPE) (INlen - 1);
    PRECTYPE x1 = 0.0;
    for (unsigned u = 0; u <= INlen; u++)
    {
        PRECTYPE x0 = x1;
	x1 += step;
	/* integer parts */
	uint_t ix0 = (uint_t) x0;
	uint_t ix1 = (uint_t) x1;

	if (ix0 == ix1)
	{
	    OUT[ix0] += (T) (IN[u] * step);
	    continue;
	}

	OUT[ix0] += (T) (IN[u] * (ix0 + 1 - x0));

	PRECTYPE dl = (IN[u + 1] - IN[u]) / step;

	for (unsigned i = ix0 + 1; i < ix1; i++)
	    OUT[i] = (T) (IN[u] + dl * (i - x0));

	OUT[ix1] += (T) ((x1 - ix1) * (IN[u] + dl * (ix1 - x0)));
    }
    OUT[OUTlen - 1] = IN[INlen - 1];
}

template <class T>
void AudioFpHQResamplerStereo<T>::resample(void* out, const void* in, uint_t OUTlen, uint_t INlen)
{
    const T* IN = (const T*) in;
    T* OUT = (T*) out;

    memset(OUT, 0, OUTlen * sizeof(T) * 2);
    PRECTYPE step = (OUTlen) / (PRECTYPE) (INlen - 1);
    //cout << "SIZE " << sizeof(T) << "   " << OUTlen << "  " << INlen << "   step " << step << endl;

    PRECTYPE x1 = 0.0;
    for (uint_t u = 0; u <= 2*INlen; u += 2)
    {
        PRECTYPE x0 = x1;
	//x1 = u/2 * step;
	x1 += step;
	/* integer parts */
	uint_t ix0 = (uint_t) x0;
	uint_t ix1 = (uint_t) x1;

	if (ix0 == ix1)
	{
	    OUT[ix0 * 2] += (T) (IN[u] * step);
	    OUT[ix0 * 2 + 1] += (T) (IN[u + 1] * step);
	    continue;
	}

	OUT[ix0 * 2] += (T) (IN[u] * (ix0 + 1 - x0));
	OUT[ix0 * 2 + 1] += (T) (IN[u + 1] * (ix0 + 1 - x0));

	PRECTYPE dl = ((int)IN[u + 2] - (int)IN[u]) / step;
	PRECTYPE dr = ((int)IN[u + 2 + 1] - (int)IN[u + 1]) / step;

	for (unsigned i = ix0 + 1; i < ix1; i++)
	{
	    OUT[i * 2] = (T) (IN[u] + dl * (i - x0));
	    OUT[i * 2 + 1] = (T) (IN[u + 1] + dr * (i - x0));
	}

	OUT[ix1 * 2] += (T) ((x1 - ix1) * (IN[u] + dl * (ix1 - x0)));
        OUT[ix1 * 2 + 1] += (T) ((x1 - ix1) * (IN[u + 1] + dr * (ix1 - x0)));
    }

#if 0
    for (int i = 0; i < 16; i++)
	printf("   %d  in:  %x %x    out:  %x %x\n", i,
	       IN[2*i], IN[2*i + 1],
	       OUT[2*i], OUT[2*i + 1]);

    for (int i = 16; i >= 0; i--)
	printf("%d  in:  %x %x    out:  %x %x\n", i,
	       IN[2*INlen - 2*i], IN[2*INlen - 2*i + 1],
	       OUT[2*OUTlen - 2*i], OUT[2*OUTlen - 2*i + 1]);
#endif
}

template <class T>
uint_t AudioFpHQResamplerStereo<T>::getBitsPerSample() const
{
    return sizeof(T) * 8;
}

template <class T>
uint_t AudioFpHQResamplerMono<T>::getBitsPerSample() const
{
    return sizeof(T) * 8;
}

/**
 * Optimized real-arithmetic based resampling which is using integers
 *
 * NOTE:
 *  these methods are accesing one member behind array
 *  this is not a bug and it it intencial to fill also the last
 *  member in the output field
 **/

template <class T>
void AudioIntHQResamplerMono<T>::resample(void* out, const void* in, uint_t OUTlen, uint_t INlen)
{
    const T* IN = (const T*) in;
    T* OUT = (T*) out;
    const T* OUTEND = &OUT[OUTlen];
    int delta = (OUTlen * PRECSIZE) / (INlen - ((INlen < OUTlen) ? 1 : 0));
    //int delta = (OUTlen * PRECSIZE) / (INlen + 1);
    //printf("Delta: %d\n", delta);
    int frac = -PRECSIZE;
    int part = 0;
    int a = 0;
    for (;;)
    {
	frac += delta;

	if (frac >= 0)
	{
	    int dl;
	    int r = *IN;

	    frac -= PRECSIZE;

	    a += (PRECSIZE - part) * r;
	    *OUT = (T) (a >> PREC);
	    OUT++;
	    if (OUT >= OUTEND)
		break;

	    dl = ((IN[1] - r) << PREC) / delta;
	    a = r + dl - ((dl * part) >> PREC);
	    while (frac >= 0)
	    {
		*OUT = (T) a;
		OUT++;
		a += dl;
		frac -= PRECSIZE;
	    }

	    part = frac & (PRECSIZE - 1);
	    a *= part;
	}
	else
	{
	    part = frac & (PRECSIZE - 1);
	    a += *IN * delta;
	}

	IN++;
    }

    //printf("OUT: %p  OUTEND: %p   IN: %p   INEND: %p\n",
    //       OUT, OUTEND, IN, &((const T*) in)[INlen]);
    //OUT[-1] = IN[-1];
}

// this version is the same code as above it just goes through every second sample
// to save some registers we use this instead of parametrized version
// ok I think we could make slightly more complicated template...
template <class T>
void AudioIntHQResamplerStereo<T>::resample(void* out, const void* in, uint_t OUTlen, uint_t INlen)
{
    const T* OUTEND = &(((const T*)out)[OUTlen * 2]);
    int delta = (OUTlen * PRECSIZE) / (INlen - ((INlen < OUTlen) ? 1 : 0));

    // use two loops and skip every second sample - should be faster
    // than processing two channels at the same time
    for (int loop = 0; loop < 2; loop++)
    {
	const T* IN = (const T*) in;
	T* OUT = (T*) out;
	int x1 = 0, ix1 = 0;
	OUT += loop; // left channel - 0    right channel - 1
        IN += loop;


	int frac = -PRECSIZE;
	int part = 0;
	int a = 0;
	for (;;)
	{
	    frac += delta;

	    if (frac >= 0)
	    {
		int dl;
		int r = *IN;

		frac -= PRECSIZE;

		a += (PRECSIZE - part) * r;
		*OUT = (T) (a >> PREC);
		OUT += 2;

		if (OUT >= OUTEND)
		    break;

		dl = ((IN[2] - r) << PREC) / delta;
		a = r + dl - ((dl * part) >> PREC);
		while (frac >= 0)
		{
		    *OUT = (T) a;
		    OUT += 2;
		    a += dl;
		    frac -= PRECSIZE;
		}

		part = frac & (PRECSIZE - 1);
		a *= part;
	    }
	    else
	    {
		part = frac & (PRECSIZE - 1);
		a += *IN * delta;
	    }

	    IN += 2;
	}
	//OUT[-1] = IN[-1];
    }
}

template <class T>
uint_t AudioIntHQResamplerStereo<T>::getBitsPerSample() const
{
    return sizeof(T) * 8;
}

template <class T>
uint_t AudioIntHQResamplerMono<T>::getBitsPerSample() const
{
    return sizeof(T) * 8;
}

IAudioResampler* CreateHQResampler(uint_t channels, uint_t bitsPerSample)
{
    IAudioResampler* r = 0;

    //cout << "Resample " << channels << "   " << bitsPerSample << endl;

    // create all the needed instancies
    switch (channels)
    {
    case 1:
	if (bitsPerSample <= 8)
	    r = new AudioIntHQResamplerMono<unsigned char>;
        else if (bitsPerSample <= 16)
	    r = new AudioIntHQResamplerMono<short>;
        else if (bitsPerSample <= 32)
	    r = new AudioFpHQResamplerMono<int>;
        break;
    case 2:
	if (bitsPerSample <= 8)
	    r = new AudioIntHQResamplerStereo<unsigned char>;
        else if (bitsPerSample <= 16)
	    r = new AudioIntHQResamplerStereo<short>;
        else if (bitsPerSample <= 32)
	    r = new AudioFpHQResamplerStereo<int>;
        break;
    }

    return r;
}

AVM_END_NAMESPACE;
