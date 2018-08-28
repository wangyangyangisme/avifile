/**********************************************

	Audio queue implementation

**********************************************/

#include "AudioQueue.h"
#include "AudioCleaner.h"
#include "IAudioResampler.h"
#include "avm_output.h"

#if 1
// slower but has much better precision
#define RESAMPLERSTEREO AudioFpHQResamplerStereo
#define RESAMPLERMONO AudioFpHQResamplerMono
#include "AudioFpHQResampler.h"
#else
// faster but not very good method
#define RESAMPLERSTEREO AudioIntResamplerStereo
#define RESAMPLERMONO AudioIntResamplerMono
#include "AudioIntResampler.h"
#endif

#include "avm_cpuinfo.h"
#include "utils.h"

#include <unistd.h> //write
#include <string.h> //memcpy
#include <stdio.h>

//#undef Debug
//#define Debug if (0)
// using different strategy - caller is responsible for holding lock!

AVM_BEGIN_NAMESPACE;

template<class T> inline T mymin(const T x, const T y)
{
    return (x < y) ? x : y;
}

AudioQueue::AudioQueue(WAVEFORMATEX& Iwf, WAVEFORMATEX& Owf) : m_Bufs(64)
{
    m_dRemains = 0.0;
    m_pResampler = 0;
    m_pCleaner = 0;
    m_Iwf = Iwf;
    m_Owf = Owf;
    m_uiBytesPerSec = (m_Owf.nChannels * ((m_Owf.wBitsPerSample + 7) / 8)
		       * m_Owf.nSamplesPerSec);

    for (uint_t i = 0; i < m_Bufs.capacity(); i++)
	m_Bufs[i].mem = 0;

    m_uiBufSize = 0;
    m_bCleared = false;

    //printf("QUEUE %x   %x\n", m_Iwf.wFormatTag, m_Owf.wFormatTag);
    if (m_Owf.wFormatTag == 0x01)
    {
	uint_t clearsz = (m_uiBytesPerSec / 10) & ~3U;  // 0.1 sec

        // for PCM (0x01) we could add some silent to remove clics & pops
	m_pCleaner = CreateAudioCleaner(m_Owf.nChannels, m_Owf.wBitsPerSample,
					clearsz);
    }
}

AudioQueue::~AudioQueue()
{
    m_Mutex.Lock(); // just for safety
    Clear();
    m_Mutex.Unlock();
    delete m_pCleaner;
    delete m_pResampler;
}

void AudioQueue::Clear()
{
    AVM_WRITE("aviplay", 2, "AudioQueue: clear\n");

    for (uint_t i = 0; i < m_Bufs.capacity(); i++)
    {
	delete[] m_Bufs[i].mem;
	m_Bufs[i].mem = 0;
    }
    m_Bufs.clear();
    m_bCleared = true;
    m_uiBufSize = 0;
    if (m_pCleaner)
	m_pCleaner->soundOff(0, 0);
    m_Cond.Broadcast();
}

void AudioQueue::DisableCleaner()
{
    delete m_pCleaner;
    m_pCleaner = 0;
}


