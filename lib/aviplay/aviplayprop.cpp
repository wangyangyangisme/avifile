#include "avm_args.h"
#include "avm_except.h"
#include "avm_creators.h" // codec sorting
#include "aviplay_impl.h"
#include "configfile.h"
#include "avm_output.h"

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>


AVM_BEGIN_NAMESPACE;

static const char* regName = "aviplay";
static bool DEFAULT_FALSE = false;
static bool DEFAULT_TRUE = true;
static int DEFAULT_0 = 0;
static int DEFAULT_1 = 1;
static int DEFAULT_RATE = 44100;
static int DEFAULT_VOLUME = 1000;
static int DEFAULT_BALANCE = 500;

// first in the list is default
static const char* subtitleRendererTxt =
#ifdef HAVE_LIBXFT
    "XFT_DRAW,"
#endif
#ifndef X_DISPLAY_MISSING
    "X11_DRAW,"
#endif
    "nosubtitles";

static const char* audioRendererTxt =
#ifdef HAVE_OSS
    "OSS,"
#endif
#ifdef HAVE_LIBSDL
    "SDL,"
#endif
#ifdef HAVE_SUNAUDIO
    "SUN,"
#endif
    "noaudio";

static const char* videoRendererTxt =
#ifdef HAVE_LIBSDL
    "SDL_XV,SDL_X11,"
#endif
    "novideo";

const Args::Option IAviPlayer::options[] =
{
    // FIXME convert rest of flags later
    // text, type, defvalue, minval, maxval
    { Args::Option::INT, 0, 0 },
    { Args::Option::REGSTRING, 0, "codecs-audio", "order of audio codecs", (void*)"" },
    { Args::Option::REGSTRING, 0, "codecs-video", "order of video codecs", (void*)"" },
    { Args::Option::REGSTRING, 0, "audio-renderer", "audio renderer", (void*)audioRendererTxt },
    { Args::Option::REGSTRING, 0, "video-renderer",  "video renderer", (void*)videoRendererTxt },
    { Args::Option::REGSTRING, 0, "subtitle-renderer", "subtitle renderer", (void*)subtitleRendererTxt },
    { Args::Option::REGBOOL, 0, "use-yuv",  "use YUV hw acceleration", &DEFAULT_TRUE },
    { Args::Option::REGBOOL, 0, "autorepeat",  "repeat movie after end of the movie", &DEFAULT_TRUE },
    { Args::Option::REGINT, 0, "audio-stream", "select audio channel", &DEFAULT_0, -1, 255 },
    { Args::Option::REGINT, 0, "video-stream", "select video channel", &DEFAULT_0, -1, 255 },

    { Args::Option::REGSTRING, "subfont", "subtitle-font", "select font for subtitles",  (void*)"-adobe-helvetica-medium-r-normal--*-160-100-100-p-*" },
    { Args::Option::REGINT, 0, "subtitle-async-ms", "async time for subtitle (ms)", &DEFAULT_0 },
    { Args::Option::REGINT, 0, "subtitle-extend-ms", "extended time for subtitle (ms)", &DEFAULT_0 },
    { Args::Option::REGSTRING, "subcp", "subtitle-codepage", "encoding code page of subtitles (iconv --list)", (void*)"default" },
    { Args::Option::REGBOOL, 0, "subtitle-enabled", "enable subtitles", &DEFAULT_TRUE },
    { Args::Option::REGBOOL, 0, "subtitle-wrap", "subtitles wrap to next line", &DEFAULT_FALSE },
    { Args::Option::REGINT, 0, "subtitle-bgcolor", "subtitles background color", &DEFAULT_0 },
    { Args::Option::REGINT, 0, "subtitle-fgcolor", "subtitles foreground color", &DEFAULT_0 },
    { Args::Option::REGINT, 0, "subtitle-hposition", "subtitles horizontal position", &DEFAULT_0 },

    { Args::Option::REGBOOL, 0, "use-http-proxy",  "use http proxy for connection", &DEFAULT_FALSE },
    { Args::Option::REGSTRING, 0, "http-proxy", "address of http proxy", (void*)"" },
    { Args::Option::REGBOOL, 0, "audio-resampling", "resample audio", &DEFAULT_FALSE },
    { Args::Option::REGINT, 0, "audio-resampling-rate", "target resampling rate",  &DEFAULT_RATE, 1, 100000 },
    { Args::Option::REGINT, 0, "audio-playing-rate", "audio playing rate", &DEFAULT_RATE, 1, 100000 },
    { Args::Option::REGBOOL, 0, "audio-master-timer", "", &DEFAULT_FALSE },
    { Args::Option::REGINT, 0, "audio-volume", "audio volume <%d, %d>", &DEFAULT_VOLUME, 0, 10000 }, // 1000 is 1.0
    { Args::Option::REGINT, 0, "audio-balance", "audio balance <%d, %d>", &DEFAULT_BALANCE, 0, 1000 }, // 500 is middle
    { Args::Option::REGSTRING, 0, "url-audio", "url for audio stream", (void*)"" },

    { Args::Option::REGBOOL, 0, "preserve-aspect", "preserve aspect during resize", &DEFAULT_TRUE },
    { Args::Option::REGBOOL, 0, "buffered", "buffer video ahead", &DEFAULT_TRUE },
    { Args::Option::REGBOOL, 0, "direct", "decompress directoly to image memory", &DEFAULT_TRUE },
    { Args::Option::REGBOOL, 0, "dropping", "drop frame when a/v desync", &DEFAULT_TRUE },
    { Args::Option::REGBOOL, 0, "quality_auto", "change postprocessing automaticaly", &DEFAULT_TRUE },

    { Args::Option::REGBOOL, 0, "display-frame-pos", "show more informative about position", &DEFAULT_FALSE },
    { Args::Option::REGINT, 0, "av-async-ms", "audio/video async time (ms)", &DEFAULT_0 },
    { Args::Option::NONE },
};

