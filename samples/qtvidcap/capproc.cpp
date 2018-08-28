#define USE_REGISTRY
#include "capproc.h"

#include "dsp.h"
#include "ccap.h"

#include <avifile.h>
#include <videoencoder.h>
#include <image.h>
#include <avm_fourcc.h>
#include <avm_cpuinfo.h>
#include <utils.h>
#include <avm_creators.h>
#include <avm_except.h>

#define DECLARE_REGISTRY_SHORTCUT
#include <configfile.h>
#undef DECLARE_REGISTRY_SHORTCUT

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include "v4lxif.h"
//#include "v4lwindow.h"

extern unsigned int m_iMemory;

#define __MODULE__ "Capture Config"

void copy_deinterlace_24(void* outpic, const void* inpic, int xdim, int height);
extern "C"{
void deinterlace_bob_yuv_mmx(void* outpic, const void* inpic, int xdim, int height);
}

static struct restable_s {
    enum Resolutions res;
    int width;
    int height;
} restable[] = {
    { W768, 768, 576 },
    { W640, 640, 480 },
    { W576, 576, 432 },
    { W400, 400, 300 },
    { W384, 384, 288 },
    { W320, 320, 240 },
    { W192, 192, 144 },
    { W160, 160, 120 },
    { W512, 512, 384 },
    { WNONE, 0, 0 }
};


#ifdef USE_REGISTRY

void CaptureConfig::setNamedCodec(avm::string named_codec){
    if(named_codec==""){
      return;
    }
      //search the regnumber of our codec
      int regnumber=0;

      char regbuf[300];

      sprintf(regbuf,"NamedCodecs-%02d-Savename",regnumber);
      avm::string nc_savename=RS(regbuf,"NONE");

      while(!(named_codec==nc_savename || nc_savename=="NONE")){
	regnumber++;

	 sprintf(regbuf,"NamedCodecs-%02d-Savename",regnumber);
	 nc_savename=RS(regbuf,"NONE");

      }
      if(nc_savename!="NONE"){
	sprintf(regbuf,"NamedCodecs-%02d-Compressor",regnumber);
	codec.compressor=RI(regbuf,fccIV50);

	sprintf(regbuf,"NamedCodecs-%02d-Quality",regnumber);
	codec.quality=RI(regbuf,9500);

	sprintf(regbuf,"NamedCodecs-%02d-Keyframe",regnumber);
	codec.keyfreq=RI(regbuf,15);

	printf("capconf: setting named codec to %s/%d\n",named_codec.c_str(), codec.compressor);
	//avm::codecSetSaveName(named_codec);
      }
}

void CaptureConfig::load()
{
    audio_compressor = 1;
    deinterlace = false;

    audio_device = RS("AudioDevice", "/dev/dsp");
    filename = RS("FileName", "./movie.avi");
    never_overwrite = RI("NeverOverwriteFiles", 0);
    segment_size = RI("IsSegmented", 1)?RI("SegmentSize", 1000*1000):-1;
    codec.compressor = RI("Compressor", fccIV50);
    codec.cname = RS("Codec", "odivx");
    codec.quality = RI("Quality", 9500);
    codec.keyfreq = RI("Keyframe", 15);
    timelimit = RI("LimitTime", 0)?RI("TimeLimit", 3600):-1;
    sizelimit = RI("LimitSize", 0)?RI("SizLimit", 2000000):-1;
    fps = RI("FPS", 25000)/1000.0;
    colorspace = (enum Colorspaces)RI("Colorspace", 0);

    //avm::codecSetSaveName("");

    avm::string named_codec=RS("NamedCodecsCurrent","");
    setNamedCodec(named_codec);
    setDimensions((enum Resolutions)RI("Resolution", 0));
    setFrequency((enum Sound_Freqs)(RI("HaveAudio", 1)?RI("Frequency", 0):NoAudio));
    if (frequency>0)
    {
	setSamplesize((enum Sample_Sizes)RI("SampleSize", 0));
	setChannels((enum Sound_Chans)RI("SndChannels", 0));
    }
}

