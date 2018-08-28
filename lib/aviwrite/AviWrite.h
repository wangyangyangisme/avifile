#ifndef AVIFILE_AVIWRITE_H
#define AVIFILE_AVIWRITE_H

#include "avifile.h"
#include "avifmt.h"
#include "FileBuffer.h"

AVM_BEGIN_NAMESPACE;

class AviWriteFile;
class AviWriteStream : public IWriteStream
{
    friend class AviWriteFile;
public:
    AviWriteStream(AviWriteFile* file, int ckid,
		   IStream::StreamType type,
		   fourcc_t handler, int frame_rate,
                   int flags = 0,
		   const void* format = 0, uint_t format_size = 0,
		   uint_t samplesize = 0, int quality = 0);
    virtual ~AviWriteStream();
    virtual int AddChunk(const void* chunk, uint_t size, int flags = 0);
    //
    // these two should be called before data insertion begins
    //
    //
    // for video streams, specify time in microsecs per frame in frame_rate
    // for audio streams - count of bytes per second
    //
    virtual uint_t GetLength() const { return m_Header.dwLength; }
    virtual StreamType GetType() const { return m_type; }
    virtual int Start() { return 0; };
    virtual int Stop() { return 0; };
protected:
    void SetAudioHeader(int samplesize, int bitrate, int scale)
    {
	m_Header.dwSampleSize = samplesize;
	m_Header.dwRate = bitrate;
	m_Header.dwScale = scale;
    }
    void SetVideoHeader(int quality, int width, int height)
    {
    	m_Header.dwQuality = quality;
        m_Header.rcFrame.right = width;
	m_Header.rcFrame.bottom = (height < 0) ? -height : height;
    }

    AviWriteFile* m_pFile;
    AVIStreamHeader m_Header;
    StreamType m_type;
    char* m_pcFormat;
    uint_t m_uiFormatSize;
    uint_t m_uiLength;
    int m_ckid;
    int m_stop; // marks if Stop should be called - for Audio or Video stream
};

class IAudioEncoder;
class AviAudioWriteStream : public AviWriteStream, public IAudioWriteStream
{
public:
    AviAudioWriteStream(AviWriteFile* file, int ckid, const CodecInfo& ci,
			const WAVEFORMATEX* fmt, int bitrate, int flags = 0);
    virtual ~AviAudioWriteStream();
    virtual int AddData(void* data, uint_t size);
    virtual const CodecInfo& GetCodecInfo() const;
    virtual uint_t GetLength() const;
    // IAudioWriteStream
    virtual int Start();
    virtual int Stop();
private:
    IAudioEncoder* m_pAudioEnc;
    WAVEFORMATEX srcfmt;
    int m_astatus;
    int m_bitrate;
};

class IVideoEncoder;
class AviVideoWriteStream : public AviWriteStream, public IVideoWriteStream
{
public:
    AviVideoWriteStream(AviWriteFile* file, int ckid, const CodecInfo& ci,
			const BITMAPINFOHEADER* srchdr,
			int frame_rate, int flags = 0);
    virtual ~AviVideoWriteStream();
    virtual int AddFrame(CImage* chunk, uint_t* = 0, int* = 0, char** = 0);
    virtual const CodecInfo& GetCodecInfo() const;
    virtual uint_t GetLength() const;
    // IVideoWriteStream
    virtual int Start();
    virtual int Stop();
private:
    IVideoEncoder* m_pVideoEnc;
    int m_vstatus;
    char* m_pBuffer;
};

class AviSegWriteFile;
class AviWriteFile: public IWriteFile
{
friend class AviWriteStream;
friend class AviSegWriteFile;
public:
    AviWriteFile(const char* name, int64_t limit = 0, int flags = 0, int mask = 00666);
    virtual ~AviWriteFile();
    virtual IVideoWriteStream* AddVideoStream(fourcc_t fourcc,
					      const BITMAPINFOHEADER* srchdr,
					      int frame_rate, int flags = 0);
    virtual IVideoWriteStream* AddVideoStream(const CodecInfo& ci,
					      const BITMAPINFOHEADER* srchdr,
					      int frame_rate, int flags = 0);
    virtual IVideoWriteStream* AddVideoStream(const VideoEncoderInfo* vi,
					      int frame_rate, int flags = 0);
    virtual IAudioWriteStream* AddAudioStream(fourcc_t fourcc,
					      const WAVEFORMATEX* format,
					      int bitrate, int flags = 0);
    virtual IAudioWriteStream* AddAudioStream(const CodecInfo& ci,
					      const WAVEFORMATEX* format,
					      int bitrate, int flags = 0);
    virtual IWriteStream* AddStream(IStream::StreamType type,
				    const void* format, uint_t format_size,
				    fourcc_t handler, int frame_rate,
				    uint_t samplesize = 0,
				    int quality = 0, int flags = 0);
    virtual IWriteStream* AddStream(IReadStream* pCopyStream);
    virtual const char* GetFileName() const { return m_Filename.c_str(); }
    virtual int64_t GetFileSize() const;
    virtual int Reserve(uint_t size);
    virtual int WriteChunk(fourcc_t fourcc, void* data, uint_t size);
    virtual void WriteHeaders();
    virtual int Segment();
    virtual void SegmentAtKeyframe();
    virtual void setSegmentName(avm::string new_name);

protected:
    //
    //called when data is added to stream
    //
    void AddChunk(const void* offset, uint_t size, uint_t ckid, int flags = 0);
    void init();
    void finish();
    void write_le32(int x) { m_pFileBuffer->write_le32(x); }
    FileBuffer* m_pFileBuffer;
    int64_t m_lFlimit;

    // local
    AVIMainHeader m_Header;
    avm::string m_Filename;
    avm::vector<AviWriteStream*> m_Streams;
    avm::vector<AVIINDEXENTRY> m_Index;
    int m_iStatus;
    int m_iFlags;
    int m_iMask;
    bool m_bSegmented;

    bool segment_flag;
    avm::string segmenting_name;
};

AVM_END_NAMESPACE;

#endif // AVIFILE_AVIWRITE_H
