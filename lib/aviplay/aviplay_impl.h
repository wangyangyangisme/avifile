#ifndef AVIPLAY_IMPL_H
#define AVIPLAY_IMPL_H

#include "IAudioRenderer.h"
#include "Statistic.h"
#include "audiodecoder.h"
#include "aviplay.h"
#include "renderer.h"
#include "videodecoder.h"

/***************************************************

Here aviplay interface goes

****************************************************/

AVM_BEGIN_NAMESPACE;

class AviPlayer: public IAviPlayer2
{
public:
    AviPlayer(const char* filename, int _depth, const char*subname,
              unsigned int flags = 0,
	      const char* vcodec = 0, const char* acodec = 0);
    ~AviPlayer();

    int SetColorSpace(fourcc_t csp, bool test_only);

    virtual int GetWidth() const {return m_iWidth;}
    virtual int GetHeight() const {return m_iHeight;}
    virtual void SetKillHandler(KILLHANDLER handler, void* arg = 0)
    {
	m_pKillhandler = handler;
	m_pKillhandlerArg = arg;
    }
    virtual void SetAudioFunc(AUDIOFUNC func, void* arg = 0);
    virtual double GetTime() const;
    virtual framepos_t GetFramePos() const;

    virtual bool GetURLs(avm::vector<avm::string>& urls) const { return (m_pClip) ? m_pClip->GetURLs(urls) : false; }
    virtual bool IsOpened() const { return (m_pClip) ? m_pClip->IsOpened() : false; }
    virtual bool IsValid() const { return (m_pClip) ? m_pClip->IsValid() : false; }
    virtual bool IsRedirector() const { return (m_pClip) ? m_pClip->IsRedirector() : false; }
    virtual bool IsStopped() const;
    virtual bool IsPlaying() const;
    virtual bool IsPaused() const;
    virtual void Restart();
    virtual double Reseek(double pos);
    virtual int ReseekExact(double pos);
    virtual void Start();
    virtual void Stop();
    virtual int NextKeyFrame();
    virtual int NextFrame();
    virtual int PrevKeyFrame();
    virtual int PrevFrame();
    virtual void Play();
    virtual void Pause(bool state);
    virtual const subtitle_line_t* GetCurrentSubtitles();
    virtual double GetLengthTime() const;
    virtual double GetAudioLengthTime() const
    {
	return (m_pAudioRenderer) ? m_pAudioRenderer->GetLengthTime() : 0.0;
    }
    virtual double GetVideoLengthTime() const
    {
	return (m_pVideostream) ? m_pVideostream->GetLengthTime() : 0.0;
    }
    virtual const char* GetAudioFormat() const;
    virtual StreamInfo* GetAudioStreamInfo() const;

    virtual const char* GetVideoFormat() const;
    virtual StreamInfo* GetVideoStreamInfo() const;
    virtual double GetFps() const
    {
	return (m_pVideostream) ? 1.0 / m_pVideostream->GetFrameTime() : 0.0;
    }
    virtual const char* GetFileName() const { return m_Filename.c_str(); }
    virtual const avm::vector<IVideoRenderer*>& GetVideoRenderers() const;
    virtual int SetVideoRenderers(avm::vector<IVideoRenderer*>);
    virtual State GetState(double* percent);
    virtual bool HasSubtitles() const;
    virtual int InitSubtitles(const char* filename);
    virtual int Set(...);
    virtual int Get(...) const;
    virtual void SetAsync(float async);
    virtual float GetAsync() const { return m_fAsync; }
    virtual const CodecInfo& GetCodecInfo(int type = VIDEO_CODECS) const;
    virtual IRtConfig* GetRtConfig(int type = VIDEO_CODECS) const;

    virtual void SetVideo(bool enabled) { m_bVideoMute = enabled; }
    virtual void SetAudio(bool enabled) { }
    virtual bool GetVideo() const { return m_bVideoMute; }
    virtual bool GetAudio() const { return (m_pAudioRenderer != 0); }

    virtual int Refresh() { return -1; }
    virtual int Resize(int& new_w, int& new_h) { return -1; }
    virtual int Zoom(int x, int y, int w, int h) { return -1; }
    virtual int ToggleFullscreen(bool maximize = false) { return -1; }

protected:
    enum ThreadId
    {   // do not change order
	THREAD_FIRST = 0,
	THREAD_DECODER = THREAD_FIRST, THREAD_AUDIO, THREAD_VIDEO,
	THREAD_LAST,
    };