void CaptureConfig::setDimensions(Resolutions res)
{
    int i = 0;
    while (restable[i].res != res && restable[i].res != WNONE)
    	i++;
    if (restable[i].res == res)
    {
	over_l = 0;
	over_r = 0;
	over_t = 0;
	over_b = 0;
    	res_w = restable[i].width;
	res_h = restable[i].height;
	//cout<<"Resolution: "<<res_w<<" x "<<res_h<<endl;
    }
    else
    	throw FATAL("Unknown video resolution");
}

void CaptureConfig::setChannels(Sound_Chans c)
{
    switch (c)
    {
    case Mono:
    case Lang1:
    case Lang2:
        chan = 1;
        break;
    case Stereo:
        chan = 2;
        break;
    default:
        throw FATAL("Unknown channel number");
    }
}
void CaptureConfig::setSamplesize(Sample_Sizes ss)
{
    switch (ss)
    {
    case S16:
	samplesize = 16;
        break;
    case S8:
        samplesize = 8;
        break;
    default:
        throw FATAL("Unknown audio sample size");
    }
}

void CaptureConfig::setFrequency(Sound_Freqs sfreq)
{
    switch (sfreq)
    {
    case NoAudio:
	frequency=0;
	break;
    case F48:
	frequency = 48000;
	break;
    case F44:
	frequency = 44100;
	break;
    case F32:
	frequency = 32000;
	break;
    case F22:
	frequency = 22050;
	break;
    case F16:
	frequency = 16000;
	break;
    case F12:
	frequency = 12000;
	break;
    case F11:
	frequency = 11025;
	break;
    case F8:
	frequency = 8000;
	break;
    default:
	throw FATAL("Unknown frequency");
    }
}
#endif //USE_REGISTRY

#undef __MODULE__
#define __MODULE__ "Capture Process"
class frame_allocator
{
public:
    int used_frames;

    frame_allocator(int w, int h, int limit, int bpp)
	:used_frames(0), _w(w), _h(h), _limit(limit), refs(2)
    {
	printf("Using %d buffers  (%dx%d)\n", limit,  w, h);
	while(frames.size()<(unsigned)limit)
	{
	    frame f;
	    f.data=new char[_w*_h*bpp/8+4];
	    f.status=0;
	    frames.push_back(f);
	}
    }
    ~frame_allocator()
    {
	while (frames.size())
	{
	    delete frames.back().data;
	    frames.pop_back();
	}
    }
    int get_limit() const { return _limit; }
    void release()
    {
	refs--;
	if(!refs)delete this;
    }
    char* alloc()
    {
	unsigned i;
	for(i=0; i<frames.size(); i++)
	{
	    if(frames[i].status==0)
	    {
		frames[i].status=1;
		used_frames++;
		*(int*)(frames[i].data)=i;
		return frames[i].data+4;
	    }
	}
	return 0;
    /*
	if ((int)frames.size( )>= _limit)
	    return 0;
	frame f;
	f.data=new char[_w*_h*3+4];
	f.status=1;
	frames.push_back(f);
	used_frames++;
	*(int*)(f.data)=i;
	return f.data+4;
    */
    }
    void free(char* mem)
    {
	if (!mem)
	{
	    fprintf(stderr,"ERROR: Freeing 0!\n");
	    return;
	}
	int id=*(int*)(mem-4);
	if (id < 0 || id >= (int)frames.size() || frames[id].data != (mem-4))
	{
	    fprintf(stderr,"ERROR: Freeing unknown memory!\n");
	    return;
	}
	if (frames[id].status == 0)
	{
	    fprintf(stderr, "ERROR: Duplicate free()!\n");
	    return;
	}
	used_frames--;
	frames[id].status=0;
    }
private:
    struct frame
    {
	char* data;
	int status;
    };
    avm::vector<frame> frames;
    int _w;
    int _h;
    int _limit;
    int refs;
};