#define LOCK_THREADS() \
    if (!locked)\
    {\
    lockThreads("SetProperties");\
    locked = true;\
    }

int AviPlayer::Set(...)
{
    va_list args;
    Property prop;
    avm::string strtmp;
    bool setvidbuf = false;
    bool locked = false;
    bool renderer = false;
    bool useyuv;
    va_start(args, this);
    while ((prop = (Property) va_arg(args, int)) != 0)
    {
	avm::vector<const CodecInfo*> vci;
	int int_param = 0;
	const char* string_param = 0;

	if (prop < LAST_PROPERTY)
	{
	    switch (options[prop].type)
	    {
	    case Args::Option::REGINT:
	    case Args::Option::REGBOOL:
		int_param = va_arg(args, int);
		break;
	    case Args::Option::REGSTRING:
		string_param = va_arg(args, char*);
		break;
	    default:
                ;
	    }

	    //printf("PropertySet %d  %s\n", prop, options[prop].olong);

	    switch (prop)
	    {
	    case AUDIO_RENDERER:
		break;
	    case VIDEO_RENDERER:
		break;
	    case VIDEO_CODECS:
		LOCK_THREADS();
		SortVideoCodecs(string_param);

		strtmp.erase();
		CodecInfo::Get(vci);
		for (uint_t i = 0; i < vci.size(); i++)
		{
		    strtmp += vci[i]->GetPrivateName();
		    if (i + 1 < vci.size())
			strtmp += ',';
		}
		string_param = strtmp.c_str();

		if (0)
		{
		    char* origval = 0;
		    Get(ASYNC_TIME_MS, &origval, NULL);
		    printf("ORIG %s\n NEW  %s\n R: %d\n", string_param, origval, strcmp(string_param, origval));
		}
		if (m_pVideostream && m_pVideostream->IsStreaming())
		{

		    //printf("POS1  %f\n", m_pVideostream->GetTime());
                    restartVideoStreaming();
		    if (m_pAudioRenderer)
		    {
			m_pVideostream->SeekTime(m_pAudioRenderer->GetTime());
			framepos_t p = m_pVideostream->GetPos();
			if ((m_pVideostream->GetNextKeyFrame() - p) < 40)
			    m_pVideostream->SeekToNextKeyFrame();
			else
			    m_pVideostream->SeekToPrevKeyFrame();
		    }
		    //printf("POS5  %f\n", m_pVideostream->GetTime());
		}
		break;
	    case AUDIO_CODECS:
		LOCK_THREADS();
		SortAudioCodecs(string_param);
		strtmp.erase();
		CodecInfo::Get(vci, CodecInfo::Audio);
		for (unsigned i = 0; i < vci.size(); i++)
		{
		    strtmp += vci[i]->GetPrivateName();
		    if (i + 1 < vci.size())
			strtmp += ',';
		}
		string_param = strtmp.c_str();
		createAudioRenderer();
		break;
	    case USE_YUV:
		LOCK_THREADS();
		Get(USE_YUV, &useyuv, 0);
                if (int_param != useyuv)
		    renderer = true;
		break;
	    case VIDEO_QUALITY_AUTO:
		LOCK_THREADS();
		if (m_bQualityAuto != int_param)
		{
		    setvidbuf = true;
		    m_bQualityAuto = int_param;
		}
		break;
	    case VIDEO_BUFFERED:
		LOCK_THREADS();
		if (m_bVideoBuffered != int_param)
		{
		    setvidbuf = true;
		    m_bVideoBuffered = int_param;
		}
		break;
	    case VIDEO_DIRECT:
		LOCK_THREADS();
		if (m_bVideoDirect != int_param)
		{
		    setvidbuf = true;
		    m_bVideoDirect = int_param;
		}
		break;
	    case VIDEO_DROPPING:
		m_bVideoDropping = int_param;
		break;
	    case ASYNC_TIME_MS:
		m_fAsync = int_param / 1000.0;
		if (m_pAudioRenderer)
		    m_pAudioRenderer->SetAsync(m_fAsync);
		break;
	    case SUBTITLE_FONT:
		LOCK_THREADS();
		setFont(string_param);
		break;
	    case SUBTITLE_ASYNC_TIME_MS:
		m_fSubAsync = int_param / 1000.0;
		break;
	    case AUDIO_VOLUME:
		if (m_pAudioRenderer)
		    m_pAudioRenderer->SetVolume(int_param);
		break;
	    case AUDIO_BALANCE:
		if (m_pAudioRenderer)
		    m_pAudioRenderer->SetBalance(int_param);
		break;
	    case AUDIO_URL:
		LOCK_THREADS();
		setAudioUrl(string_param);
		break;
	    case VIDEO_STREAM:
		//delete m_pVideostream;
		//m_pVideostream = m_pClip->GetStream(0, IAviReadStream::Video);
		break;
	    case AUDIO_STREAM:
		LOCK_THREADS();
		setAudioStream(int_param);
		break;
	    default:
		//AVM_WRITE("Property", "Unknown (?unsupported?) propety  %d\n", prop);
		break;
	    }

	    switch (options[prop].type)
	    {
	    case Args::Option::REGINT:
	    case Args::Option::REGBOOL:
		RegWriteInt(regName, options[prop].olong, int_param);
		break;
	    case Args::Option::REGSTRING:
		RegWriteString(regName, options[prop].olong, string_param);
		break;
	    default:
                ;
	    }
	}
    }
    va_end(args);

    if (locked)
    {
	if (setvidbuf)
	    setVideoBuffering();
	if (renderer && m_pVideostream)
	{
	    double pos = GetTime();
	    m_pVideostream->StopStreaming();
	    m_pVideostream->StartStreaming();
	    createVideoRenderer();

	    double posn = m_pVideostream->SeekTimeToKeyFrame(pos);
	    if (m_pAudiostream && posn + 5 < pos)
		Reseek(posn); // complet reseek
	}
	unlockThreads();
    }

    return 0;
}
#undef LOCK_THREADS

