#ifndef __VIDEOENCODER_H
#define __VIDEOENCODER_H
/********************************************************

	Video encoder interface
	Copyright 2000 Eugene Kuznetsov  (divx@euro.ru)

*********************************************************/

#include "videoencoder.h"
#include "Module.h"

AVM_BEGIN_NAMESPACE;

class VideoEncoder: public IVideoEncoder
{
public:
    VideoEncoder(const CodecInfo& info, fourcc_t compressor, const BITMAPINFOHEADER& header);
    ~VideoEncoder();
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
    virtual const BITMAPINFOHEADER& GetOutputFormat() const;
    virtual int GetOutputSize() const;

    virtual int Start();
    virtual int Stop();
    //
    // Quality takes values from 0 to 10000, 10000 the best
    //
    virtual int GetQuality() const { return m_iQuality; }
    virtual int SetQuality(int quality);
    virtual int GetKeyFrame() const { return m_iKeyRate; }
    virtual int SetKeyFrame(int frequency);
    virtual float GetFps() const { return m_fFps; }
    virtual int SetFps(float fps);

    int init(); // avoid throws
protected:
    void setDivXRegs(void);
    Module* m_pModule;
    HIC m_HIC;
    BitmapInfo* m_bh;
    BitmapInfo* m_bhorig;
    BITMAPINFOHEADER* m_obh;
    BITMAPINFOHEADER* m_obhformat;
    char* m_prev;
    char* m_pConfigData;
    int m_iConfigDataSize;
    int m_comp_id;
    int m_iState;
    int m_iFrameNum;
    int m_iQuality;
    int m_iKeyRate;
    int m_iBitrate;
    int m_iLastKF;
    float m_fFps;
};

AVM_END_NAMESPACE;

#endif // __VIDEOENCODER_H