CaptureProcess::CaptureProcess(v4lxif* v4l,		// v4lx interface pointer
			       CaptureConfig* conf,
			       ClosedCaption* ccap)
: m_v4l(v4l), m_qvid(64), m_qaud(64), m_quit(0),
m_pProgress(0), m_ccap(ccap), m_pAllocator(0),
m_conf(*conf), error(""), segment_flag(0), vid_clear(0), aud_clear(0)
{
  segmenting_name="";
    starttime=longcount();

    if(ccap)
    {
	avm::string ccname;
	if (m_conf.filename.size() >= 4
            && (strncasecmp(m_conf.filename.c_str() + m_conf.filename.size() - 4, ".avi", 4) == 0))
	    ccname=m_conf.filename.substr(0, m_conf.filename.size()-4)+".sub";
	else
	{
	    ccname=m_conf.filename;
	    ccname+=".sub";
	}
	ccfd=open(ccname.c_str(), O_WRONLY|O_CREAT|O_TRUNC, 00666);
	lastcctime=starttime;
	ccap->add_callback(capwriter, (void*)this);
    }
    else
	ccfd=-1;
    int bpp;

    switch(m_conf.colorspace)
    {
    case cspRGB24:
    default:
	m_v4l->grabSetParams(m_conf.res_w+m_conf.over_l+m_conf.over_r, m_conf.res_h+m_conf.over_t+m_conf.over_b, VIDEO_PALETTE_RGB24);
	bpp=24;
	break;
    case cspRGB15:
	m_v4l->grabSetParams(m_conf.res_w+m_conf.over_l+m_conf.over_r, m_conf.res_h+m_conf.over_t+m_conf.over_b, VIDEO_PALETTE_RGB555);
	bpp=16;
	break;
    case cspRGB32:
	m_v4l->grabSetParams(m_conf.res_w+m_conf.over_l+m_conf.over_r, m_conf.res_h+m_conf.over_t+m_conf.over_b, VIDEO_PALETTE_RGB32);
	bpp=32;
	break;
    case cspYUY2:
	m_v4l->grabSetParams(m_conf.res_w+m_conf.over_l+m_conf.over_r, m_conf.res_h+m_conf.over_t+m_conf.over_b, VIDEO_PALETTE_YUV422);
	bpp=16;
	break;
    case cspYV12:
    case cspI420:
	m_v4l->grabSetParams(m_conf.res_w+m_conf.over_l+m_conf.over_r, m_conf.res_h+m_conf.over_t+m_conf.over_b, VIDEO_PALETTE_YUV420P);
	bpp=12;
	break;
    }
    int maxframes = m_iMemory/(m_conf.res_w * m_conf.res_h * bpp / 8);
    if (maxframes < 5)
	maxframes = 5;

    m_v4l->setAudioMode(m_conf.chan);
    if (m_conf.frequency > 0)
    {
	m_pDsp = new dsp();
	if (m_pDsp->open(m_conf.samplesize, m_conf.chan,
			 m_conf.frequency, m_conf.audio_device) < 0)//returns file descriptor
    	    throw FATAL("Failed to open audio device");
	m_pAudc = new avm::PthreadTask(0, CaptureProcess::audcap_starter, this);
    }
    try
    {
	m_pAllocator=new frame_allocator(m_conf.res_w,m_conf.res_h,maxframes,bpp);
    }
    catch(...)
    {
	throw FATAL("Out of memory");
    }
    m_pVidc = new avm::PthreadTask(0, CaptureProcess::vidcap_starter, this);
    m_pWriter = new avm::PthreadTask(0, CaptureProcess::writer_starter, this);
}

