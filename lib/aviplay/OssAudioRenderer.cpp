/*********************************************************

	DSP OssAudioRenderer implementation

*********************************************************/

#include "OssAudioRenderer.h"

#include "AudioQueue.h"
#include "avifile.h"
#include "avm_cpuinfo.h"
#include "avm_output.h"

#ifdef __NetBSD__
#include <soundcard.h>
#else
#include <sys/soundcard.h>
#endif

#include <sys/ioctl.h>
#include <errno.h>
#include <string.h> // strerror
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#ifndef AFMT_AC3
#define AFMT_AC3 0x400
#endif

AVM_BEGIN_NAMESPACE;

static const char* DSP_DEVICE = "/dev/dsp";
static const char* DEVDSP_DEVICE = "/dev/sound/dsp";
static const char* MIXER_DEVICE = "/dev/mixer";
static const char* DEVMIXER_DEVICE = "/dev/sound/mixer";

int OssAudioMix::Mix(void* data, const void* src, uint_t n) const
{
    //int ds = open("/tmp/wav", O_WRONLY | O_CREAT | O_APPEND, 0666); write(ds, src, n); close(ds);
    return ::write(m_iFd, src, n);
}

int OssAudioRenderer::Init()
{
    m_iMixFd = ::open(DEVMIXER_DEVICE, O_RDONLY);
    if (m_iMixFd < 0)
	m_iMixFd = ::open(MIXER_DEVICE, O_RDONLY);

    m_iAudioFd = ::open(DEVDSP_DEVICE, O_WRONLY
#ifdef __linux__
		      | O_NDELAY
#endif
		     );
    if (m_iAudioFd < 0)
	m_iAudioFd = ::open(DSP_DEVICE, O_WRONLY
#ifdef __linux__
			    | O_NDELAY
#endif
			   );
    for (;;)
    {
	if (m_iAudioFd < 0)
	{

	    AVM_WRITE("OSS audio renderer", "Can't open %s audio device: %s\n", DSP_DEVICE, strerror(errno));
	    break;
	}
#ifdef __linux__
	int flag = fcntl(m_iAudioFd, F_GETFL, 0);
	if (flag < 0)
	    break;
	flag &= ~O_NDELAY;
	if (fcntl(m_iAudioFd, F_SETFL, flag) < 0)
	    break;
#endif
	m_uiWriteSize = m_Owf.nBlockAlign;
	if (m_Owf.wFormatTag == 0x01)
	{
	    if (reset() != 0)
		return -1;
	    audio_buf_info zz;
	    ioctl(m_iAudioFd, SNDCTL_DSP_GETOSPACE, &zz);
	    m_iSndLimit = zz.bytes;
	    ioctl(m_iAudioFd, SNDCTL_DSP_GETBLKSIZE, &m_uiWriteSize);
	    AVM_WRITE("OSS audio renderer", "frags=%d  size=%db  buffer=%db (%db)\n",
		      zz.fragments, zz.fragsize, m_iSndLimit, m_uiWriteSize);
	}
	else if (m_Owf.wFormatTag == 0x2000)
	{
	    m_uiWriteSize = m_Owf.nBlockAlign;
	    // AC3 passthrough support
	    int tmp = AFMT_AC3;
	    if (ioctl(m_iAudioFd, SNDCTL_DSP_SETFMT, &tmp) < 0 || tmp != AFMT_AC3)
	    {
		AVM_WRITE("OSS audio renderer", "AC3 SNDCTL_DSP_SETFMT failed"
			  "** Have you set emu10k1 into proper state?? (see README) **\n");
                break;
	    }
	    else
		AVM_WRITE("OSS audio renderer", "AC3 pass-through enabled\n");
	}

	m_AudioMix.m_iFd = m_iAudioFd;
        mixer(GET);

	m_pAudioThread = new PthreadTask(0, doAudioOut, (void*)this);
        return 0;
    }
    return -1;
}

OssAudioRenderer::~OssAudioRenderer()
{
    AVM_WRITE("OSS audio renderer", 1, "destroy\n");
    m_pQueue->Lock();
    m_bQuit = true;
    m_pQueue->Broadcast();
    m_pQueue->Unlock();
    delete m_pAudioThread;
    m_pAudioThread = 0; // so reset will work for 0x2000
    if (m_iAudioFd >= 0)
    {
        if (m_uiWriteSize == 4096)
	    reset();
	::close(m_iAudioFd);
    }
    if (m_iMixFd >= 0)
	::close(m_iMixFd);
}

