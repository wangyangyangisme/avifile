/*********************************************************

	Implementation of base audio renderer class

*********************************************************/

#include "IAudioRenderer.h"
#include "AudioQueue.h"

#include "avm_cpuinfo.h"
#include "avm_output.h"
#include "audiodecoder.h"
#include "avifile.h"
#include "utils.h"

#include <stdio.h>

AVM_BEGIN_NAMESPACE;

IAudioRenderer::IAudioRenderer(IReadStream* as, WAVEFORMATEX& Owf)
    : m_pAudiostream(as), m_pQueue(0),
    m_lTimeStart(0), m_uiSamples(0),
    m_Owf(Owf), m_iBalance(BAL_MAX/2), m_iVolume(VOL_MAX),
    m_bQuit(false), m_bPaused(false), m_bInitialized(false)
{
    m_dSeekTime = m_dStreamTime = m_dAudioRealpos = m_dPauseTime = m_pAudiostream->GetTime();

    // needed for bitrate in nAvgBytesPerSec
    WAVEFORMATEX Inwf;
    m_pAudiostream->GetAudioFormat(&Inwf, sizeof(Inwf));
    m_pAudiostream->GetOutputFormat(&m_Iwf, sizeof(m_Iwf));

    // format can't be changed
    m_Owf.wFormatTag = m_Iwf.wFormatTag;
    m_Owf.nBlockAlign = m_Iwf.nBlockAlign;

    if (m_Owf.nChannels == 0)
    {
	m_Owf.nChannels = m_Iwf.nChannels;
	if (m_Owf.nChannels > 2)
	    m_Owf.nChannels = 2;
    }

    if (m_Owf.nSamplesPerSec == 0)
	m_Owf.nSamplesPerSec = m_Iwf.nSamplesPerSec;

    if (m_Owf.wBitsPerSample == 0)
    {
	m_Owf.wBitsPerSample = m_Iwf.wBitsPerSample;
        if (m_Owf.wBitsPerSample > 16)
	    m_Owf.wBitsPerSample = 16;
    }

    m_Owf.nBlockAlign = m_Owf.nChannels	* ((m_Owf.wBitsPerSample + 7) / 8);
    m_Owf.nAvgBytesPerSec = m_Owf.nSamplesPerSec * m_Owf.nBlockAlign;

    m_dOwfBPS = m_Owf.nAvgBytesPerSec;
    m_dIwfBPS = m_Iwf.nSamplesPerSec * m_Owf.nChannels /* tricky */
	* ((m_Owf.wBitsPerSample + 7) / 8);

    char b[200];
    avm_wave_format(b, sizeof(b), &Inwf);
    AVM_WRITE("audio renderer", "src %s\n", b);
    avm_wave_format(b, sizeof(b), &m_Owf);
    AVM_WRITE("audio renderer", "dst %s\n", b);

    // no need to catch - we allocate m_pQueue as last object
    m_pQueue = new AudioQueue(m_Iwf, m_Owf);
    SetAsync(0.0);
}

IAudioRenderer::~IAudioRenderer()
{
    delete m_pQueue;
}

const char* IAudioRenderer::GetAudioFormat() const
{
    return m_pAudiostream->GetAudioDecoder()->GetCodecInfo().GetName();
}

bool IAudioRenderer::Eof() const
{
    //if (m_pAudiostream->Eof())
    //    AVM_WRITE("audio renderer", "Audio EOF: %d --- bt:%f  ren:%f  %f\n", m_pAudiostream->Eof(), GetBufferTime(), getRendererBufferTime(), m_pAudiostream->GetTime());
    return m_pAudiostream->Eof() && ((GetBufferTime() - getRendererBufferTime()) < 0.01);
}

int IAudioRenderer::Extract()
{
    if (m_pAudiostream->Eof() || m_pQueue->IsFull())
	return -1; // reader is supposed to wait!!

    uint_t frame_size = m_pAudiostream->GetFrameSize();
    if (frame_size < 10000)
	frame_size = 10000;
    uint_t ocnt;
    uint_t samples;
    char* frame = new char[2 * frame_size]; // safety reason
    //for (int i = 0; i < 64; i++) printf("%d 0x%x\n", i, (unsigned char) m_pcLocalFrame[i]);
    m_pAudiostream->ReadFrames(frame, frame_size, frame_size, samples, ocnt);
    //if (m_uiSamples < 100000) printf("Ocnt %d  %d  %d\n", ocnt, samples, frame_size);

    if ((int)ocnt <= 0)
    {
        delete[] frame;
	if (ocnt == 0) // && m_uiSamples > 0)
	{
	    m_dSeekTime = m_pAudiostream->GetTime();
	    m_uiSamples = 0;
	    AVM_WRITE("audio renderer", 1, "new seektime set: %f  (eof:%d)\n", m_dSeekTime, m_pAudiostream->Eof());
	}
	return 0;
    }
    if (samples > frame_size)
	AVM_WRITE("audio renderer", "OOPS: samples (%d) > one_frame_sound (%d) at %s\n", samples, frame_size, __FILE__);
    if (ocnt > frame_size)
    {
	AVM_WRITE("audio renderer", "OOPS: ocnt (%d)  > one_frame_sound (%d) at %s\n", ocnt, frame_size, __FILE__);
	ocnt = frame_size;
    }

    m_pQueue->Lock();
    m_pQueue->Write(frame, ocnt);
    m_uiSamples += ocnt;
    //m_dStreamTime = m_dSeekTime + m_uiSamples / m_dIwfBPS;
    m_dStreamTime = m_pAudiostream->GetTime();
    //if (m_uiSamples < 10000000) printf("AUDIDIFF %f  na: %f  a: %f   smp: %d  %d\n", m_dStreamTime - m_pAudiostream->GetTime(), m_dSeekTime, m_pAudiostream->GetTime(), m_uiSamples, ocnt);
    m_pQueue->Unlock();
    //AVM_WRITE("audio renderer", 1, "extracted %d sample(s) (%d bytes) newtime: %f  %d %f\n", samples, ocnt, m_dStreamTime, m_uiSamples, m_dOwfBPS);
    return 0;
}

