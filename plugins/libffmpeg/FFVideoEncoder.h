#ifndef AVIFILE_FFVIDEOENCODER_H
#define AVIFILE_FFVIDEOENCODER_H

#include "videoencoder.h"
#include "avcodec.h"

#include <stdlib.h>

AVM_BEGIN_NAMESPACE;

class FFVideoEncoder: public IVideoEncoder
{
public:
    FFVideoEncoder(AVCodec* av, const CodecInfo& info, fourcc_t compressor, const BITMAPINFOHEADER& header);
    ~FFVideoEncoder();
    //
    // Encodes single frame
    // On success, is_keyframe contains 0 if frame was encoded into delta frame, otherwise AVIIF_KEYFRAME (?)
    // size receives size of compressed frame
    //
    virtual int EncodeFrame(const CImage* src, void* dest, int* is_keyframe, uint_t* size, int* lpckid = 0);
    //
    // Queries encoder about desired buffer size for compression
    // You should allocate at least this much bytes for dest ( arg for EncodeFrame )
    //
    virtual const BITMAPINFOHEADER& GetOutputFormat() const
    {
	return m_obh;
    }
    virtual int GetOutputSize() const
    {
	return m_bh.biWidth * labs(m_bh.biHeight) * 4;
    }
    virtual int Start();
    virtual int Stop();
    //
    // Quality takes values from 0 to 10000, 10000 the best
    //
    virtual int SetQuality(int quality);
    virtual int GetQuality() const { return m_iQual; }
    virtual int SetKeyFrame(int frequency);
    virtual int GetKeyFrame() const { return m_iKfFreq; }
    virtual float GetFps() const { return m_fFps; }
    virtual int SetFps(float fps);
protected:
    AVCodec* m_pAvCodec;
    AVCodecContext* m_pAvContext;

    CAPS m_Caps;
    BITMAPINFOHEADER m_bh;
    BITMAPINFOHEADER m_obh;
    int m_iQual;
    int m_iKfFreq;
    float m_fFps;
};

AVM_END_NAMESPACE;

#endif //AVIFILE_FFVIDEOENCODER_H