int OssAudioRenderer::GetBalance()
{
    return (mixer(GET) == 0) ? m_iBalance :  -1;
}

int OssAudioRenderer::GetVolume()
{
    return (mixer(GET) == 0) ? m_iVolume : -1;
}

int OssAudioRenderer::SetBalance(int balance)
{
    return (IAudioRenderer::SetBalance(balance) == 0
	    && mixer(SET) == 0) ? m_iBalance :  -1;
}

int OssAudioRenderer::SetVolume(int volume)
{
    return (IAudioRenderer::SetVolume(volume) == 0
	    && mixer(SET) == 0) ? m_iVolume : -1;
}

int OssAudioRenderer::mixer(Act a)
{
    int r = -1;
    if (m_iMixFd >= 0 && m_Owf.wFormatTag != 0x2000)
    {
	int devm;
	ioctl(m_iMixFd, SOUND_MIXER_READ_DEVMASK, &devm);
	if (devm & SOUND_MASK_PCM)
	{
            int t = 0;
	    switch (a)
	    {
	    case GET:
		ioctl(m_iMixFd, SOUND_MIXER_READ_PCM, &t);
		//printf("GET %x\n", t);
                m_iVolume = (t >> 8) & 0x7f;
		t &= 0x7f;
		if (m_iVolume >= t)
		{
                    if (m_iVolume > 6)
			m_iBalance = BAL_MAX - t * (BAL_MAX / 2) / m_iVolume;
                    m_iVolume = m_iVolume * VOL_MAX / OSS_VOL_MAX;
		}
		else
		{
		    if (t > 6)
			m_iBalance = m_iVolume * (BAL_MAX / 2) / t;
		    m_iVolume = t * VOL_MAX / OSS_VOL_MAX;
		}
		//printf("GET  VOLUME %d    BALANCE %d\n", m_iVolume, m_iBalance);
                break;
	    case SET:
		t = m_iVolume * OSS_VOL_MAX / VOL_MAX;
		if (m_iBalance < (BAL_MAX / 2))
		    t = ((t * m_iBalance / (BAL_MAX / 2)) << 8) + t;
                else
		    t = (t << 8) + t * (BAL_MAX - m_iBalance) / (BAL_MAX / 2);
		//printf("SETVOL  %x\n", t);
		ioctl(m_iMixFd, SOUND_MIXER_WRITE_PCM, &t);
                break;
	    }
            r = 0;
	}
    }
    return r;
}

void* OssAudioRenderer::doAudioOut(void* arg)
{
    OssAudioRenderer& a = *(OssAudioRenderer*)arg;
    a.m_pQueue->Lock();
    while (!a.m_bQuit)
    {
	uint_t wsize = a.m_pQueue->GetSize();
	audio_buf_info zz;
	ioctl(a.m_iAudioFd, SNDCTL_DSP_GETOSPACE, &zz);
	//AVM_WRITE("audio renderer", "ospace %d  %d   frag %d  fsize %d\n", zz.bytes,
	//	  zz.fragments*zz.fragsize, zz.fragments, zz.fragsize);
	zz.fragments *= zz.fragsize;
        // checking for EOF to avoid busy loop
	if (!a.m_bInitialized || a.m_bPaused
	    || (wsize < a.m_uiWriteSize
		&& (!wsize || !a.m_pAudiostream->Eof()))
	    || zz.fragments == 0)
	{
	    //printf("WAIT1 %d  %d\n", a.m_bPaused, a.m_pQueue->GetSize());
	    //printf("OSSSLEEP  %d  %d  %d  %d\n", wsize, a.m_pAudiostream->Eof(), a.m_bInitialized, a.m_bPaused);
	    a.m_pQueue->Wait(0.02);
	    continue;
	}

	if (wsize > a.m_uiWriteSize)
            wsize = a.m_uiWriteSize;
#if 0
	static int64_t las = longcount();
	int64_t tlas = longcount();
	AVM_WRITE("OSS audio renderer", "diff %f    %f\n", to_float(tlas, las), a.getRendererBufferTime());
	las = tlas;
#endif
	int result = a.m_pQueue->Read(0, wsize, &a.m_AudioMix);
	//printf("wtime %f   %f\n", to_float(longcount(), ct), to_float(longcount(), a.m_lTimeStart) );

	//AVM_WRITE("audio renderer", "Wrote %d bytes\n", result);
	if (result < (int) a.m_uiWriteSize)
	{
            if (result < 0)
		perror("AudioQueue::write");
	    else
	    {
		// this should assure the we will actually
                // hear the very last bytes from buffer
		const uint_t sz = 32768/4;
		uint32_t* d = new uint32_t[sz];
		int r = 0;
		if (a.m_Owf.wBitsPerSample <= 8)
		    r = 0x80808080;
		for (uint_t i = 0; i < sz; i++) d[i] = r;
		a.m_AudioMix.Mix(0, d, sz*4);
		delete[] d;
	    }
	}
	else
            a.updateTimer();
    }
    a.m_pQueue->Unlock();

    AVM_WRITE("OSS audio renderer", 1, "audio thread finished\n");
    return 0;
}