int AviPlayer::Get(...) const
{
    va_list args;
    Property prop;

    //printf("Sizeof %d\n", sizeof(Property));
    va_start(args, this);
    while ((prop = (Property) va_arg(args, int)) != 0)
    {
	bool ok = true;
	bool isBool = false;
	avm::string string_param = "_empty_";
	int int_param = 0;

	if (prop < LAST_PROPERTY)
	{
	    //printf("PropertyGet %d  %s  (readcnt: %d)\n", prop, options[prop].olong, propertyRead[prop]);
	    Property force = prop;
	    isBool = (options[prop].type == Args::Option::REGBOOL);

	    if (!propertyRead[prop])
		force = LAST_PROPERTY; // force reading from registry

	    switch (force)
	    {
	    case VIDEO_BUFFERED:
		int_param = m_bVideoBuffered;
		break;
	    case VIDEO_DROPPING:
		int_param = m_bVideoDropping;
		break;
	    case VIDEO_DIRECT:
		int_param = m_bVideoDirect;
		break;
	    case VIDEO_QUALITY_AUTO:
		int_param = m_bQualityAuto;
		break;
	    case ASYNC_TIME_MS:
		int_param = int(m_fAsync * 1000.0);
		break;
	    case SUBTITLE_ASYNC_TIME_MS:
		int_param = int(m_fSubAsync * 1000.0);
		break;
	    case AUDIO_VOLUME:
		int_param = (m_pAudioRenderer) ? m_pAudioRenderer->GetVolume() : 0;
		break;
	    case AUDIO_BALANCE:
		int_param = (m_pAudioRenderer) ? m_pAudioRenderer->GetBalance() : 500;
		break;
	    default:
		//printf("Default propety: %s (idx = %d)\n", propertyList[prop].text, prop);
		switch (options[prop].type)
		{
		case Args::Option::REGBOOL:
		case Args::Option::REGINT:
		    int_param = RegReadInt(regName, options[prop].olong,
					   *(const int*)options[prop].value);
		    break;
		case Args::Option::REGSTRING:
		    string_param = RegReadString(regName, options[prop].olong,
						 (const char*)options[prop].value);
		    break;
		default:
                    ;
		}

		propertyRead[prop]++;
		break;
	    }
	}
	else
	{
	    switch (prop)
	    {
	    case QUERY_AVG_QUALITY:
		int_param = int(m_Quality.average());
		break;
	    case QUERY_AVG_DROP:
		int_param = int(m_Drop.average());
		break;
	    case QUERY_AUDIO_RENDERERS:
		string_param = audioRendererTxt;
		break;
	    case QUERY_VIDEO_RENDERERS:
		string_param = videoRendererTxt;
		break;
	    case QUERY_SUBTITLE_RENDERERS:
		string_param = subtitleRendererTxt;
		break;
	    case QUERY_EOF:
		isBool = true;
		int_param = (m_pVideostream) ? m_pVideostream->Eof() : true;
		//printf("VSTREAM %d\n", int_param);
		//  audiostream doesn't detect eof - have to use Length FIXME
		if (int_param && m_pAudioRenderer)
		    int_param = m_pAudioRenderer->Eof();
		//printf("???EOF %d %d	%f  %f\n", int_param, m_pAudiostream->Eof(),
		//	 m_pAudiostream->GetTime(), m_pAudiostream->GetLengthTime());
		break;
	    case QUERY_VIDEO_WIDTH:
		{
		    int w = m_iWidth, h;
		    if (m_VideoRenderers.size())
			m_VideoRenderers[0]->GetSize(w, h);
		    int_param = w;
		}
		break;
	    case QUERY_VIDEO_HEIGHT:
		{
		    int w, h = m_iHeight;
		    if (m_VideoRenderers.size())
			m_VideoRenderers[0]->GetSize(w, h);
		    int_param = h;
		}
		break;
	    case QUERY_VIDEO_STREAMS:
		int_param = (m_pClip) ? m_pClip->VideoStreamCount() : 0;
		break;
	    case QUERY_AUDIO_STREAMS:
		if (m_pClipAudio)
		    int_param = m_pClipAudio->AudioStreamCount();
		else
		    int_param = (m_pClip) ? m_pClip->AudioStreamCount() : 0;
		break;
	    default:
		AVM_WRITE("aviplay", "Unexpected property value: %d\n", prop);
		break;
	    }
	}

	int* ptr = va_arg(args, int*);
	//printf("Ptr:	%p\n", ptr);
	if (ok && ptr)
	{
	    if (isBool)
		*(bool*)ptr = (bool) int_param;
	    else
	    {
		if (string_param == "_empty_")
		    *ptr = int_param;
		else
		    *(char**)ptr = (char*) strdup(string_param.c_str());
	    }
	}
    }
    va_end(args);

    return 0;
}

