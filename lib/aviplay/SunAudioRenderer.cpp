/*********************************************************

SunAudioRenderer implementation

*********************************************************/

#include "SunAudioRenderer.h"

#ifdef HAVE_SUNAUDIO

#include "avifile.h"
#include "except.h"
#include "cpuinfo.h"
#include "utils.h"
#include "avm_output.h"

#include <sys/ioctl.h>
#include <sys/audioio.h>
#include <unistd.h>
#include <stropts.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

#define Debug
#define __MODULE__ "SunAudioRenderer"

AVM_BEGIN_NAMESPACE;

int SunAudioMix::Mix(void* data, const void* src, uint_t n) const
{
    return ::write(m_iFd, src, n);
}

SunAudioRenderer::SunAudioRenderer(IAviReadStream* as, WAVEFORMATEX& Owf, const char* privcname)
:IAudioRenderer(as, Owf), m_iAudioFd(-1), m_pAudioMix(0)
{
    try
    {
	int audio_status = m_pAudiostream->StartStreaming();
	if(audio_status != 0)
	    throw FATAL("Failed to start streaming");

	m_pAudiostream->GetOutputFormat(&m_Owf, sizeof m_Owf);

	//	Debug std::cout << "Audio format " << m_Owf.nSamplesPerSec
	//	    << "/" << m_Owf.wBitsPerSample
	//	    << "/" << ((m_Owf.nChannels != 1) ? "stereo" : "mono")
	//	    << endl;

	m_iAudioFd = open("/dev/audio", O_WRONLY|O_NDELAY);
	if(m_iAudioFd < 0)
	    throw FATAL("Can't open audio device");

	//	m_bForce44KHz = RegAccess::ReadForce44KHz();
	//	if(m_Owf.nSamplesPerSec == 44100 && m_bForce44KHz)
	//	    m_bForce44KHz = false;

	audio_info_t audio_info;
	AUDIO_INITINFO(&audio_info);
	audio_info.play.channels    = m_Owf.nChannels;
	audio_info.play.precision   = m_Owf.wBitsPerSample;
	audio_info.play.sample_rate = 0 // m_bForce44KHz
	    ? 44100 : m_Owf.nSamplesPerSec;
	audio_info.play.encoding    = m_Owf.wBitsPerSample == 8
	    ? AUDIO_ENCODING_LINEAR8
	    : AUDIO_ENCODING_LINEAR;
	audio_info.play.samples     = 0;
	if(ioctl(m_iAudioFd, AUDIO_SETINFO, &audio_info) < 0)
	{
	    char failure_msg[80];
	    sprintf(failure_msg,
		    "ioctl(AUDIO_SETINFO) failed: chnl=%d, prec=%d, rate=%d, enc=%d",
		    audio_info.play.channels,
		    audio_info.play.precision,
		    audio_info.play.sample_rate,
		    audio_info.play.encoding);
	    throw FATAL(failure_msg);
	}
	m_pAudioMix = new SunAudioMix(m_iAudioFd);
    }
    catch(FatalError& error)
    {
	if(m_iAudioFd > 0)
	    ::close(m_iAudioFd);
	m_iAudioFd = -1;
	m_pAudiostream = 0;
	throw;
    }

    m_iSamplesSent = 0;

    m_pAudioThread = new PthreadTask(0, doAudioOut, (void*)this);
}

SunAudioRenderer::~SunAudioRenderer()
{
    //Debug cout<<"Destroying audio renderer"<<endl;
    m_pQueue->Lock();
    m_bQuit = true;
    m_pQueue->Broadcast();
    m_pQueue->Unlock();
    delete m_pAudioThread;
    delete m_pAudioMix;
    reset();
    ::close(m_iAudioFd);
    //Debug cout<<"Destroy() successful"<<endl;
}