/*  ineffective */
uint_t AudioQueue::Resample(void* dest, const void* src, uint_t src_size)
{
    if (m_Iwf.wBitsPerSample != m_Owf.wBitsPerSample
	|| m_Iwf.nChannels != m_Owf.nChannels
	|| (m_Iwf.nChannels != 1 && m_Iwf.nChannels != 2))
    {
#if 0
	cerr << "AudioQueue::resample()  unsupported resampling conversion!" << endl
	    << "From:  bps: " << m_Iwf.wBitsPerSample
	    << "  ch: " << m_Iwf.nChannels
	    << "  To:  bps: " << m_Owf.wBitsPerSample
	    << "  ch: " << m_Owf.nChannels << endl;
#endif
	return 0;
    }

    if (!m_pResampler || (int)m_pResampler->getBitsPerSample() != m_Owf.wBitsPerSample)
    {
	delete m_pResampler;

	m_pResampler = CreateHQResampler(m_Owf.nChannels, m_Owf.wBitsPerSample);

	if (!m_pResampler)
	{
	    AVM_WRITE("aviplay", "AudioQueue::resample()  creation of resampler failed\n");
	    return 0;
	}
    }
    double ndest_size = (double) src_size / m_Iwf.nSamplesPerSec * m_Owf.nSamplesPerSec
	/ (m_Owf.wBitsPerSample / 8 * m_Owf.nChannels);

    src_size /= (m_Iwf.wBitsPerSample / 8 * m_Iwf.nChannels);
    uint_t dest_size = (uint_t) ndest_size;

    m_dRemains += (ndest_size - dest_size);
    if (m_dRemains > 1.0)
    {
	m_dRemains -= 1.0;
        dest_size++;
    }

    //cout << "dest_size " << dest_size << "    orig " << ndest_size << endl;

    AVM_WRITE("aviplay", 2, "AudioQueue::resample()  freq: %d   ->   %d\n", src_size, dest_size);
    if (dest_size > 0)
	m_pResampler->resample(dest, src, dest_size, src_size);

    //cout << "resampled " << src_size<< " to " << dest_size
    //    << " >>> " << src_size << "   b:" << m_Owf.wBitsPerSample
    //    << "  ch:" << m_Owf.nChannels << endl;
    return (uint_t) (dest_size * (m_Owf.wBitsPerSample / 8) * m_Owf.nChannels);
}

int AudioQueue::Read(void* data, uint_t count, const IAudioMix* amix)
{
    //AVM_WRITE("aviplay", 2, "AudioQueue: read: %d\n", count);

    uint_t bread = 0;
    while (bread < count && !m_Bufs.empty())
    {
	chunk& rchunk = m_Bufs.front();
        int r = rchunk.size - rchunk.rsize;
	if (r > int(count - bread))
	    r = (count - bread);

	if (amix == 0)
	    memcpy((char*)data + bread, rchunk.mem + rchunk.rsize, r);
	else
	{
	    r = amix->Mix((char*)data + bread, rchunk.mem + rchunk.rsize, r);
	    if (r <= 0)
                break;
	}
	//printf("ReadAudioChunk %d %d\n", rchunk.rsize, r);
	rchunk.rsize += r;
	bread += r;
	if (rchunk.rsize >= rchunk.size)
	{
	    delete[] rchunk.mem;
	    rchunk.mem = 0;
	    m_Bufs.pop();
	}
    }

    if (m_Bufs.empty() && !bread)
	AVM_WRITE("aviplay", "AudioQueue::Read() Warning: audio queue drain\n");

    m_uiBufSize -= bread;
    m_Cond.Broadcast();

    return bread;
}

int AudioQueue::Write(char* data, uint_t count)
{
    //AVM_WRITE("aviplay", 2, "AudioQueue::Write(%d) %d\n", count, m_uiBufSize);

    uint_t new_count = count * m_Owf.nSamplesPerSec / m_Iwf.nSamplesPerSec + 16;

    chunk newch;

    newch.mem = data;
    newch.size = count;

    if (m_Iwf.nSamplesPerSec != m_Owf.nSamplesPerSec)
    {
	newch.mem = new char[new_count];
	newch.size = Resample(newch.mem, data, count);
	delete[] data;
    }

#if 0
    // checking if the sample continues correctly
    short *p = (short*) (m_pcAudioFrame + ((m_uiFrameIn > 64) ? m_uiFrameIn : 64));

    for (int i = -32; i < 32; i++)
    {
	if (!(i % 8))
	    printf("\n%4d ", i);
	//printf("  0x%4.4x", abs(p[i] - p[i-2]) & 0xffff);
	printf("  0x%4.4x", p[i] & 0xffff);
    }
#endif

    if (m_bCleared && m_pCleaner)
	m_bCleared = (m_pCleaner->soundOn(newch.mem, newch.size) != 0);

    newch.rsize = 0;
    m_Bufs.push(newch);
    m_uiBufSize += newch.size;
    m_Cond.Broadcast();

    return 0;
}

AVM_END_NAMESPACE;