double IAudioRenderer::GetBufferTime() const
{
    if (!m_bInitialized)
        return 0.0;

    //AVM_WRITE("audio renderer", "GBT %f   %f\n", m_pQueue->GetBufferTime(), getRendererBufferTime());
    double btime = m_pQueue->GetBufferTime() + getRendererBufferTime();
    //printf("BTIME %f  %f  %f\n", btime, m_pQueue->GetBufferTime(),getRendererBufferTime());
    return btime;
}

double IAudioRenderer::GetCacheSize() const
{
    return m_pAudiostream->CacheSize();
}

double IAudioRenderer::GetLengthTime() const
{
    return m_pAudiostream->GetLengthTime();
}

double IAudioRenderer::GetTime()
{
    //printf("GetTime %f  %d\n", m_dPauseTime, m_bPaused);
    if (m_dPauseTime != -1.0)
	return m_dPauseTime;

    double actual_time = to_float(longcount(), m_lTimeStart)
        + m_dAudioRealpos - m_fAsync;

    //AVM_WRITE("audio renderer", "stream: %f    buffered: %f   actual: %f\n",
    //          m_pAudiostream->GetTime(), GetBufferTime(), actual_time);
    //actual_time = m_pAudiostream->GetTime() - GetBufferTime();
    return (actual_time > 0) ? actual_time : 0.0;
}

int IAudioRenderer::Pause(bool state)
{
    m_pQueue->Lock();
    if (!m_bInitialized)
    {
	m_pQueue->Unlock();
	return -1;
    }

    if (m_bPaused != state)
    {
	m_bPaused = state;
	pause((state) ? 1 : 0);
	if (state)
	{
	    m_dPauseTime = GetTime();
	    reset();
	}
    }
    m_pQueue->Broadcast(); // in case audio renderer waits for new data
    m_pQueue->Unlock();
    return 0;
}

int IAudioRenderer::SeekTime(double pos)
{
    m_pQueue->Lock();
    pos += m_fAsync;
    if (pos < 0.)
	pos = 0.;
    int hr = m_pAudiostream->SeekTime(pos);
    m_lTimeStart = 0;

    if (m_bInitialized)
    {
	m_pAudiostream->SkipTo(pos);
	m_pQueue->Clear();
	reset();
    }
    m_dSeekTime = m_dStreamTime = m_dAudioRealpos = m_dPauseTime = m_pAudiostream->GetTime();
    m_uiSamples = 0;
    //AVM_WRITE("audio renderer", "AUDIOREALPOS  %f  to %f\n", m_dAudioRealpos, pos);
    m_pQueue->Unlock();
    return hr;
}

/*
int IAudioRenderer::Skip(double skiptime)
{
    Locker locker(m_Mutex);
    int r = m_pAudiostream->SkipTo(skiptime);
    m_dStreamTime = m_dAudioRealpos = m_pAudiostream->GetTime();

    return r;
}
*/

int IAudioRenderer::GetBalance()
{
    return m_iBalance;
}

int IAudioRenderer::GetVolume()
{
    return m_iVolume;
}

int IAudioRenderer::SetBalance(int balance)
{
    if (balance < 0 || balance > BAL_MAX)
	return -1;

    m_iBalance = balance;
    return 0;
}

int IAudioRenderer::SetVolume(int volume)
{
    if (volume < 0 || volume > VOL_MAX)
	return -1;

    m_iVolume = volume;
    return 0;
}

void IAudioRenderer::Start()
{
    m_pQueue->Lock();
    if (!m_bInitialized)
    {
	m_bInitialized = true;
	m_bPaused = false;
	pause(0);
    } else {
	AVM_WRITE("audio renderer", "already started\n");
    }
    m_pQueue->Unlock();
}

void IAudioRenderer::Stop()
{
    m_pQueue->Lock();
    if (m_bInitialized)
    {
	m_bInitialized = false;
	m_pQueue->Clear();
        pause(1);
    }
    m_pQueue->Unlock();
    return;
}

void IAudioRenderer::updateTimer()
{
    double nt = GetStreamTime() - GetBufferTime() - m_fAsync;
    //printf("NT time %f            %f\n", nt - GetTime(), nt);
    if (nt < 0.)  nt = 0.;

    if (m_dPauseTime != -1.0)
    {
	m_dAudioRealpos = nt;
	m_lTimeStart = longcount();
	m_dPauseTime = -1.0;
    }
    else
    {
	static const double df = 0.04;
	const double st = GetTime();
	const double dt = st - nt;

	//AVM_WRITE("audio renderer", "nt:%.4f st:%.4f dt:%.4f t:%.4f bt:%.4f  df:%.4f\n", nt, st, dt,
	//	  GetStreamTime(), GetBufferTime(), getRendererBufferTime());
	if (dt < -df || dt > df)
	{
	    AVM_WRITE("audio renderer", 1, "stime %f  %f  dt: %f   t: %f   b: %f  rt: %f\n",
		      nt, st, dt, GetStreamTime(), GetBufferTime(), getRendererBufferTime());
	    // we need bigger jump when sound is going slower!
	    m_dAudioRealpos -= dt / 20;
	}
    }
}

AVM_END_NAMESPACE;