CaptureProcess::~CaptureProcess()
{
    m_quit=1;
    fprintf(stderr, "waiting for threads to exit...\n");
    delete m_pWriter;
    fprintf(stderr, "writer has exited...\n");
    if(m_conf.frequency>0)
	delete m_pAudc;
    fprintf(stderr, "audio capture has exited...\n");
    delete  m_pVidc;
    fprintf(stderr, "video capture has...\n");
    if(m_ccap)
	m_ccap->remove_callback(capwriter, (void*)this);
    if(ccfd)
	close(ccfd);
    fprintf(stderr, "All threads exited\n");
    while (m_qaud.size())
    {
	chunk& z = m_qaud.front();
	if (z.data) delete z.data;
	m_qaud.pop();
    }
}

void CaptureProcess::capwriter(void* arg, const char* data)
{
    CaptureProcess& a=*(CaptureProcess*)arg;
    //  printf("%s\n", data);
    if(a.ccfd<0)
	return;
    if(a.lastccstring == data)
	return;
    char s[4096];
    long long curtime=longcount();
    int frame1=(int) ((a.lastcctime-a.starttime)*a.m_conf.fps/(1000.*freq));
    int frame2=(int) ((curtime-a.starttime)*a.m_conf.fps/(1000.*freq));
    if(a.lastccstring.size()==0)
    {
	a.lastcctime=curtime;
	a.lastccstring=data;
	return;
    }
    sprintf(s, "{%d}{%d}", frame1, frame2-1);
    char* ptr=&s[strlen(s)];
    const char* pp=a.lastccstring.c_str();
    while(*pp)
    {
	if(*pp==0xa)
	{
	    *ptr++='|';
	    pp++;
	}
	else
	    *ptr++=*pp++;
    }
    *ptr++=0xa;
    *ptr++=0;
    write(a.ccfd, s, strlen(s));
    //    printf("%s", s);
    a.lastccstring=data;
    a.lastcctime=curtime;
}

