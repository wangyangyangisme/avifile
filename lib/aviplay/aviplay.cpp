/********************************************************

	AVI player object
	Copyright 2000 Eugene Kuznetsov  (divx@euro.ru)

*********************************************************/

#include "aviplay_impl.h"

#ifdef HAVE_LIBSDL
#include "SdlAudioRenderer.h"
#endif

#ifdef HAVE_OSS
#include "OssAudioRenderer.h"
#endif

#ifdef HAVE_SUNAUDIO
#include "SunAudioRenderer.h"
#endif

#include "CopyAudioRenderer.h"

#include "avm_cpuinfo.h"
#include "avm_creators.h"
#include "avm_except.h"
#include "avm_fourcc.h"
#include "avm_output.h"
#include "utils.h"


#include <unistd.h> // geteuid
#include <string.h> // memcpy
#include <stdlib.h> // getenv, free()
#include <stdio.h>

AVM_BEGIN_NAMESPACE;

#define __MODULE__ "IAviPlayer"

//int AVIPLAY_DEBUG = 0;
//#undef Debug
//#define Debug if(AVIPLAY_DEBUG)

#undef min
#define min(X,Y) ((X)<(Y)?(X):(Y))


AviPlayer::AviPlayer(const char* filename, int depth, const char* subname,
                     unsigned int flags,
		     const char* vcodec, const char* acodec)
    :m_Drop("Drop", 50), m_Quality("Quality", 25)
{
    m_pVideostream = 0;
    m_pAudiostream = 0;
    m_bInitialized = false;
    m_bHangup = false;
    m_bPaused = false;
    m_bQuit = false;
    m_bBuffering = false;
    m_bCallSync = false;
    m_bConstructed = false;
    m_bDropping = false;
    m_iLockCount = 0;
    m_lTimeStart = 0;
    m_dFrameStart = 0;
    m_dLastFrameStart = 0;
    m_iFramesVideo = 0;
    m_iFrameDrop = 0;
    m_pClip = 0;
    m_pClipAudio = 0;
    m_fLastSyncTime = 0.0;
    m_fLastDiff = 0.;
    m_pAudiofunc = 0;
    m_pAudioRenderer = 0;
    m_fAsync = 0.0;
    m_fSubAsync = 0.0;
    m_dVframetime = 0.04;
    m_dLastAudioSync = 0.0;
    m_fLastSleepTime = 0.0;
    m_bCallSync = false;
    m_Filename = filename;
    m_Subfilename = subname ? subname : "";
    m_iDepth = depth;
    m_pKillhandler = 0;
    m_pSubtitles = 0;
    m_pSubline = 0;
    m_iWidth = 0;
    m_iHeight = 0;
    m_CSP = 24;
    m_pAudioThread =
	m_pVideoThread =
	m_pDecoderThread = 0;

    m_bQualityAuto = false;
    m_bVideoBuffered = false;
    m_bVideoDirect = false;

    m_uiPgPrevPos = ~0UL;
    m_lPgPrevTime = 0LL;

    if (vcodec && strlen(vcodec))
	m_vcodec = vcodec;
    if (acodec && strlen(acodec))
	m_acodec = acodec;

    memset(propertyRead, 0, sizeof(propertyRead));

    // remember effective ids at the creation time of the object
    // and use the real user id until we really need the euid
    m_iEffectiveUid = geteuid();
    m_iEffectiveGid = getegid();
    if ((int) getuid() != (int) m_iEffectiveUid)
	seteuid(getuid());
    if ((int) getgid() != (int) m_iEffectiveGid)
	setegid(getgid());

    //m_bAudioMute = (getenv("AVIPLAY_MUTE_AUDIO") != 0);
    m_bVideoMute = (getenv("AVIPLAY_MUTE_VIDEO") != 0) ? true : false;

    m_bVideoAsync = (getenv("AVIPLAY_VIDEO_ASYNC") != 0) ? true : false;

    const char* ad = getenv("AVIPLAY_DEBUG");
    if (ad)
    {
	int AVIPLAY_DEBUG = atoi(ad);
	if (AVIPLAY_DEBUG)
	{
	    avm::out.setDebugLevel("aviplayxx", 4);
	    AVM_WRITE("aviplay", "Debug is on %d\n", AVIPLAY_DEBUG);
	}
    }

    if (m_bVideoAsync)
	AVM_WRITE("aviplay", 1, "Video is running asynchronously\n");

    m_pClip = CreateReadFile(filename, flags);
}