void AviPlayer::setVideoBuffering()
{
    lockThreads("SetVideoBuffered");

    if (m_pVideostream)
    {
	IImageAllocator* ia = (m_bVideoDirect && m_VideoRenderers.size()) ? m_VideoRenderers[0] : 0;
	uint_t bufs = (m_bVideoBuffered) ? 6 : 1;

	//printf("Buffering: %d   Allocator: %p\n", bufs, ia);
	m_pVideostream->SetBuffering(bufs, ia);
    }
    unlockThreads();
}

int AviPlayer::setAudioStream(int channel)
{
    IReadStream* astream = 0;
    if (channel > 127)
	channel = 127;
    else if (channel < 0)
	channel = 0;

    IReadFile* qf = m_pClipAudio;
    if (!qf)
	qf = m_pClip;

    while (qf && !astream && channel >= 0)
	astream = qf->GetStream(channel--, IReadStream::Audio);

    if (astream != m_pAudiostream)
    {
	double tpos = 0.0;
	if (m_pAudioRenderer)
	{
	    tpos = m_pAudioRenderer->GetTime();
	    delete m_pAudioRenderer;
	    m_pAudioRenderer = 0;
	}
	else if (m_pVideostream)
	    tpos = m_pVideostream->GetTime();

	if (m_pAudiostream)
	    m_pAudiostream->StopStreaming();
	// NOTE: never call delete on	m_pAudiostream !
	m_pAudiostream = astream;
	createAudioRenderer();
	if (m_pAudioRenderer)
	{
	    m_pAudioRenderer->SeekTime(tpos);
	    m_pAudioRenderer->Start();
	}
    }
    return 0;
}

int AviPlayer::setAudioUrl(const char* url)
{
    lockThreads("SetAudioUrl");

    IReadFile* newClipAudio = 0;

    try
    {
	// compare Url's from the end (as filename could be either
	// fullpath or just filename
	int u = strlen(url);
	int f = m_Filename.size();

	while (u > 0 && f > 0)
	    if (url[--u] != m_Filename[--f])
		break;
	newClipAudio = (u == 0 || f == 0) ? m_pClip : CreateReadFile(url);

	//printf("CLUPO %s %s %p %p\n", m_Filename.c_str(), url, m_pClip, newClipAudio);
    }
    catch (FatalError& e)
    {
	e.Print();
    }

    if (newClipAudio)
    {
	IReadFile* tmp = m_pClipAudio;
	m_pClipAudio = (newClipAudio != m_pClip) ? newClipAudio : 0;
	int def_audio;
	Get(AUDIO_STREAM, &def_audio, 0);

	setAudioStream(def_audio);
	delete tmp;
    }

    unlockThreads();
    return 0;
}

AVM_END_NAMESPACE;
