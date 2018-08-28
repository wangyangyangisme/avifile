#ifndef CAPPROC_H
#define CAPPROC_H

#include <avm_stl.h>
#include <avm_locker.h>
#include <infotypes.h>

#include <string.h>

class ClosedCaption;
class frame_allocator;
class dsp;
class v4lxif;

enum Sound_Freqs { NoAudio = -1, F48, F44, F32, F22, F16, F12, F11, F8 };
enum Sample_Sizes { S16, S8 };
enum Sound_Chans { Mono, Stereo, Lang1, Lang2 };
enum Colorspaces { cspYV12, cspYUY2, cspRGB15, cspRGB24, cspRGB32, cspI420 };

enum Resolutions
{
    WNONE =-1,
    W160  =0,
    W192  =1,
    W320  =2,
    W384  =3,
    W400  =4,
    W512  =5,
    W576  =6,
    W640  =7,
    W768  =8,
};

struct chunk
{
    char* data;
    double timestamp;
    int size;
};

struct CaptureProgress
{
    static const int HISTORY_SIZE = 256;

    avm::PthreadMutex mutex;
    int video_frame_len;
    int buffer_len;
    int max_frames;

    int64_t file_size;
    int64_t enc_time;
    int64_t video_bytes;
    int64_t audio_bytes;
    double video_time;
    double audio_time;
    int video_frames;
    int synch_fix;
    int video_frame_sizes[HISTORY_SIZE];
    int video_frame_head;
    int buffer_occupancies[HISTORY_SIZE];
    int buffer_head;
    int dropped_cap;
    int dropped_enc;

    const char* filename;
    float framerate;
    int xdim;
    int ydim;
    int audio_freq;
    int audio_ssize;
    int audio_channels;

    double audio_error,video_error,timestamp_shift;

    CaptureProgress() : video_frame_len(0), buffer_len(0), max_frames(0),
	file_size(0), enc_time(0),
	video_bytes(0), audio_bytes(0),
	video_time(0), audio_time(0),
	video_frames(0), synch_fix(0),
	video_frame_head(0), buffer_head(0),
	dropped_cap(0), dropped_enc(0),
	audio_error(0),video_error(0),
	timestamp_shift(0)
    {
    }

    ~CaptureProgress() {}
    void lock() { mutex.Lock(); }
    void unlock() { mutex.Unlock(); }
    void init(const char* fn, float fps, int xd, int yd, int freq, int ssize, int channels, int frames)
    {
	lock();
	filename=fn;
	framerate=fps;
	xdim=xd;
	ydim=yd;
	audio_freq=freq;
	audio_ssize=ssize;
	audio_channels=channels;
	max_frames=frames;
	memset(video_frame_sizes, 0, sizeof(video_frame_sizes));
	unlock();
    }
    void update(long long etime, double vidtime, double audtime,
		long long audbytes, long long fsize, int synchfix,
		int vframe, int bufstatus,
		int drop_cap, int drop_enc,
		const char* fn)
    {
	lock();
	enc_time = etime;
	video_time = vidtime;
	audio_time = audtime;
	if (vframe > 0)
	    video_bytes += (vframe & ~0x40000000);
	audio_bytes = audbytes;
	file_size = fsize;
	synch_fix = synchfix;
	if (vframe >= 0)
	{
	    video_frames++;
	    video_frame_sizes[video_frame_head]=vframe;
	    video_frame_head++;
	    video_frame_head %= HISTORY_SIZE;
	    if (video_frame_len < HISTORY_SIZE)
		video_frame_len++;
	    buffer_occupancies[buffer_head]=bufstatus;
	    buffer_head++;
	    buffer_head %= HISTORY_SIZE;
	    if (buffer_len < HISTORY_SIZE)
		buffer_len++;
	    dropped_cap = drop_cap;
	    dropped_enc = drop_enc;
	}
	filename = fn;
	unlock();
    }
    void setVideoError(double val){
	lock();
	video_error=val;
	unlock();
    }
    void setAudioError(double val){
	lock();
	audio_error=val;
	unlock();
    }
    void setTimestampShift(double val){
	lock();
	timestamp_shift=val;
	unlock();
    }
};

struct CaptureConfig
{
    avm::string filename;
    bool never_overwrite;
    int segment_size;
    // Segmentation is implicit finishing one
    // file and starting next one when old file
    // grows to specified size
    //Kbytes
    //-1 for no segmentation
    avm::VideoEncoderInfo codec;
    // compressor-dependent
    // for IV 5.0 Quick it's ignored
    int frequency;
    // frequency ID
    //-1 for no audio
    avm::string audio_device;
    int audio_compressor;
    int audio_bitrate;
    int samplesize;
    int chan;
    int res_w;
    int res_h;
    int over_t, over_b, over_l, over_r;
    int timelimit;		//in seconds. -1 if no limit
    int sizelimit;		//in Kbytes. -1 if no limit
    float fps;
    bool deinterlace;
    Colorspaces colorspace;
    void load();
    void setNamedCodec(avm::string named_codec);
    void setDimensions(Resolutions res);
    void setChannels(Sound_Chans c);
    void setSamplesize(Sample_Sizes ss);
    void setFrequency(Sound_Freqs freq);
    void setFilename(avm::string fname) { filename=fname; };
};

template <class Type> class lockqring : public avm::qring<Type>
{
    avm::PthreadMutex mutex;
public:
    lockqring<Type>(uint_t cap) : avm::qring<Type>(cap) {}
    void push(const Type& m) { avm::Locker locker(mutex); avm::qring<Type>::push(m); }
    void pop() { avm::Locker locker(mutex); avm::qring<Type>::pop(); }
};

class CaptureProcess
{
public:
    CaptureProcess(v4lxif* v4l,
		   CaptureConfig* conf,
		   ClosedCaption* ccap);
    ~CaptureProcess();

    void getState(int& finished, avm::string& e)
    {
	finished = m_quit;
	e = error;
    }

    void setMessenger(CaptureProgress* pProgress);

    void segment() { segment_flag = 1; }
    void setSegmentName(avm::string name) { segmenting_name=name; }

protected:
    void* vidcap();
    void* audcap();
    void* writer();
    static void* vidcap_starter(void* arg) { return ((CaptureProcess*)arg)->vidcap(); }
    static void* audcap_starter(void* arg) { return ((CaptureProcess*)arg)->audcap(); }
    static void* writer_starter(void* arg) { return ((CaptureProcess*)arg)->writer(); }
    static void capwriter(void*, const char*);
    v4lxif* m_v4l;
    avm::PthreadTask* m_pVidc;
    avm::PthreadTask* m_pAudc;
    avm::PthreadTask* m_pWriter;
    avm::PthreadTask* m_pSub;
    lockqring<chunk> m_qvid;
    lockqring<chunk> m_qaud;
    int m_quit;
    int cnt;
    int cap_drop;
    int comp_drop;
    int ccfd;
    int64_t starttime;
    int64_t lastcctime;
    avm::string lastccstring;

    CaptureProgress* m_pProgress;
    ClosedCaption* m_ccap;
    frame_allocator* m_pAllocator;
    CaptureConfig m_conf;
    dsp* m_pDsp;

    avm::string error;
    avm::string segmenting_name;
    int segment_flag;

    int vid_clear;
    int aud_clear;
    int audioblock;

private:
    avm::string usedname;
    avm::string find_filename(avm::string checkname);
};

#endif /* CAPPROC_H */