AviPlayer::~AviPlayer()
{
    Stop();

    assert(!m_bQuit);
    m_bQuit = true;
    if (!IsRedirector())
    {
	if (m_pVideostream)
	    m_pVideostream->StopStreaming();
	if (m_pAudiostream)
	    m_pAudiostream->StopStreaming();

	while (!m_bInitialized)
	    unlockThreads();

	//avm_usleep(10000);
	//printf("VT\n");
	delete m_pVideoThread;
	delete m_pDecoderThread;
	delete m_pAudioThread;
    }
    if (m_pClipAudio)
    {
	AVM_WRITE("aviplay", "Closing audio clip\n");
	delete m_pClipAudio;
        m_pClipAudio = 0;
    }
    if (m_pClip)
    {
	AVM_WRITE("aviplay", "Closing clip\n");
	delete m_pClip;
        m_pClip = 0;
    }

    while (m_VideoRenderers.size() > 0)
    {
	delete m_VideoRenderers.back();
	m_VideoRenderers.pop_back();
    }

    if (m_iFramesVideo)
	AVM_WRITE("aviplay", "Played %d video frames ( %f%% drop )\n",
	       m_iFramesVideo, m_iFrameDrop*100./m_iFramesVideo);
    if (m_pSubtitles)
	subtitle_close(m_pSubtitles);
    if (m_pSubline)
        subtitle_line_free(m_pSubline);
}

void AviPlayer::construct()
{
    if (IsRedirector())
    {
	AVM_WRITE("aviplay", "Redirector\n");
	return;
    }

    int def_audio, def_video, asynctm, avol, subasync;
    Get(AUDIO_STREAM, &def_audio,
	VIDEO_STREAM, &def_video,
	ASYNC_TIME_MS, &asynctm,
	SUBTITLE_ASYNC_TIME_MS, &subasync,
	AUDIO_VOLUME, &avol,
	0);

    m_iWidth = 0;
    m_iHeight = 0;
    m_fAsync = asynctm / 1000.0;
    m_fSubAsync = subasync / 1000.0;
    while (!m_pAudiostream && def_audio >= 0)
	m_pAudiostream = m_pClip->GetStream(def_audio--, IReadStream::Audio);
    // Initialize audio - thread from SDL is initialized here
    if (m_pAudiostream == 0)
	AVM_WRITE("aviplay", "Audiostream not detected\n");

    if (!m_bVideoMute && m_iDepth)
    {
        // >128 will try to 'touch' all streams
        int o = def_video;
	while ((!m_pVideostream || o > 128) && def_video >= 0)
	    m_pVideostream = m_pClip->GetStream(def_video--, IReadStream::Video);
    }
    else
    {
        m_pVideostream = 0;
	AVM_WRITE("aviplay", "Video disabled\n");
    }
    // check if it's still valid
    // might not be in the case of network stream
    m_pClip->IsValid();

    Get(VIDEO_QUALITY_AUTO, &m_bQualityAuto,
	VIDEO_DROPPING, &m_bVideoDropping,
	VIDEO_BUFFERED, &m_bVideoBuffered,
	VIDEO_DIRECT, &m_bVideoDirect,
	0);

    if (!m_pVideostream)
	AVM_WRITE("aviplay", "Videostream not detected\n");
    else
    {
	//printf("VSTREAMTIME %f\n", m_pVideostream->GetTime());
	// reading subtitles
	avm::string fn = m_Filename;
	char* p = strrchr(fn.c_str(), '.');
	if (p)
            *p = 0;
	InitSubtitles(m_Subfilename.size()
		      ? m_Subfilename.c_str() : fn.c_str());

	if (restartVideoStreaming(m_vcodec.size() ? m_vcodec.c_str() : 0) == 0)
	    createVideoRenderer();
    }

    m_iFramesVideo = 0;
    m_iFrameDrop = 0;
    m_lTimeStart = 0;
    m_bQuit = false;
    m_bConstructed = true;
    //	short fs;
    //	__asm__ __volatile__ ("movw %%fs, %%ax":"=a"(fs));
    //	cout<<"Before pthread_create: fs is "<<fs<<endl;
    //	fs_seg=fs;

    m_bHangup = true; // newly created thread should hangup and wait
    if (m_pVideostream)
    {
	m_pDecoderThread = new PthreadTask(0, startDecoderThread, (void*)this);
	m_pVideoThread = new PthreadTask(0, startVideoThread, (void*)this);
    }
    else if (!m_pAudiostream)
    {
	AVM_WRITE("aviplay", "Cannot play this\n");
	delete m_pClip;
	m_pClip = 0;
	return;
    }

    if (m_pAudiostream)
	m_pAudioThread = new PthreadTask(0, startAudioThread, (void*)this);

    // now wait until all new threads hang in the cond_wait
    lockThreads("Init");
}