void* CaptureProcess::vidcap()
{
    const float fps = m_conf.fps;
    cnt=0;
    cap_drop=0;

    int w = m_conf.res_w;
    int h = m_conf.res_h;
    int line_stride = w + m_conf.over_l + m_conf.over_r;
    int bpp;

    switch (m_conf.colorspace)
    {
    case cspRGB15: bpp = 2; break;
    case cspRGB24: bpp = 3; break;
    case cspRGB32: bpp = 4; break;
    default: bpp = 1; break;
    }

    int start_pad = bpp * (w * m_conf.over_t + m_conf.over_l);
    int end_pad = bpp * (w * m_conf.over_b + m_conf.over_r);
    line_stride *= bpp;
    w *= bpp;

    //printf("W: %d   Stride %d    %d  %d   %d\n", w, line_stride, bpp, start_pad, m_conf.colorspace);

    while (!m_quit)
    {
	int64_t currenttime = longcount();
//	cerr<<currenttime<<" "<<starttime<<" "<<fps<<std::endl;
//	cerr<<to_float(currenttime, starttime)*fps<<" "<<cnt<<std::endl;
//	double freq=550000.;
	double dist=double(currenttime-starttime)/(freq*1000.);
//	double dist=to_float(currenttime, starttime);
//	cerr<<dist<<" "<<freq<<std::endl;
	if (dist * fps < cnt || m_qvid.size() >= (m_qvid.capacity() - 1))
	{
	    avm_usleep(10000);
//	    std::cerr<<"Sleeping"<<std::endl;
	    continue;
	}
	chunk ch;
	//printf("CONF %d\n", m_conf.colorspace);
	if(dist*fps<(cnt+1))
	{
	    char* z = m_v4l->grabCapture(false);

	    //4lw->avicap_renderer_display();

	    char* tmpframe = m_pAllocator->alloc();
	    char* zptr, *fptr;
	    //	    printf("%f %x %x\n", dist, z, tmpframe);
	    if (tmpframe)
	    {
		int i;
		switch (m_conf.colorspace)
		{
		case cspRGB15:
		case cspRGB24:
		case cspRGB32:
		default:
		    zptr = z + start_pad + (h - 1) * line_stride;
		    fptr = tmpframe;
		    for (i = 0; i < h; i++) {
			memcpy(fptr, zptr, w);
			fptr += w;
			zptr -= line_stride;
		    }
		    break;
		case cspYUY2:
		    zptr = z + start_pad*2;
		    fptr = tmpframe;
		    for(i=0; i<h; i++) {
			memcpy(fptr, zptr, w*2);
			fptr += w*2;
			zptr += line_stride*2;
		    }
//		    memcpy(tmpframe, z, 2*w*h);
		    break;
		case cspI420:
		    zptr = z + start_pad*3/2;
		    fptr = tmpframe;
		    for(i=0; i<h; i++) {
			memcpy(fptr, zptr, w*3/2);
			fptr += w*3/2;
			zptr += line_stride*3/2;
		    }
//		    memcpy(tmpframe, z, 3*w*h/2);
		    break;
		case cspYV12:
		    // LINUX v4l knows only I420 planar format
		    // so let's just swap two last planes
		    // and prepare YV12 planar surface
		    zptr = z + start_pad;
		    fptr = tmpframe;

		    for(i=0; i<h; i++) {
			memcpy(fptr, zptr, w);
			fptr += w;
			zptr += line_stride;
		    }
		    zptr += end_pad;

		    zptr += start_pad/4;
		    fptr = tmpframe + (w * h) * 5 / 4;
		    for(i=0; i<h; i++) {
			memcpy(fptr, zptr, w/4);
			fptr += w/4;
			zptr += line_stride/4;
		    }
		    zptr += end_pad / 4;

		    zptr += start_pad/4;
		    fptr = tmpframe + (w * h);
		    for(i=0; i<h; i++) {
			memcpy(fptr, zptr, w/4);
			fptr += w/4;
			zptr += line_stride/4;
		    }
/*
		    memcpy(tmpframe, z, w*h);
		    memcpy(tmpframe + (w * h) * 5 / 4, z + w * h, w * h / 4);
		    memcpy(tmpframe + (w * h), z + (w * h) * 5 / 4, w * h / 4);
*/
		    break;
		}
	    }
	    ch.data=tmpframe;
	}
	else
	{
	    ch.data=0;
	    cap_drop++;
	}
	ch.timestamp=dist;
        ch.size = 0;
	m_qvid.push(ch);
	cnt++;
//	if(cnt%100==0)
//	    cerr<<"Capture thread: read "<<cnt<<" frames, dropped "<<cap_drop<<" frames"<<std::endl;
    }
    m_pAllocator->release();
    fprintf(stderr, "Capture thread exiting\n");
    return 0;
}

void* CaptureProcess::audcap()
{
    float abps=m_conf.samplesize*m_conf.chan*m_conf.frequency/8;
    int bufsize=0;
    int blocksize=m_pDsp->getBufSize();
    m_pDsp->synch();
    audioblock=blocksize;
    while (!m_quit)
    {
	chunk ch;
	ch.data = new char[audioblock];
	//printf("ACT  %d  %d   %f\n", audioblock, m_pDsp->getSize(), abps);
	double timestamp_shift=(audioblock+m_pDsp->getSize())/abps;
	if(m_pProgress){
	    m_pProgress->setTimestampShift(timestamp_shift);
	}
	ch.size = m_pDsp->read(ch.data, audioblock);
	int64_t ct = longcount();
	//double freq=550000.;

	//printf("ACT  %d  %d   %f    %f\n", audioblock, m_pDsp->getSize(), abps, timestamp_shift);

#if 1
	if(starttime)
	    ch.timestamp=double(ct-starttime)/(freq*1000.)-timestamp_shift;
	else
	    ch.timestamp=0;
#else
#warning "compiling without synchfix!"
	ch.timestamp = bufsize / abps;
#endif

#ifdef AVICAP_PARANOIA1
#if 0
	char tbuf[300];
	sprintf(tbuf,"TIME %f   %f  %f\n", to_float(ct, starttime) - ch.timestamp, ch.timestamp, timestamp_shift);
	ParanoiaLog(tbuf);

	sprintf(tbuf,"ch.timestamp=%f  bufsize/abps=%f\n",ch.timestamp,bufsize / abps);
	ParanoiaLog(tbuf);
#endif
#endif

	if (m_qaud.size() < m_qaud.capacity() - 1)
	{
	    m_qaud.push(ch);
	    bufsize += ch.size;
	    //printf("Blocksze %d  %d\n", bufsize, blocksize);
	    if(blocksize/abps>.1)
	    {
		avm_usleep(10000);
	    }
	}
	//else printf("AUDIO qdrop???\n");
	//avm_usleep(20000);
    }
    delete m_pDsp;
    m_pDsp = 0;
    return 0;
}

