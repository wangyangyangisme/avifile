#ifndef AVIFILE_AUDIOQUEUE_H
#define AVIFILE_AUDIOQUEUE_H

#include "formats.h"
#include "avm_locker.h"
#include "avm_stl.h"

AVM_BEGIN_NAMESPACE;

class IAudioResampler;
class IAudioCleaner;

// buggy gcc-4.0 refused to compile static const double
#define MAX_BUFFER_TIME 1.0

class IAudioMix
{
public:
    virtual ~IAudioMix() {}
    virtual int Mix(void* data, const void* src, uint_t n) const = 0;
};

class AudioQueue
{
public:
    //static const double MAX_BUFFER_TIME = 1.0;

    AudioQueue(WAVEFORMATEX& Iwf, WAVEFORMATEX& Owf);
    ~AudioQueue();

    void Broadcast() { m_Cond.Broadcast(); }
    void Clear();
    void DisableCleaner();

    double GetBufferTime() const { return GetSize() / (double) GetBytesPerSec(); }
    uint_t GetBytesPerSec() const { return m_uiBytesPerSec; }
    uint_t GetSize() const { return m_uiBufSize; }

    bool IsFull() { return m_Bufs.full() || GetBufferTime() > MAX_BUFFER_TIME; }

    int Read(void* data, uint_t count, const IAudioMix* amix = 0);
    uint_t Resample(void* dest, const void* src, uint_t src_size);
    int Write(char* b, uint_t count); // add new samples

    int Wait(float waitTime = -1.0)
    {
        Broadcast();
	return m_Cond.Wait(m_Mutex, waitTime);
    }
    int Lock() { return m_Mutex.Lock(); }
    int Unlock() { return m_Mutex.Unlock(); }
protected:
    WAVEFORMATEX m_Iwf;		///< format of data we get from audio stream
    WAVEFORMATEX m_Owf;		///< format of data audio output

    PthreadCond m_Cond;      	///< broadcast to notify audio_thread about
				///< new available data OR about quit=1
    PthreadMutex m_Mutex;	///< controls access to audio buffer
    uint_t m_uiBytesPerSec;	///< input and output positions in queue
    IAudioResampler* m_pResampler;
    double m_dRemains;          ///< remainder for resampling

    uint_t m_uiBufSize;

    struct chunk {
	char* mem;              ///< memchunk
        int size;               ///< chunk size
        int rsize;              ///< already readed samples from this chunk
    };

    qring<chunk> m_Bufs;

    IAudioCleaner* m_pCleaner;
    bool m_bCleared;            ///< cleared buffers
};

AVM_END_NAMESPACE;

#endif // AVIFILE_AUDIOQUEUE_H