int AviPlayer::SetColorSpace(fourcc_t csp, bool test_only)
{
    if (!m_pVideostream || !m_pVideostream->GetVideoDecoder())
	return -1;

    //cout << "SET COLOR SPACE " << hex << csp << dec << "  " << test_only << endl;
    if (!test_only)
	return m_pVideostream->GetVideoDecoder()->SetDestFmt(0, csp);

    IVideoDecoder::CAPS cap;
    switch (csp)
    {
    case fccYUY2:
	cap = IVideoDecoder::CAP_YUY2;
        break;
    case fccI420:
	cap = IVideoDecoder::CAP_I420;
	break;
    case fccYV12:
	cap = IVideoDecoder::CAP_YV12;
        break;
    case fccIYUV:
	cap = IVideoDecoder::CAP_IYUV;
        break;
    case fccUYVY:
	cap = IVideoDecoder::CAP_UYVY;
        break;
    case fccYVYU:
	cap = IVideoDecoder::CAP_YVYU;
	break;
    default:
	cap = IVideoDecoder::CAP_NONE;
    }

    return !(m_pVideostream->GetVideoDecoder()->GetCapabilities() & cap);
}

int AviPlayer::InitSubtitles(const char* filename)
{
    if (lockThreads("InitSubtitles") == 0)
    {
	if (m_pSubtitles)
	    subtitle_close(m_pSubtitles);
	char* fn = NULL;
	int fd = subtitle_filename(filename, &fn);
	if (fd >= 0)
	{
            char* cp;
	    Get(SUBTITLE_CODEPAGE, &cp, NULL);
	    m_pSubtitles = subtitle_open(fd, GetFps(), cp);
	    if (fn)
	    {
		AVM_WRITE("aviplay", "Subtitles from: %s  (codepage: %s)\n", fn, cp);
		free(fn);
	    }
	    if (cp)
                free(cp);
	}
        unlockThreads();
    }
    return 0;
}

const char* AviPlayer::GetAudioFormat() const
{
    return (m_pAudioRenderer) ? m_pAudioRenderer->GetAudioFormat() : 0;
}

const char* AviPlayer::GetVideoFormat() const
{
    return (m_pVideostream) ? GetCodecInfo().GetName() : 0;
}

StreamInfo* AviPlayer::GetAudioStreamInfo() const
{
    return (m_pAudiostream) ? m_pAudiostream->GetStreamInfo() : 0;
}

StreamInfo* AviPlayer::GetVideoStreamInfo() const
{
    return (m_pVideostream) ? m_pVideostream->GetStreamInfo() : 0;
}

const CodecInfo& AviPlayer::GetCodecInfo(int type) const
{
    if (type == AUDIO_CODECS)
	return m_pAudiostream->GetAudioDecoder()->GetCodecInfo();

    return m_pVideostream->GetVideoDecoder()->GetCodecInfo();
}