void* CaptureProcess::writer()
{
    avm::IWriteFile* file=0;
    avm::IWriteFile* sfile=0;
    const avm::CodecInfo* ci=0;
    //    const CodecInfo* ci=0; //use this, if it does not compile...

//    int p = getpriority(PRIO_PROCESS, 0);
    //attention: only root is allowed to lower priority
//    setpriority(PRIO_PROCESS, 0, (p - 3 > -20) ? p - 3 : -20);

    avm::IVideoWriteStream* stream;
    avm::IWriteStream* audioStream=0;
    avm::IAudioWriteStream* audioCompStream=0;

    uint_t cs;
    switch (m_conf.colorspace)
    {
    case cspRGB24:
    default:
	cs=24;
	break;
    case cspRGB15:
	cs=15;
	break;
    case cspYUY2:
	cs=fccYUY2;
	break;
    case cspYV12:
	cs=fccYV12;
	break;
    }

    avm::BitmapInfo bh(m_conf.res_w, m_conf.res_h, cs);
    if (cs <= 32)
	bh.SetDirection(1); // upside down image
    m_conf.codec.header = bh;

    printf("colorspace = %d    0x%x\n", m_conf.colorspace, cs);

    uint8_t *buf = NULL;
    //    if(m_conf.deinterlace && ((m_conf.colorspace == cspRGB24) || (m_conf.colorspace == cspYUY2)))
    if (m_conf.deinterlace)
	buf = new uint8_t[m_conf.res_w * m_conf.res_h * 3];

    const double fps=m_conf.fps;

    //m_conf.filename=find_filename(m_conf.filename);
    if(m_conf.never_overwrite)
	usedname=find_filename(m_conf.filename);
    else
	usedname=m_conf.filename;

    try
    {
	if (m_conf.segment_size == -1)
	  //file = avm::CreateWriteFile(m_conf.filename.c_str());
	    file = avm::CreateWriteFile(usedname.c_str());
	else
	{
	  //sfile = avm::CreateWriteFile(m_conf.filename.c_str(), m_conf.segment_size*1024LL);
    	    sfile = avm::CreateWriteFile(usedname.c_str(), m_conf.segment_size*1024LL);
	    file = sfile;
	}
	//	FILE* zz=fopen("bin/uncompr.bmp", "rb");
	stream = file->AddVideoStream(&m_conf.codec, int(1000000./m_conf.fps));
//	stream=file->AddStream(AviStream::Video);
//	ve.Init(fccIV50, (const char*)&bh);
    }
    catch (FatalError& e)
    {
	e.Print();
	error=e.GetDesc();
	m_quit=1;
	return 0;
    }

    float abps=(m_conf.samplesize*m_conf.frequency*m_conf.chan)/8;

    WAVEFORMATEX wfm;
    wfm.wFormatTag=m_conf.audio_compressor;//PCM or MP3
    wfm.nChannels=m_conf.chan;
    wfm.nSamplesPerSec=m_conf.frequency;
    //wfm.nSamplesPerSec=frequency * chan;  CHECK THIS
    wfm.nAvgBytesPerSec=(int)abps;
    wfm.nBlockAlign=(m_conf.samplesize*m_conf.chan)/8;
    wfm.wBitsPerSample=m_conf.samplesize;
    wfm.cbSize=0;

//    ve.SetQuality(9500);
//    ve.Start();
    stream->SetQuality(m_conf.codec.quality);
    stream->Start();
    fprintf(stderr, "Entering loop\n");
//    BITMAPINFOHEADER obh=ve.GetOutputFormat();
//    stream->SetFormat((const char*)&obh, sizeof obh);
    int cnt=0;
    int64_t audiodata=0LL;
    int videodata=0;
    double video_error=0;
    int hide_video=0;
    int dup_video=0;
    double aud_time = 0., vid_time = 0.;
    comp_drop=0;
    for (;;)
    {
	//printf("SIZE v: %d  a: %d   %d    %f  %f\n", m_qvid.size(), m_qaud.size(), m_pAllocator->get_limit(), vid_time, aud_time);
	while (m_qvid.size() > m_qvid.capacity() - 2)
	{
	    chunk& ch = m_qvid.front();
	    vid_time = ch.timestamp;
	    cnt++;
	    if(ch.data)
	    {
		m_pAllocator->free(ch.data);
                ch.data = 0;
	    }
	    stream->AddFrame(0);
	    videodata++;
	    comp_drop++;

	    if(m_pProgress)
		m_pProgress->update(longcount()-starttime,
				    vid_time, aud_time,
				    audiodata, file->GetFileSize(),
				    videodata-cnt, 0, m_pAllocator->used_frames,
				    cap_drop, comp_drop, file->GetFileName());
	    m_qvid.pop();
	}
	if ((m_qvid.size()==0) && (m_qaud.size()==0))
	{
	    if (m_quit)
                break;
	    avm_usleep(10000);
            continue;
	}
	if (m_qaud.size())
	{
	    if (!audioStream && !audioCompStream)
	    {
		if(m_conf.audio_compressor == 0x55)
		{
		    audioCompStream = file->AddAudioStream(m_conf.audio_compressor,
							   &wfm,
							   m_conf.audio_bitrate*1000/8
							  );
		    audioCompStream->Start();
		}
		else
		{
		    audioStream=file->AddStream(avm::IStream::Audio,
						&wfm, sizeof(wfm),
						1, //uncompressed PCM data
						(int)abps, //bytes/sec
						(m_conf.samplesize*m_conf.chan)/8
						//bytes/sample
					       );
		}
	    }
	    chunk& ch = m_qaud.front();
	    aud_time = ch.timestamp;
	    audiodata += ch.size;
	    if(m_conf.audio_compressor == 0x55){
		audioCompStream->AddData(ch.data, ch.size);
	    }else{
		audioStream->AddChunk(ch.data, ch.size, audioStream->KEYFRAME);
	    }
	    double audio_error=audiodata/abps-ch.timestamp;
	    if(m_pProgress){
		m_pProgress->update(longcount()-starttime, vid_time, aud_time,
				    audiodata, file->GetFileSize(), videodata-cnt,
				    -1, m_pAllocator->used_frames, cap_drop,
				    comp_drop, file->GetFileName());
		m_pProgress->setAudioError(audio_error);
	    }
	    //double audio_error=audiodata/abps-ch.timestamp;
	    if(audio_error<video_error-5./fps){
		hide_video=1;
#ifdef AVICAP_PARANOIA1
		ParanoiaLog("hiding frame\n");
#endif
	    }
	    if(audio_error>video_error+5./fps){
		dup_video=1;
#ifdef AVICAP_PARANOIA1
		ParanoiaLog("duplicating frame\n");
#endif
	    }
	    delete[] ch.data;
            ch.data = 0;
	    m_qaud.pop();
	}
	if(m_qvid.size())
	{
	    chunk& ch = m_qvid.front();
	    vid_time = ch.timestamp;
	    if (m_qaud.size() && vid_time < m_qaud.front().timestamp)
                continue;
	    cnt++;
	    if(!hide_video)
	    {
		videodata++;
		avm::CImage* im = 0;
		uint_t uiSize;
		int iKeyframe;
		if (ch.data) {
		    if (buf) {
			if (m_conf.colorspace == cspRGB24)
			    copy_deinterlace_24(buf, ch.data, 3* m_conf.res_w, m_conf.res_h);
			else if(m_conf.colorspace == cspYUY2)
			    deinterlace_bob_yuv_mmx(buf, ch.data, 2 * m_conf.res_w, m_conf.res_h);
			else
			    fprintf(stderr, "wrong colorspace\n");

			im = new avm::CImage(&bh, buf, false);
		    }else
			im = new avm::CImage(&bh, (unsigned char*)ch.data, false);
		}
		int result = stream->AddFrame(im, &uiSize, &iKeyframe);
		//printf("ADDFRAME  %d   %d %d\n", cnt, uiSize, iKeyframe);
		// fixme - handle errors
		if (result == 0)
		{
		    uiSize &= ~0x40000000;
		    if (iKeyframe)
			uiSize |= 0x40000000;
		}
		if(m_pProgress)
		    m_pProgress->update(longcount()-starttime, vid_time, aud_time,
					audiodata, file->GetFileSize(), videodata-cnt,
					uiSize, m_pAllocator->used_frames, cap_drop,
					comp_drop, file->GetFileName());
		if (dup_video)
		{
		    videodata++;
		    stream->AddFrame(im, &uiSize);
		    if(m_pProgress)
			m_pProgress->update(longcount()-starttime, vid_time, aud_time,
					    audiodata, file->GetFileSize(), videodata-cnt,
					    uiSize, m_pAllocator->used_frames, cap_drop,
					    comp_drop, file->GetFileName());
		    video_error+=1./fps;
		}
		if (im)
		    im->Release();
	    }
	    else
		video_error-=1./fps;
	    dup_video=hide_video=0;
	    if (m_pProgress)
	      m_pProgress->setVideoError(video_error);

	    if(ch.data)
	    {
		m_pAllocator->free(ch.data);
                ch.data = 0;
	    }
	    m_qvid.pop();
	}
	if (segment_flag && sfile)
	{
	    segment_flag = 0;
	    sfile->SegmentAtKeyframe();
	    //	    vid_clear=aud_clear=0;
	}
	if (segmenting_name!=""){
	  sfile->setSegmentName(segmenting_name);
	  segmenting_name="";
	}
	if (m_conf.timelimit!=-1
	    && (aud_time>m_conf.timelimit
		|| vid_time>m_conf.timelimit))
	    m_quit = 1;

	if (m_conf.sizelimit!=-1
	    && (file->GetFileSize() > m_conf.sizelimit*1024LL))
	    m_quit = 1;
    }

    delete[] buf;
    delete file;
    m_pAllocator->release();

    //    if(stream) delete stream;
    //    if(audioStream) delete audioStream;
    //    if(audioCompStream) delete audioCompStream;

    printf("RETURN FROM WRITER\n");
    return 0;
}

void CaptureProcess::setMessenger(CaptureProgress* pProgress)
{
     m_pProgress = pProgress;
     m_pProgress->init(m_conf.filename.c_str(), m_conf.fps,
		       m_conf.res_w, m_conf.res_h,
		       m_conf.frequency, m_conf.samplesize,
		       m_conf.chan, m_pAllocator->get_limit());
}

avm::string CaptureProcess::find_filename(avm::string checkname)
{
    FILE *checkfile = fopen(checkname.c_str(),"r");
    printf("checkfile %s\n",checkname.c_str());

    if(checkfile!=NULL){
	fclose(checkfile);
	avm::string::size_type st=checkname.find(".avi");
	if (st == avm::string::npos){
	    checkname += ".avi";
	}
	else{
	    checkname.insert(st,"_");
	}

	return find_filename(checkname);
    }
    else
	return checkname;
}
