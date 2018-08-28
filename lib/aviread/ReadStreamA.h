#ifndef AVIPLAY_READSTREAMA_H
#define AVIPLAY_READSTREAMA_H

#include "ReadStream.h"

AVM_BEGIN_NAMESPACE;

class IAudioDecoder;
class AudioInfoMp3;

class ReadStreamA : public ReadStream
{
public:
    ReadStreamA(IMediaReadStream* stream);
    virtual ~ReadStreamA();
    virtual uint_t GetAudioFormat(void* format = 0, uint_t size = 0) const;
    virtual IAudioDecoder* GetAudioDecoder() const { return m_pAudiodecoder; };
    virtual uint_t GetFrameSize() const;
    virtual uint_t GetOutputFormat(void* format = 0, uint_t size = 0) const;
    virtual framepos_t GetPos() const;
    virtual double GetTime(framepos_t frame = ERR) const;
    virtual bool IsStreaming() const;
    virtual int SkipTo(double pos);
    virtual int ReadFrames(void* buffer, uint_t bufsize, uint_t samples,
			       uint_t& samples_read, uint_t& bytes_read);
    virtual int SeekTime(double pos);
    virtual int StartStreaming(const char*);
    virtual int StopStreaming();

protected:
    virtual void Flush();
    IAudioDecoder* m_pAudiodecoder;
    uint_t m_uiBps;
    uint_t m_uiMinSize;
    uint_t m_uiSampleSize;
    bool m_bIsMp3;
};

AVM_END_NAMESPACE;

#endif // AVIPLAY_READSTREAMA_H