const subtitle_line_t* AviPlayer::GetCurrentSubtitles()
{
    if (!m_pSubtitles)
	return 0;
    if (m_pSubline == 0)
    {
	m_pSubline = subtitle_line_new();
	if (m_pSubline == 0)
            return 0;
    }

    if (subtitle_get(m_pSubline, m_pSubtitles, GetTime() + m_fSubAsync) != 0)
	return 0;

    return m_pSubline;
}

bool AviPlayer::HasSubtitles() const
{
    return (m_pSubtitles && subtitle_get_lines(m_pSubtitles) > 0);
}

void AviPlayer::Start()
{
    AVM_WRITE("aviplay", 1, "AviPlayer::Start()\n");
    if (!IsValid() || IsRedirector())
	return;

    //printf("CONSTRUCT  %d  %d  i: %d  p: %d\n", IsValid(), IsRedirector(), m_bInitialized, m_bPaused);
    if (!m_bConstructed)
	construct();

    if (IsPlaying())
    {
	AVM_WRITE("aviplay", "AviPlayer::Start(), already started\n");
	return;
    }

    createAudioRenderer();

    if (m_pAudioRenderer)
	m_pAudioRenderer->Start();
    else if (!m_pVideostream)
	return; // nothing to do

    m_lLastTimeStart = longcount();
    m_Drop.clear();
    m_bQuit = false;
    m_bPaused = false;
    m_bBuffering = false;
    unlockThreads();
}

void AviPlayer::Stop()
{
    AVM_WRITE("aviplay", 1, "AviPlayer::Stop()\n");
    if (!IsPlaying())
	return;

    if (lockThreads("Stop") == 0)
    {
	delete m_pAudioRenderer;
	m_pAudioRenderer = 0;

	m_bPaused = false;

	if (m_pKillhandler)
	    m_pKillhandler(0, m_pKillhandlerArg);

	if (m_pVideostream)
	    m_pVideostream->SeekTime(0);//0x7fffffff);

	if (m_pAudiostream)
	    m_pAudiostream->SeekTime(0);//x7fffffff);
    }
}

void AviPlayer::Pause(bool state)
{
    AVM_WRITE("aviplay", 1, "AviPlayer::pause() ( %d -> %d )\n", m_bPaused, state);
    if (!IsPlaying())
        return;

    if (m_bPaused == state)
	return;

    if (state)
    {
	lockThreads("Pause");
	if (m_pAudioRenderer)
	    m_pAudioRenderer->Pause(state);
	m_bPaused = state;
        m_bBuffering = true;
    }
    else
    {
	if (m_pAudioRenderer && (m_pAudioRenderer->Pause(state) != 0))
	    return;
	m_bPaused = state;
	m_bBuffering = false;
	unlockThreads();
    }
}

void AviPlayer::Play()
{
    AVM_WRITE("aviplay", 1, "AviPlayer::Play()  ( %d )\n", m_bPaused);
    if (!IsPlaying())
        return;

    if (m_bPaused)
	Pause(!m_bPaused);
}

double AviPlayer::GetTime() const
{
    if (m_pVideostream && !m_pVideostream->Eof())
    {
	// this is bit more complicate so here goes light explanation:
	// Asf file could display same frame for 5 second - and
	// it doesn't look good when we show jumping numbers
	// so we pick the last remebered frame time and we are
	// calculating time from this frame - we should be preserveing
	// this speed - for paused mode we display time for currently
        // displayed frame
	double len = m_pVideostream->GetLengthTime();
	if (len > 0.)
	{
	    double t = m_pVideostream->GetTime();
	    if (m_pAudioRenderer && !m_pAudioRenderer->Eof())
	    {
		double at = m_pAudioRenderer->GetTime();
		if ((t - at) > 5.)
		    t = at;
	    }
	    return t;
	}
    }
    return (m_pAudioRenderer) ? m_pAudioRenderer->GetTime() : 0.;
}

double AviPlayer::GetLengthTime() const
{
    double len = GetVideoLengthTime();
    double alen = GetAudioLengthTime();
    return (alen > len) ? alen : len;
}

framepos_t AviPlayer::GetFramePos() const
{
    return (m_pVideostream) ? m_pVideostream->GetPos() : 0;
}

