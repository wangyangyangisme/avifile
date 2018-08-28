#ifndef AVIFILE_STREAMINFO_H
#define AVIFILE_STREAMINFO_H

#ifndef AVIFILE_INFOTYPES_H
#error Wrong usage of StreamInfo.h - include only infotypes.h!
#endif
#include "avm_default.h"
#include "avm_stl.h"

/*
 * this file could be a part of infotypes in future  > so be careful
 *
 * so include only !!!! infotypes.h !!!! (StreamInfo is included there!)
 */

AVM_BEGIN_NAMESPACE;

class IMediaReadStream;
class AviReadStream;
class AsfReadStream;
class FFReadStream;


class StreamInfo
{
    friend class IMediaReadStream;
    friend class AviReadStream;
    friend class AsfReadStream;
    friend class FFReadStream;
public:
    enum Type { Video, Audio, Other };

    StreamInfo();
    StreamInfo(const StreamInfo&);// copy
    ~StreamInfo();

    /// returns string representation of this Info structure
    avm::string GetString() const;

    /// stream type (Video, Audio,...)
    Type GetType() const;

    /// Format tag (FourCC for Video, WAVEFORMAT Tag for audio,
    uint_t GetFormat() const;
    const char* GetFormatName() const;

    /// Quality of the stream 0..10000 (not sure here)
    int GetQuality() const;

    double GetBps() const;	// calculated Bps
    double GetFps() const;	// calculated Fps

    /// total length of the stream in seconds
    double GetLengthTime() const;
    /// total stream size
    int64_t GetStreamSize() const;
    /// frames in the stream
    uint_t GetStreamFrames() const;

    /// size of each sample in stream - will be 0 for VBR streams
    int GetSampleSize() const;

    /// some stats about frames
    /// note - for Audio stream are all frames marked as KeyFrames
    uint_t GetMaxKfFrameSize() const;
    uint_t GetMinKfFrameSize() const;
    uint_t GetAvgKfFrameSize() const;
    uint_t GetKfFrames() const;
    int64_t GetKfFramesSize() const;

    /// delta frames - valid for video stream
    uint_t GetMaxFrameSize() const;
    uint_t GetMinFrameSize() const;
    uint_t GetAvgFrameSize() const;
    uint_t GetFrames() const;
    int64_t GetFramesSize() const;


    // for non video returns -1
    int GetVideoWidth() const;
    int GetVideoHeight() const;
    float GetAspectRatio() const;

    // for non audio returns -1
    int GetAudioBitsPerSample() const;
    int GetAudioChannels() const;
    int GetAudioSamplesPerSec() const;

    const avm::vector<avm::string>& GetProperties() const;
protected:
    struct StreamInfoPriv;      // forward declaration
    struct StreamInfoPriv* m_p; // private structure - hidden
};

AVM_END_NAMESPACE;

#ifdef AVM_COMPATIBLE
typedef avm::StreamInfo StreamInfo;
#endif
#endif // AVIFILE_STREAMINFO_H
