#ifndef RECOMPRESSOR_H
#define RECOMPRESSOR_H

/*******************************************************

	GUI-independent section of recompressor

*******************************************************/

#include "filters.h"
#include "imagecontrol.h"
#include "rec_cb.h"
#include <avifile.h>
#include <audioencoder.h>
#include <videodecoder.h>
#include <videoencoder.h>
#include <avm_locker.h>

extern char* g_pcProgramName;

struct AudioEncoderInfo
{
    int fmt;
    int bitrate;
};

class RecompressFilter: public Filter
{
    friend class RecKernel;
public:
    RecompressFilter(const avm::VideoEncoderInfo& in);
    ~RecompressFilter();
    const char* name() const { return "Recompress filter"; }
    avm::string fullname() { return name(); }
    avm::CImage* process(avm::CImage* im, int pos);
    void about() {}
    void config() {}
    avm::string save();
    void load(avm::string);
private:
    avm::VideoEncoderInfo info;
    char* comp_frame;
    avm::IVideoDecoder* vd;
    avm::IVideoEncoder* ve;
};

class RecKernel
{
friend class Filter;
public:
    enum KernelMode { Remove = 0, Copy, Recompress };

    RecKernel();
    ~RecKernel();
    Filter* getFilter(uint_t id);		//creates filter registered under id
    Filter* getFilterAt(uint_t id);		//returns filter at pos id in current list
    unsigned int getFilterCount() const;	//size of current list
    int addFilter(Filter* fi);			//adds filter in the end of list
    int removeFilter(uint_t id);
    int moveUpFilter(uint_t id);
    int moveDownFilter(uint_t id);

    int openFile(const char* fn);
    const char* getSrcFile() {return m_ReadFn.c_str();}
    int setDestFile(const char* fn);
    const char* getDestFile() {return m_Filename.c_str();}


    avm::string aboutStream(uint_t id) const;	//first all video streams, then audio

    avm::string lastError() const;

    bool isVideoStream(uint_t id) const;
    bool isAudioStream(uint_t id) const;

    avm::IStream::StreamType getStreamType(uint_t id) const;

//
//    int getVideoInfo(int id, VideoEncoderInfo& vi) const;
//    int getAudioInfo(int id, WAVEFORMATEX& wfmtx) const;
//
    bool isStream(uint_t id) const;//nonzero if stream

    int setStreamMode(uint_t id, KernelMode mode);
    int getStreamMode(uint_t id, KernelMode& mode) const;
    int getSelection(uint_t id, framepos_t& start, framepos_t& end) const;
    int setSelection(uint_t id, framepos_t start, framepos_t end);
    int setSelectionStart(uint_t id, framepos_t start);
    int setSelectionEnd(uint_t id, framepos_t end);
    double getFrameTime(uint_t id) const;
    double getTime(uint_t id) const;
    void setImageControl(IImageControl* ctl);//should set size & pictures if ctl!=0 & file is opened

    avm::string getStreamName(uint_t id) const;
    unsigned int getStreamCount() const;
    framepos_t seek(int delta);//in frames
    //negative seeks are very slow
    framepos_t seekPos(framepos_t pos);//seek to nearest keyframe, return its position
    framepos_t seekNextKeyFrame();
    framepos_t seekPrevKeyFrame();
    framepos_t pos() const;

    int setCompress(uint_t id, const avm::VideoEncoderInfo& vi);
    int setAudioCompress(uint_t id, const AudioEncoderInfo& wf);
    int getCompress(uint_t id, avm::VideoEncoderInfo& vi) const;
    int getAudioCompress(uint_t id, AudioEncoderInfo& wf) const;

    unsigned int getVideoLength() const;

    int setCallback(IRecompressCallback* pRecCallback);
    int startRecompress();
    int pauseRecompress();
    int stopRecompress();

    void redraw(bool readFrame = false);

    int loadConfig(const char*);
    int saveConfig(const char*);
private:
    static void* recompressThreadStart(void* arg);
    void* recompressThread();
    void _destruct();
    void swap(int x, int y) {
	Filter* t = m_FilterList[x];
	m_FilterList[x] = m_FilterList[y];
        m_FilterList[y] = t;
    }

    avm::string m_Filename;
    avm::string m_ReadFn;
    avm::string strLastError;
    IImageControl* _ctl;
    RecompressFilter* m_pRecFilter;
    IRecompressCallback* m_pRecCb;
    avm::IWriteFile* m_pWriteFile;
    avm::vector<avm::IReadFile*> m_ReadFiles;
    uint_t vs, as;
    struct full_stream
    {
	avm::IReadStream* stream;
	KernelMode mode;
	framepos_t startpos;
	framepos_t endpos;
	AudioEncoderInfo ai;
	avm::VideoEncoderInfo vi;
	int audiosz;
    };
    avm::vector<full_stream> m_Streams;
    avm::vector<Filter*> m_FilterList;

    int rec_status;
    int pause_status;
    avm::PthreadTask* rec_thread;
    avm::PthreadMutex m_Mutex;
    avm::PthreadCond  m_Cond;
};

#endif // RECOMPRESSOR_H