double AviPlayer::Reseek(double pos)
{
    AVM_WRITE("aviplay", 1, "Seek pos: %f  %d\n", pos, m_VideoRenderers.size());

    if (!IsPlaying())
	return -1;

    if (lockThreads("Reseek") == 0)
    {
	if (m_pVideostream)
	{
	    pos = m_pVideostream->SeekTimeToKeyFrame(pos);
	    AVM_WRITE("aviplay", 1, "Keyframe pos: %f\n", pos);
	    if (m_bPaused)
		drawFrame(false);
	}
	if (m_pAudioRenderer)
	    m_pAudioRenderer->SeekTime(pos);

	m_Drop.clear();
	unlockThreads();
    }

    return pos;
}

int AviPlayer::ReseekExact(double pos)
{
    if (!IsPlaying())
	return -1;

    // we might not need this in future
    int r = 0;
    if (lockThreads("ReseekExact") == 0)
    {
	AVM_WRITE("aviplay", 1, "Reseek pos: %f  %p %d\n", pos, m_pVideoThread, m_VideoRenderers.size());

	double pos2 = pos;

	if (m_pVideostream)
	{
	    double cft = m_pVideostream->GetTime();
	    double nkft = m_pVideostream->GetTime(m_pVideostream->GetNextKeyFrame());

	    if (cft < pos && (pos < nkft || !nkft))
	    {
		// do nothing
		unlockThreads();
		return 0;
	    }

	    pos2 = m_pVideostream->SeekTimeToKeyFrame(pos);
	    // in case the video is delayed and doesn't start at 0.0s
	    if (pos < (m_pVideostream->GetTime(0) - 0.001) || pos > m_pVideostream->GetLengthTime())
		pos2 = pos;
	}

	AVM_WRITE("aviplay", 1, "Seek OK ( %fs -> %fs )\n", pos, pos2);
	if (pos2 < 0.0)
	{
	    pos2 = pos = 0.0;
	    AVM_WRITE("aviplay", "Warning: reseek_exact  pos2<0!\n");
	    r = -1;
	}

	if (m_pVideostream)
	{
	    if (pos2 > pos && pos2 > (m_pVideostream->GetTime(0) + 0.001))
	    {
		AVM_WRITE("aviplay", "Warning: reseek_exact: pos2>pos! %f %f   %f\n", pos2, pos, m_pVideostream->GetTime(0));
		r = -1;
	    }
	    drawFrame(false);
	}
	if (m_pAudioRenderer)
	    m_pAudioRenderer->SeekTime(pos2); // might be different from vpos

	m_Drop.clear();
	unlockThreads();
    }
    return r;
}

int AviPlayer::NextKeyFrame()
{
    if (!IsPlaying())
	return -1;

    int r = 0;
    if (lockThreads("NextKeyFrame") == 0)
    {
	if (m_pVideostream)
	{
	    m_pVideostream->SeekToNextKeyFrame();
	    drawFrame();
	}
	else if (m_pAudioRenderer)
	    m_pAudioRenderer->SeekTime(m_pAudioRenderer->GetTime() + 1.0);

	unlockThreads();
    }

    return r;
}

int AviPlayer::PrevKeyFrame()
{
    if (!IsPlaying())
	return -1;

    int r = 0;
    if (lockThreads("PrevKeyFrame") == 0)
    {
	if (m_pVideostream && m_pVideostream->GetPos())
	{
	    framepos_t cpos = m_pVideostream->GetPos();
	    framepos_t npos = m_pVideostream->SeekToPrevKeyFrame();
	    if (to_float(longcount(), m_lPgPrevTime) < 0.3)
	    {
		framepos_t p = npos;
		while (npos > 0 && npos >= m_uiPgPrevPos)
		{
		    npos = m_pVideostream->SeekToPrevKeyFrame();
		    if (p == npos)
			break;
		}
	    }

	    if ((cpos - npos < 5) && !IsPaused())
	    {
		m_pVideostream->Seek(npos);
		npos = m_pVideostream->SeekToPrevKeyFrame();
	    }
	    m_lPgPrevTime = longcount();
	    m_uiPgPrevPos = npos;

	    if (npos != m_pVideostream->ERR)
	    {
		//m_pVideostream->SeekToKeyFrame(npos);
		double pos = m_pVideostream->GetTime();
		if (m_pAudioRenderer)
		    m_pAudioRenderer->SeekTime(pos > 0.0 ? pos : 0.0);
	    }
	    drawFrame();
	}
	else if (m_pAudioRenderer)
	    m_pAudioRenderer->SeekTime(m_pAudioRenderer->GetTime() - 1.);

	unlockThreads();
    }

    return r;
}