    static void* constructThread(void* arg);
    void* constructThreadfunc();
    void construct();
    int lockThreads(const char *name = 0);
    void unlockThreads(void);
    int checkSync(ThreadId);
    int drawFrame(bool aseek = true);
    void syncFrame();
    void changePriority(const char* taskName, int add, int schedt = 0);

    float getVideoAsync();
    bool dropFrame();
    void setQuality();

    void* videoThread();
    void* audioThread();
    void* decoderThread();
    // special way to have function  compatible with C code pthread parameter
    // we could use only one parameter and this seems to be simpliest way
    static void* startVideoThread(void* arg);
    static void* startAudioThread(void* arg);
    static void* startDecoderThread(void* arg);

    virtual int setAudioStream(int channel);
    virtual int setAudioUrl(const char* filename);
    virtual int setFont(const char* fn) { return -1; }
    virtual void setVideoBuffering();
    virtual void createVideoRenderer() {}
    void createAudioRenderer();
    int restartVideoStreaming(const char* codec = 0);

    mutable int propertyRead[LAST_PROPERTY]; // have we read at least once from Registry::

    avm::vector<IVideoRenderer*> m_VideoRenderers; // we could draw image to more places
    IAudioRenderer* m_pAudioRenderer; // not sure about the sound - this will be
                                      // more complicated because of buffer sizes
    KILLHANDLER m_pKillhandler;
    void* m_pKillhandlerArg;
    AUDIOFUNC m_pAudiofunc;
    void* m_pAudiofuncArg;
    IReadFile* m_pClip;
    IReadFile* m_pClipAudio;            // second (audio - only stream)
    IReadStream* m_pVideostream;
    IReadStream* m_pAudiostream;
    Statistic m_Drop;
    Statistic m_Quality;
    avm::string m_Filename;
    avm::string m_Subfilename;
    avm::string m_vcodec;
    avm::string m_acodec;
    subtitles_t* m_pSubtitles;	// list of subtitles
    subtitle_line_t* m_pSubline;// contains current subtitle line

    float m_fAsync;		// a-v async time
    float m_fSubAsync;
    int m_iMaxAuto;
    int m_iFramesVideo;		// just counters
    int m_iFrameDrop;
    int m_iLockCount;

    int m_iWidth;
    int m_iHeight;
				// these vars are used in syncing video with time
    int64_t m_lTimeStart;	// timestamp of moment when we started playing
				// updated after each reseek or pause
    int64_t m_lLastVideoSync;	// stamp for last video time in sync
    int64_t m_lLastTimeStart;	// timestamp of the last displayed frame
    double m_dFrameStart;	// precise position of video at time_start ( needed
				// for timing of video-only streams )
    double m_dLastFrameStart;	// precise position of video at last frame
    double m_dLastAudioSync;	// stamp for last audio time in sync
    float m_fLastDiff;          // difference between succesive
    float m_fLastSyncTime;      // length of the last Video sync call
    float m_fLastSleepTime;
    float m_fDecodingTime;
    PthreadTask* m_pVideoThread;   // performs video output and sync
    PthreadTask* m_pAudioThread;   // performs audio output and sync
    PthreadTask* m_pDecoderThread; // performs video decompression and caching

    uint_t m_iEffectiveUid;	// effective user id
    uint_t m_iEffectiveGid;	// effective group id

    framepos_t m_uiPgPrevPos;
    int64_t m_lPgPrevTime;


    PthreadMutex m_ThreadMut[THREAD_LAST]; // 3 mutexes
    PthreadCond m_ThreadCond[THREAD_LAST]; // 3 conditions
    PthreadMutex m_LockMutex;

    PthreadMutex m_QueueMutex;
    PthreadCond m_QueueCond;

    double m_dVideoSeekDest;
    double m_dVframetime;
    int m_iDepth;
    uint_t m_CSP;               // used colorspace by renderer

    bool m_bVideoMute;
    bool m_bVideoAsync;		// true - do not care about a-v sync
    bool m_bQualityAuto;
    bool m_bVideoBuffered;
    bool m_bVideoDirect;
    bool m_bVideoDropping;
    bool m_bDropping;           // internal flag for temporal stop of decoder

    bool m_bPaused;		// true if we are paused
    bool m_bInitialized;	// true if we are playing OR paused
    bool m_bQuit;		// true signals to all processing threads
				// to terminate

    bool m_bHangup;		// signals main_thread to enter 'waiting' state
				// and set initialized=0
    bool m_bBuffering;
    bool m_bCallSync;           // marks that video update is needed
    bool m_bConstructed;           // marks that video update is needed
};

AVM_END_NAMESPACE;

#endif // AVIPLAY_IMPL_H
