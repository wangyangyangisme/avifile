#ifndef AVIFILE_IAUDIORENDERER_H
#define AVIFILE_IAUDIORENDERER_H

#include "avifile.h"
#include "formats.h"
#include "avm_locker.h"

AVM_BEGIN_NAMESPACE;

class AudioQueue;

// call with  started audio stream!
class IAudioRenderer
{
public:
    static const int VOL_MAX = 1000;
    static const int BAL_MAX = 1000;

    IAudioRenderer(IReadStream* astream, WAVEFORMATEX& m_Owf);
    virtual ~IAudioRenderer();
    virtual int Init() { return 0; }
    // -1: error   range 0-1000
    virtual int GetVolume();
    // -1: error   range 0-500-1000  500 middle
    virtual int GetBalance();
    virtual int SetVolume(int volume);
    virtual int SetBalance(int balance);

    // no overloadable at this moment
    bool Eof() const;
    int Extract();
    const char* GetAudioFormat() const;
    double GetCacheSize() const;
    double GetLengthTime() const;
    double GetStreamTime() const { return m_dStreamTime; }
    double GetTime();
    double GetBufferTime() const;//for how long we can play without Extract()'s?
    int Pause(bool state);      // 0 - Ok   -1 - ignored
    int SeekTime(double pos);
    void SetAsync(float async) { m_fAsync = async; }

    int SetPlayingRate(int rate) { return -1; }
    int SetResamplingRate(int rate) { return -1; }

    void Start();
    void Stop();

protected:
    // overloaded
    virtual void pause(int) {}
    virtual int reset() { return 0; }
    virtual double getRendererBufferTime() const { return 0.0; }
    void updateTimer();


    IReadStream* m_pAudiostream;
    AudioQueue* m_pQueue; 	///< using pointer - faster gcc3.0 compilation
    int64_t m_lTimeStart;	///< Timestamp of start
				/// updated after each reseek or pause
    uint_t m_uiSamples;         ///< Decoded samples from last seek
    double m_dStreamTime;	///< Precise stream time
    double m_dSeekTime;		///< Precise position at seek
    double m_dAudioRealpos;	///< Position used for timing
    double m_dPauseTime;	///< Timestamp at pause

    WAVEFORMATEX m_Iwf;		///< Input format of data we get from the audiostream
    WAVEFORMATEX m_Owf;		///< Resulting Output format of data
    double m_dIwfBPS;
    double m_dOwfBPS;

    int m_iBalance;
    int m_iVolume;
    float m_fAsync;		///< by default 0, hardware-dependent

    bool m_bQuit;		///< true when terminating thread
    bool m_bPaused;		///< true if we are paused
    bool m_bInitialized;	///< true if we are playing OR paused
};

AVM_END_NAMESPACE;

#endif // AVIFILE_IAUDIORENDERER_H