int AviPlayer::NextFrame()
{
    if (!IsPlaying())
        return -1;
    if (lockThreads("NextFrame") == 0)
    {
	drawFrame();
	unlockThreads();
    }

    return 0;
}

int AviPlayer::PrevFrame()
{
    if (!IsPlaying())
	return -1;

    if (lockThreads("PrevFrame") == 0)
    {
	if (m_pVideostream)
	{
	    framepos_t cpos = m_pVideostream->GetPos();
	    framepos_t prev = cpos;
	    m_pVideostream->SeekToPrevKeyFrame();
	    m_fDecodingTime = 1.0;
	    setQuality();
	    if ((m_pVideostream->GetPos() + 2) < cpos)
	    {
		//printf("GGP %d  %d\n",m_pVideostream->GetPos(), cpos);
		for (;;)
		{
		    m_pVideostream->ReadFrame(false);
		    //printf("npos %d\n", m_pVideostream->GetPos());
		    if ((m_pVideostream->GetPos() + 2) >= cpos)
			break;
		}
		if (m_pVideostream->GetPos() > cpos)
		{
		    // there were some skiped frame - so jump to keyframe
		    m_pVideostream->SeekToPrevKeyFrame();
		}
	    }

	    //printf("RENDER %d  %f  %d\n", m_pVideostream->GetPos(), m_pVideostream->GetTime(), cpos);
	    if (m_pVideostream->GetPos() < cpos || cpos == 0)
		drawFrame();
	}
	unlockThreads();
    }

    return 0;
}

IRtConfig* AviPlayer::GetRtConfig(int type) const
{
    // maybe add lock ???
    switch (type)
    {
    case AUDIO_CODECS:
	{
	    IAudioDecoder* d = (m_pAudiostream) ? m_pAudiostream->GetAudioDecoder() : 0;
	    // here is INCREDIBLE BUG in g++ 3.0.4
	    //IRtConfig* c = dynamic_cast<IRtConfig*>(d)
            // so whole avifile now doesn't use dynamic_cast operator
	    return (d) ? d->GetRtConfig() : 0;
	}
    case AUDIO_RENDERER:
	{
	    return 0;
	}
    case VIDEO_CODECS:
	{
	    IVideoDecoder* d = (m_pVideostream) ? m_pVideostream->GetVideoDecoder() : 0;
	    return (d) ? d->GetRtConfig() : 0;
	}
    case VIDEO_RENDERER:
	{
	    if (m_VideoRenderers.size() > 0)
		return m_VideoRenderers[0]->GetRtConfig();
	    return 0;
	}
    }
    return 0;
}

// used when we are in pause mode or the player is not playing
int AviPlayer::drawFrame(bool aseek)
{
    if (!m_bQuit && m_pVideostream)
    {
	CImage* im = m_pVideostream->GetFrame(true); // ReadFrame
	m_fLastDiff = 0.0;
	setQuality();
	if (im)
	{
	    const subtitle_line_t* sl = GetCurrentSubtitles();
	    for (unsigned i = 0; i < m_VideoRenderers.size(); i++)
	    {
		m_VideoRenderers[i]->Draw(im);
		if (HasSubtitles())
		    m_VideoRenderers[i]->DrawSubtitles(sl);

		m_VideoRenderers[i]->Sync();
	    }
	    m_Quality.insert(im->GetQuality() * 100.0);
	    im->Release();
	    m_iFramesVideo++;
	}
	if (aseek && m_pAudioRenderer)
	    m_pAudioRenderer->SeekTime(m_pVideostream->GetTime());
	return 0;
    }
    return -1;
}