int OssAudioRenderer::reset()
{
    int tmp;

    if (m_pAudioThread && m_Owf.wFormatTag != 0x01)
        return 0; // avoid RESET for AC3 passthrough code

#ifdef SNDCTL_DSP_RESET
    ioctl(m_iAudioFd, SNDCTL_DSP_RESET, 0);
#endif

    tmp = (8 << 16) | 12; // 16 frags  of  2^12 bytes
    ioctl(m_iAudioFd, SNDCTL_DSP_SETFRAGMENT, &tmp);

    tmp = m_Owf.nChannels - 1;
    if (ioctl(m_iAudioFd, SNDCTL_DSP_STEREO, &tmp) != 0
	|| tmp != (m_Owf.nChannels - 1))
    {
	AVM_WRITE("OSS audio renderer", "WARNING: ioctl(stereo) (%d != %d)\n",
		  tmp, (m_Owf.nChannels - 1));
	return -1;
    }

    tmp = m_Owf.wBitsPerSample;
    if (ioctl(m_iAudioFd, SNDCTL_DSP_SAMPLESIZE, &tmp) < 0)
    {
	AVM_WRITE("OSS audio renderer", "WARNING: ioctl(samplesize)\n");
        return -1;
    }

    tmp = (m_uiUseFreq) ? m_uiUseFreq : m_Owf.nSamplesPerSec;
    if (ioctl(m_iAudioFd, SNDCTL_DSP_SPEED, &tmp) != 0)
    {
	AVM_WRITE("OSS audio renderer", "WARNING: ioctl(speed)\n");
        return -1;
    }
    return 0;
}

double OssAudioRenderer::getRendererBufferTime() const
{
    if (m_Owf.wFormatTag != 0x01)
        return 0.0; // to be solved - problems with SBLive and AC3

    audio_buf_info zz;
    int r;
    switch (m_iDelayMethod)
    {
    case 2:
#ifdef SNDCTL_DSP_GETODELAY
	if (ioctl(m_iAudioFd, SNDCTL_DSP_GETODELAY, &r)!=-1)
	    break;
#endif
	m_iDelayMethod--; // fallback if not supported
    case 1:
	if (ioctl(m_iAudioFd, SNDCTL_DSP_GETOSPACE, &zz)!=-1)
	{
	    r = m_iSndLimit - zz.bytes;
            break;
	}
	m_iDelayMethod--; // fallback if not supported
    default:
	r = m_iSndLimit; // full buffer
    }
#if 0
    count_info info;
    if (ioctl(m_iAudioFd, SNDCTL_DSP_GETOPTR, &info)!=-1)
    {
	printf("GETOPTR  %f   %f   %f    %d   %d   %d\n",
	       info.bytes / m_dOwfBPS,
	       to_float(longcount(), m_lTimeStart), m_dAudioRealpos,
	       info.bytes, info.blocks, info.ptr);
    }
#endif
    //AVM_WRITE("audio renderer", "GETRENDER %f   %d  %d\n", r / (double) m_pQueue->GetBytesPerSec(), r, m_pQueue->GetBytesPerSec());
    return r / (double) m_pQueue->GetBytesPerSec();
}

AVM_END_NAMESPACE;