int SunAudioRenderer::SetVolume(int volume)
{
    int r = IAudioRenderer::SetVolume(volume);
    if (r == 0)
    {
	audio_info_t audio_info;
	AUDIO_INITINFO(&audio_info);
	audio_info.play.gain = int(volume * AUDIO_MAX_GAIN / VOL_MAX);
	// not yet, initial 100% volume setting is much too loud
	// (at least on my SB16pci :-)
	//ioctl(m_iAudioFd, AUDIO_SETINFO, &audio_info);
    }
    return r;
}

double SunAudioRenderer::getRendererBufferTime() const
{
    double rate = m_Owf.nSamplesPerSec;
    double frame_time = 0;
    audio_info_t audio_info;
    if (m_bInitialized && ::ioctl(m_iAudioFd, AUDIO_GETINFO, &audio_info) == 0)
    {
	frame_time += (m_iSamplesSent - audio_info.play.samples);
	AVM_WRITE("audio renderer", 1, "SAMPLES queue %d, device %d - %d = %d, time = %f\n",
		  m_pQueue->GetSize(),
		  m_iSamplesSent, audio_info.play.samples,
		  m_iSamplesSent - audio_info.play.samples,
		  m_pQueue->GetBufferTime());
    }
    return frame_time / rate;
}

void* SunAudioRenderer::doAudioOut(void* arg)
{
    SunAudioRenderer& a = *(SunAudioRenderer*) arg;
    int reset_dev = 0;
    while (!a.m_bQuit)
    {
	if (!a.m_bInitialized)
	{
	    a.m_pQueue->Lock();
	    if (reset_dev)
	    {
		a.m_pQueue->Clear();
		a.reset();
		reset_dev = 0;
	    }
	    else
	    {
		AVM_WRITE("audio renderer", 0, "not yet m_bInitialized\n");
		a.m_pQueue->Wait();
	    }
	    a.m_pQueue->Unlock();
	    continue;
	} else
	    reset_dev=1;


	a.m_pQueue->Lock();
	if (a.m_pQueue->GetSize() < a.m_uiWriteSize
	    || a.m_bPaused )
	{
	    if (!a.m_bQuit)
		a.m_pQueue->Wait();
	    a.m_pQueue->Unlock();
	    continue;
	}

	int result = a.m_pQueue->Read(0, 8192, a.m_pAudioMix);

	if (result < 0)
	    perror("AudioQueue::write");
	else
            a.updateTimer();
	a.m_pQueue->Unlock();
    }
    AVM_WRITE("audio renderer", 1, "Exiting audio thread\n");
    return 0;
}

void SunAudioRenderer::pause(int state)
{
    AVM_WRITE("audio renderer", 1, "SunAudioRenderer::pause %d\n", state);

    audio_info_t audio_info;
    AUDIO_INITINFO(&audio_info);
    audio_info.play.pause = state;
    ::ioctl(m_iAudioFd, AUDIO_SETINFO, &audio_info);

    AVM_WRITE("audio renderer", 1, "Samples buffered in audio_queue %f\n",
	      m_pQueue->GetBufferTime());
}

int SunAudioRenderer::reset()
{
    if (m_iAudioFd)
    {
	/*
	 * clear data in the STREAMS queue, and wait for
	 * playback end.  Make sure the audio device is not
	 * 'paused' state, else the AUDIO_DRAIN ioctl hangs
	 * forever.
	 */
	ioctl(m_iAudioFd, I_FLUSH, FLUSHW);

	audio_info_t audio_info;
	AUDIO_INITINFO(&audio_info);
	audio_info.play.pause = 0;
	ioctl(m_iAudioFd, AUDIO_SETINFO, &audio_info);

	ioctl(m_iAudioFd, AUDIO_DRAIN, 0);

	AUDIO_INITINFO(&audio_info);
	audio_info.play.samples = 0;
	ioctl(m_iAudioFd, AUDIO_SETINFO, &audio_info);
	m_iSamplesSent = 0;
	return 0;
    }
    return -1;
}

AVM_END_NAMESPACE;

#endif // HAVE_SUNAUDIO