void AviPlayer::createAudioRenderer()
{
    double origtime = -1.0;
    if (m_pAudioRenderer)
        origtime = m_pAudioRenderer->GetTime();
    delete m_pAudioRenderer;
    m_pAudioRenderer = 0;
    if (!m_pAudiostream)
        return;

    WAVEFORMATEX MyOwf;
    memset(&MyOwf, 0, sizeof(MyOwf));
    uint_t forcedFreq = 0;

    bool resamp;
    Get(AUDIO_RESAMPLING, &resamp, 0);
    if (resamp)
    {
	Get(AUDIO_RESAMPLING_RATE, &MyOwf.nSamplesPerSec,
	    AUDIO_PLAYING_RATE, &forcedFreq,
	    0);
    }

    char* arend;
    Get(AUDIO_RENDERER, &arend, 0);
    if (!arend)
	return;
    m_pAudiostream->StopStreaming();
    if (m_pAudiostream->StartStreaming(m_acodec.size() ? m_acodec.c_str() : 0))
	return;

    char* arends = arend;
    while (arends && !m_pAudioRenderer)
    {
	WAVEFORMATEX Owf = MyOwf;
	AVM_WRITE("aviplay", "Will try audio renderers in this order: %s\n", arends);

	if (m_pAudiofunc)
	{
	    // we could run this code sooner - but why...
	    m_pAudioRenderer = new CopyAudioRenderer(m_pAudiostream, Owf, m_pAudiofunc, m_pAudiofuncArg);
	}
	else if (!strncasecmp(arends, "SDL", 3))
	{
#ifdef HAVE_LIBSDL
	    m_pAudioRenderer = new SdlAudioRenderer(m_pAudiostream, Owf, forcedFreq);
#else
	    AVM_WRITE("aviplay", "Warning: SDL audio renderer unavailable!\n");
#endif
	}
	else if (!strncasecmp(arends, "OSS", 3))
	{
#ifdef HAVE_OSS
	    m_pAudioRenderer = new OssAudioRenderer(m_pAudiostream, Owf, forcedFreq);
#else
	    AVM_WRITE("aviplay", "Warning: OSS audio renderer unavailable!\n");
#endif
	}
	else if (!strncasecmp(arends, "SUN", 3))
	{
#ifdef HAVE_SUNAUDIO
	    AVM_WRITE("aviplay", "Warning: Sun audio renderer out of date - update me!\n");
	    m_pAudioRenderer = new SunAudioRenderer(m_pAudiostream, Owf);
#else
	    AVM_WRITE("aviplay", "Warning: Sun audio renderer unavailable!\n");
#endif
	}
	else if (!strncasecmp(arends, "noaudio", 5))
	{
	    if (strlen(arend) < 6)
	    {
		AVM_WRITE("aviplay", "--- 'noaudio' audio renderer selected - if this is unintentional\n");
		AVM_WRITE("aviplay", "--- please remove ~/.avm directory and default renderer set will be used\n");
	    }
	    break;
	}
	if (m_pAudioRenderer && m_pAudioRenderer->Init() < 0)
	{
            delete m_pAudioRenderer;
	    m_pAudioRenderer = 0;
	}
	arends = strchr(arends, ',');
	if (arends)
            arends++;
    }
    if (m_pAudioRenderer)
    {
	m_pAudioRenderer->SetAsync(m_fAsync);
//	m_pAudioRenderer->SetVolume(m_fVolume * m_pAudioRenderer->VOL_MAX);
	if (origtime > 0)
	{
	    m_pAudioRenderer->SeekTime(origtime);
	    m_pAudioRenderer->Start();
	}
    }
    free(arend);
}

AviPlayer::State AviPlayer::GetState(double* percent)
{
    if (!m_pClip || !m_pClip->IsValid())
    {
	if (m_pClip && !m_pClip->IsOpened())
	    return Opening;
	return Invalid;
    }

    double p;
    if (m_pVideostream)
	p = m_pVideostream->CacheSize();
    else if (m_pAudioRenderer)
	p = m_pAudioRenderer->GetCacheSize();
    else
        p = 0;
    if (percent)
	*percent = p;

    if (m_bBuffering)
    {
	if (p >= 1.0)
            m_bBuffering = false;
	return Buffering;
    }
    if (IsPaused())
	return Paused;
    if (IsPlaying())
	return Playing;

    return Stopped;
}

bool AviPlayer::IsPaused() const
{
    return (m_pClip && m_bPaused);
}

bool AviPlayer::IsPlaying() const
{
    // when we are in paused mode  initialized is 0
    return m_pClip && !IsRedirector() && (m_bInitialized || m_bPaused);
}

bool AviPlayer::IsStopped() const
{
    return (m_pClip && !IsPlaying());
}

const avm::vector<IVideoRenderer*>& AviPlayer::GetVideoRenderers() const
{
    return m_VideoRenderers;
}

int AviPlayer::SetVideoRenderers(avm::vector<IVideoRenderer*> rv)
{
    if (lockThreads("SetVideoRenderes") == 0)
    {
	m_VideoRenderers = rv;
	unlockThreads();
    }
    return 0;
}

void AviPlayer::SetAsync(float async)
{
    Set(ASYNC_TIME_MS, int(async * 1000), 0);
}

void AviPlayer::SetAudioFunc(AUDIOFUNC func, void* arg)
{
    if (lockThreads("SetAudioFunc") == 0)
    {
	m_pAudiofunc = func;
	m_pAudiofuncArg = arg;
	createAudioRenderer();
	unlockThreads();
    }
}

void AviPlayer::Restart()
{
    if (!IsPlaying() || !m_pVideostream)
	return;

    IVideoDecoder* vs = m_pVideostream->GetVideoDecoder();
    if (!vs)
	return;

    if (m_bPaused)
    {
	vs->Restart();
	return;
    }

    lockThreads("Restart");
    vs->Restart();
    unlockThreads();
    ReseekExact(GetTime());
}

int AviPlayer::restartVideoStreaming(const char* vc)
{
    int r = -1;
    if (m_pVideostream)
    {
        m_pVideostream->StopStreaming();

	r = m_pVideostream->StartStreaming(vc);
	if (r == 0)
	{
	    if (!GetRtConfig(VIDEO_CODECS)
		|| !GetCodecInfo().FindAttribute("maxauto")
		|| CodecGetAttr(GetCodecInfo(), "maxauto", &m_iMaxAuto) < 0)
		m_iMaxAuto = -1;

	    // prefer this format
	    // remove 0 to get YUY2 rendering
	    StreamInfo* si = m_pVideostream->GetStreamInfo();
	    if (si)
	    {
		m_iWidth = si->GetVideoWidth();
		m_iHeight = si->GetVideoHeight();
		delete si;
	    }
	    // FIXME - might be sometimes better to use different
	    // colorspace
	    fourcc_t m[] = { m_CSP, IMG_FMT_YUY2, IMG_FMT_YV12, 0 };
	    for (int i = 0; m[i]; i++)
	    {
		if (m[i] > 32 && m_pVideostream->GetVideoDecoder()->SetDestFmt(m_CSP) == 0)
		{
		    //printf("OK %x\n", m[i]);
		    m_CSP = m[i];
		    break;
		}
	    }
	}
	else
	{
	    AVM_WRITE("aviplay", "Failed to initialize decoder\n");
	    m_pVideostream = 0;
	}
    }
    return r;
}

void* AviPlayer::startVideoThread(void* arg)
{
    return ((AviPlayer*) arg)->videoThread();
}

void* AviPlayer::startAudioThread(void* arg)
{
    return ((AviPlayer*) arg)->audioThread();
}

void* AviPlayer::startDecoderThread(void* arg)
{
    return ((AviPlayer*) arg)->decoderThread();
}

IAviPlayer* CreateAviPlayer(const char* filename, int bitdepth, const char* subname,
                            unsigned long flags,
			    const char* vcodec, const char* acodec)
{
    return new AviPlayer(filename, bitdepth, subname, flags,
			 vcodec, acodec);
}

#undef __MODULE__

AVM_END_NAMESPACE;
