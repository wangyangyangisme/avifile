#include "recompressor.h"
#include "filters.h"
#include "conf.h"

#include <infotypes.h>
#include <avm_cpuinfo.h>
#include <avm_creators.h>
#include <avm_except.h>
#include <utils.h>

#include <qapplication.h>
#include <qmessagebox.h>

#include <unistd.h>
#include <string.h>
#include <stdio.h>

char* g_pcProgramName = "avirecompress";

#define Debug if(0)
#define __MODULE__ "Recompress filter"

/*************************
    Work with filters.
 *************************/
RecompressFilter::RecompressFilter(const avm::VideoEncoderInfo& in)
    :info(in)
{
    ve = 0;
    vd = 0;
    comp_frame = 0;
    //info.header.biHeight = -1;
    printf("RecompressFilter: Create: 0x%x  \"%.4s\" name: %s\n",
	   in.compressor, (const char*)&in.compressor, info.cname.c_str());    //lazy init - in process
}

RecompressFilter::~RecompressFilter()
{
    Debug printf("RecompressFilter::~RecompressFilter()\n");
    avm::FreeEncoderVideo(ve);
    avm::FreeDecoderVideo(vd);
    delete[] comp_frame;
}

avm::CImage* RecompressFilter::process(avm::CImage* im, int pos)
{
    avm::BitmapInfo bm(*(im->GetFmt()));
    //printf("***--- PROCESS  %d\n", info.header.biHeight); bm.Print(); BitmapInfo(info.header).Print();

    if (labs(bm.biHeight) != labs(info.header.biHeight)
	|| bm.biWidth != info.header.biWidth
	|| bm.biBitCount != info.header.biBitCount
	|| bm.biCompression != info.header.biCompression)
    {
	try
	{
	    //bm.biCompression = 0;
	    //bm.SetSpace(IMG_FMT_YV12);
	    bm.SetBits(24);
	    bm.SetDirection(true);

	    //avm::BitmapInfo bh1(info.header);
	    //bh1.Print();
	    info.header = bm;
	    avm::BitmapInfo bh(bm);
	    bh.Print();

	    printf("RecompressFilter: Decompress: 0x%x  \"%.4s\"  Compressor: 0x%x \"%.4s\" %s\n",
		   info.header.biCompression, (char*)&info.header.biCompression,
		   info.compressor, (char*)&info.compressor, info.cname.c_str());

	    avm::FreeDecoderVideo(vd);
            vd = 0;
	    avm::FreeEncoderVideo(ve);
	    ve = avm::CreateEncoderVideo(info);
	    if (!ve)
		throw FATAL(avm::CodecGetError().c_str());
	    ve->SetKeyFrame(1);

	    const BITMAPINFOHEADER& obh = ve->GetOutputFormat();
	    //printf("OPBEH\n"); BitmapInfo(obh).Print();
	    //vd = CreateVideoDecoder(obh, 24, 1);
	    vd = avm::CreateDecoderVideo(obh);
	    if (!vd)
		throw FATAL(avm::CodecGetError().c_str());
	    delete[] comp_frame;
	    comp_frame = new char[obh.biWidth * labs(obh.biHeight) * 4];
	    vd->Start();
        }
        catch (FatalError& e)
        {
	    avm::FreeDecoderVideo(vd);
	    avm::FreeEncoderVideo(ve);
	    delete[] comp_frame;
	    vd = 0;
	    ve = 0;
	    comp_frame = 0;
	    e.PrintAll();
	    throw;
	}
    }

    Debug printf("RecompressFilter::process() start\n");
    if(!ve || !vd)
    {
	// we tried to start encoding into this format, but failed
        im->AddRef();
	return im;
    }

    ve->Start();
    int is_keyframe;
    uint_t size = 0;

    int r = ve->EncodeFrame(im, comp_frame, &is_keyframe, &size);
    ve->Stop();
    fprintf(stderr, "Encoded to %d bytes, error %d\n", size, r);
    if (r == 0)
    {
	avm::BitmapInfo bi(vd->GetDestFmt());
	im = new avm::CImage(&bi);
	//printf("DECOMPRESS\n"); bm.Print();
	r = vd->DecodeFrame(im, comp_frame, size, 0, 16);
	//vd->Stop();
	if (r != 0)
	    fprintf(stderr, "Decoded result: %d\n", r);
	Debug printf("RecompressFilter::process() stop\n");
	return im;
    }
    im->AddRef();
    return im;
}

avm::string RecompressFilter::save()
{
    return "";
}

void RecompressFilter::load(avm::string)
{
}

//creates filter registered under id
RecKernel::RecKernel()
:_ctl(0), m_pRecFilter(0), m_pRecCb(0), m_pWriteFile(0),
 vs(0), as(0), rec_status(0), pause_status(0)
{
}

RecKernel::~RecKernel()
{
    delete m_pRecFilter;
    _destruct();
}

Filter* RecKernel::getFilter(uint_t id)
{
    switch(id)
    {
    case 0:
	return new GammaFilter(this);
    case 1:
	return new BlurFilter(this);
    case 2:
	return new RefractFilter(this);
    case 3:
	return new NoiseFilter(this);
    case 4:
	return new MoveFilter(this);
    case 5:
	return new ScaleFilter(this);
/*    case 5:
	return new SwapFilter(this);*/
    default:
	return 0;
    }

}

//returns filter at pos id in current list
Filter* RecKernel::getFilterAt(uint_t id)
{
    if (id >= m_FilterList.size())
	return 0;
    Filter* fi = m_FilterList[id];
    fi->addref();

    return fi;
}

//size of current list
unsigned int RecKernel::getFilterCount() const
{
    return m_FilterList.size();
}

//adds filter in the end of list
int RecKernel::addFilter(Filter* fi)
{
    m_FilterList.push_back(fi);
    fi->addref();
    redraw();
    return 0;
}

int RecKernel::removeFilter(uint_t id)
{
    if (id >= m_FilterList.size())
    {
	printf("removeFilter: weird id %d\n", id);
        return -1;
    }
    else if (!m_FilterList.size())
	return 0;

    Filter* fi = m_FilterList[id];
    swap(id, m_FilterList.size() - 1);
    m_FilterList.pop_back();
    delete fi;
    redraw();
    return 0;
}

int RecKernel::moveUpFilter(uint_t id)
{
    if (id >= m_FilterList.size() || id == 0)
	return 0;
    swap(id, id - 1);

    return 0;
}

int RecKernel::moveDownFilter(uint_t id)
{
    if (m_FilterList.size() < 1 || id >= (m_FilterList.size()-1))
	return 0;
    swap(id, id + 1);

    return 0;
}

/*
 *  Work with files.
 */
int RecKernel::openFile(const char* fn)
{
    if (m_ReadFiles.size())
	_destruct();
    try
    {
	avm::IReadFile* rf = avm::CreateReadFile(fn);
	if (!rf)
	    return -1;
        m_ReadFiles.push_back(rf);
	m_ReadFn = fn;
	int i;
	vs = as = 0;
	for(i = 0; i < 16; i++)
	{
	    avm::IReadStream* s = rf->GetStream(i, avm::IStream::Video);
	    if (!s)
		break;
	    /**
		IV32 has serious problems without this line
		Since we can draw upside-down pictures, who cares.
	    **/

	    s->SetDirection(true);

	    full_stream fs;
	    fs.stream = s;
	    fs.startpos = 0;
	    fs.endpos = s->GetLength();
	    fs.mode = Copy;

	    BITMAPINFOHEADER bh;
	    s->GetVideoFormat(&bh, sizeof(bh));
	    fs.vi.compressor = bh.biCompression;

	    fs.vi.quality = 9500;
	    fs.vi.keyfreq = 15;
	    avm::StreamInfo* info = s->GetStreamInfo();
	    if (info)
	    {
		fs.vi.quality = info->GetQuality();
		delete info;
	    }

	    m_Streams.push_back(fs);
	    vs++;
	}
	for (i = 0; i < 16; i++)
	{
	    avm::IReadStream* s = rf->GetStream(i, avm::IStream::Audio);
	    if (!s)
		break;
	    full_stream fs;
	    fs.stream = s;
	    fs.startpos = 0;
	    fs.endpos = s->GetLength();
	    fs.mode = Copy;

	    WAVEFORMATEX wf;
	    s->GetAudioFormat(&wf, sizeof(wf));
	    fs.ai.fmt = 1;
	    if (wf.wFormatTag == 0x55)
	    {
		fs.ai.fmt = 0x55;
		fs.ai.bitrate = wf.nAvgBytesPerSec;
	    }

	    m_Streams.push_back(fs);
	    as++;
	}
	return 0;
    }
    catch (FatalError& e)
    {
	strLastError = avm::string(e.GetModule()) + avm::string(": ") + e.GetDesc();
	return -1;
    }
}

int RecKernel::setDestFile(const char* fn)
{
    avm::IWriteFile* wf;
    try
    {
	wf = avm::CreateWriteFile(fn);
	m_Filename = fn;
	delete wf;
        wf = 0;
	return 0;
    }
    catch (FatalError& e)
    {
	e.Print();
	return -1;
    }
}

//first all video streams, then audio
avm::string RecKernel::aboutStream(uint_t id) const
{
    if (!isStream(id))
	return "";
    char s[1024];
    avm::IReadStream* stream = m_Streams[id].stream;
    if (isAudioStream(id))
    {
	WAVEFORMATEX wf;
	stream->GetAudioFormat(&wf, sizeof(wf));
	const char* enc;
        char dummy[128];
	const avm::CodecInfo* ci = avm::CodecInfo::match(wf.wFormatTag, avm::CodecInfo::Audio, 0, avm::CodecInfo::Encode);
	if (ci)
	    enc = ci->GetName();
	else
	{
	    sprintf(dummy, "ID %d (%x) (unsupported) ", wf.wFormatTag,
		    wf.wFormatTag);
	    enc = dummy;
	}
	avm::StreamInfo* info = stream->GetStreamInfo();
	if (info)
	{
	    sprintf(s, "Audio stream #%d. Average rate: %dkbps (%d bytes/s)\n"
		    "Length: %d samples (Sample size %d bytes)\n"
		    "Encoding: %s. ( %.2f seconds )",
		    (int)id - vs, ((wf.nAvgBytesPerSec * 8 + 500) / 1000),
		    wf.nAvgBytesPerSec,
		    info->GetStreamFrames(), wf.nBlockAlign,
		    enc, info->GetLengthTime());
	    delete info;
	}
	return avm::string(s);
    }
    if (isVideoStream(id))
    {
	BITMAPINFOHEADER bh;
	stream->GetVideoFormat(&bh, sizeof(bh));
	const avm::CodecInfo* ci = avm::CodecInfo::match(bh.biCompression);
	avm::string encoding;
	if (ci)
	    encoding = ci->GetName();
	else
	{
	    char dummy[128];
            char ft[4];
	    sprintf(dummy, "unsupported %X='%.4s'", bh.biCompression,
		    avm_set_le32(ft, bh.biCompression));
	    encoding = dummy;
	}
	avm::StreamInfo* info = stream->GetStreamInfo();
	if (info)
	{
	    sprintf(s, "Video stream #%d. %dx%d  %.2f fps.\n"
		    "Length: %d frames ( %.2f seconds ).\nEncoding: %s.",
		    (int)id, bh.biWidth, bh.biHeight,
		    info->GetFps(),
		    info->GetStreamFrames(),
		    info->GetLengthTime(),
		    encoding.c_str());
	    delete info;
	}
	return avm::string(s);
    }
    return avm::string("");
}

avm::string RecKernel::lastError() const
{
    return strLastError;
}

bool RecKernel::isVideoStream(uint_t id) const
{
    if (id < m_Streams.size())
	return (m_Streams[id].stream->GetType() == avm::IStream::Video);
    return false;
}
bool RecKernel::isAudioStream(uint_t id) const
{
    if (id < m_Streams.size())
	return (m_Streams[id].stream->GetType() == avm::IStream::Audio);
    return false;
}

avm::IStream::StreamType RecKernel::getStreamType(uint_t id) const
{
    if (id < m_Streams.size())
	return m_Streams[id].stream->GetType();
    return avm::IStream::Other;
}

int RecKernel::getCompress(uint_t id, avm::VideoEncoderInfo& vi) const
{
    if (!isVideoStream(id))
	return -1;
    vi = m_Streams[id].vi;
    return m_Streams[id].stream->GetOutputFormat(&vi.header, sizeof(vi.header));
}

int RecKernel::getAudioCompress(uint_t id, AudioEncoderInfo& ai) const
{
    if (!isAudioStream(id))
	return -1;
    ai = m_Streams[id-vs].ai;
    return 0;
}

int RecKernel::setCompress(uint_t id, const avm::VideoEncoderInfo& vi)
{
    if (!isVideoStream(id))
	return -1;
    m_Streams[id].vi = vi;
    try
    {
	avm::VideoEncoderInfo vinfo;
	RecompressFilter* nrecf = (getCompress(0, vinfo) < 0)
	    ? 0 : new RecompressFilter(vi);
	if (nrecf)
	{
	    delete m_pRecFilter;
	    m_pRecFilter = nrecf;
	}
    }
    catch (FatalError& e)
    {
	e.Print();
    }
    redraw();
    return 0;
}

int RecKernel::setAudioCompress(uint_t id, const AudioEncoderInfo& ai)
{
    if (!isAudioStream(id))
	return -1;
    m_Streams[id - vs].ai = ai;
    return 0;
}

//nonzero if stream
bool RecKernel::isStream(uint_t id) const
{
    return (id < m_Streams.size());
}

int RecKernel::setStreamMode(uint_t id, KernelMode mode)
{
    if (!isStream(id))
	return -1;
    m_Streams[id].mode = mode;
    redraw();
    return 0;
}

int RecKernel::getStreamMode(uint_t id, KernelMode& mode) const
{
    if (!isStream(id))
	return -1;
    mode = m_Streams[id].mode;
    return 0;
}

int RecKernel::getSelection(uint_t id, framepos_t& start, framepos_t& end) const
{
    if (!isStream(id))
	return -1;
    start = m_Streams[id].startpos;
    end = m_Streams[id].endpos;
    return 0;
}

int RecKernel::setSelection(uint_t id, framepos_t start, framepos_t end)
{
    if (!isStream(id))
	return -1;
    m_Streams[id].startpos = start;
    m_Streams[id].endpos = end;
    return 0;
}

int RecKernel::setSelectionStart(uint_t id, framepos_t start)
{
    if (!isStream(id))
	return -1;
    m_Streams[id].startpos = start;
    return 0;
}

int RecKernel::setSelectionEnd(uint_t id, framepos_t end)
{
    if (!isStream(id))
	return -1;
    m_Streams[id].endpos = end;
    return 0;
}

double RecKernel::getFrameTime(uint_t id) const
{
    if (!isStream(id))
	return 0;
    return m_Streams[id].stream->GetFrameTime();
}

double RecKernel::getTime(uint_t id) const
{
    if (!isStream(id))
	return 0;

    return m_Streams[id].stream->GetTime();
}

//should set size & pictures if ctl!=0 & file is opened
void RecKernel::setImageControl(IImageControl* ctl)
{
    if (vs)
    {
	try
	{
	    m_Streams[0].stream->StopStreaming();
	    m_Streams[0].stream->StartStreaming();
	    m_Streams[0].stream->Seek((framepos_t) 0);
	    BITMAPINFOHEADER bh;
	    m_Streams[0].stream->GetVideoFormat(&bh, sizeof(bh));
	    _ctl = ctl;
	    _ctl->setSize(bh.biWidth, bh.biHeight);

	    delete m_pRecFilter;
	    avm::VideoEncoderInfo info;

	    m_pRecFilter = (getCompress(0, info) < 0)
		? 0 : new RecompressFilter(info);

	    redraw(true);
	}
	catch (FatalError& e)
	{
	    e.Print();
	}
    }
    return;
}

avm::string RecKernel::getStreamName(uint_t id) const
{
    avm::string s;

    if (isStream(id))
    {
	if (isAudioStream(id))
	{
	    s = "Audio";
	    id -= vs;
	}
	else
	    s = "Video";
	char q[64];
	sprintf(q, " #%d", id);
	s += q;
    }
    return s;
}

unsigned int RecKernel::getStreamCount() const
{
    return m_Streams.size();
}

//in frames
//negative seeks are very slow
// FIXME: implement frame caching : store frame every second

framepos_t RecKernel::seek(int delta)
{
    if (!vs)
	return 0;

    avm::IReadStream* stream = m_Streams[0].stream;

    framepos_t cur_pos = stream->GetPos();
    framepos_t endpos = stream->GetLength();
    framepos_t newpos = cur_pos + delta;

    if (delta < 0 && cur_pos < (unsigned) -delta)
	newpos = 0;

    if (newpos >= endpos)
	newpos = endpos;

    if (newpos == cur_pos)
	return cur_pos;

    framepos_t next_kern = stream->GetPrevKeyFrame(newpos);

    //cout << "pos:" << cur_pos << " prev_keyfr:" << next_kern << " d:" << delta << " newpos:" << newpos << endl;

    if (delta < 0 || next_kern > cur_pos)
	stream->Seek(next_kern);

    framepos_t lpos = ~0U, prevp = ~0U;
    int i = 5;
    while (i > 0) // avoid deadlock if can't seek
    {
	lpos = stream->GetPos();
	//printf("pos: %d  newpos: %d    %d  %d  del:%d\n", lpos, newpos, prevp, cur_pos, delta);
	if ((lpos + 1) >= newpos || lpos == stream->ERR)
	    break;

	stream->ReadFrame(false);

	// some error could happen - prevent deadlock loop
	// we are checking if the position is changing
	if (prevp == lpos)
            i--;
	prevp = lpos;
    }
    redraw(true);
    return lpos;
}

//seek to nearest keyframe, return its position
framepos_t RecKernel::seekPos(framepos_t fpos)
{
    if (!vs)
	return fpos;
    if (fpos >= m_Streams[0].stream->GetLength())
        fpos = m_Streams[0].stream->GetLength() - 1;

    framepos_t r = m_Streams[0].stream->SeekToKeyFrame(fpos);
    redraw(true);

    return m_Streams[0].stream->GetPos();
/*    if(m_Streams.size()==0)return fpos;
    if(vs==0)
    {
	for(int i=0; i<m_Streams.size(); i++)
	    m_Streams[i].stream->Seek(fpos);
	return fpos;
    }
    else
    {
	fpos=m_Streams[0].stream->SeekToKeyframe(fpos);
	for(int i=1; i<m_Streams.size(); i++)
	    m_Streams[i].stream->Seek(fpos);
	redraw();
	return fpos;
    }
*/
}

framepos_t RecKernel::seekNextKeyFrame()
{
    if (!vs)
	return 0;

    avm::IReadStream* stream = m_Streams[0].stream;
    framepos_t op = pos();

    //cout << "POS1 " << pos() << endl;
    if (stream->SeekToNextKeyFrame() != stream->ERR)
	redraw(true);
    return pos();
}

framepos_t RecKernel::seekPrevKeyFrame()
{
    if (!vs)
	return 0;

    avm::IReadStream* stream = m_Streams[0].stream;
    framepos_t op = pos();

    if (stream->SeekToPrevKeyFrame() != stream->ERR)
	redraw(true);

    return pos();
}

framepos_t RecKernel::pos() const
{
    return (vs) ? m_Streams[0].stream->GetPos() : 0;
}

unsigned int RecKernel::getVideoLength() const
{
    if (!vs)
	return 0;
    return (unsigned int) m_Streams[0].stream->GetLength();
}

/***********************************

	Actual recompression

************************************/

int RecKernel::setCallback(IRecompressCallback* pRecCallback)
{
    m_pRecCb = pRecCallback;
    return 0;
}

void RecKernel::_destruct()
{
    //cout << "RecKernel::_destruct()" << endl;
    m_Streams.clear();
    while (m_ReadFiles.size())
    {
	delete m_ReadFiles.back();
        m_ReadFiles.pop_back();
    }
    delete m_pWriteFile;
    m_pWriteFile = 0;
}

void RecKernel::redraw(bool readFrame)
{
    if (!vs || !_ctl)
	return;

    avm::CImage* pix = m_Streams[0].stream->GetFrame(readFrame);

    if (!pix)
    {
	///printf("ERROR: zero frame\n");
	return;
    }

    _ctl->setSourcePicture(pix);

    avm::CImage* im = pix;
    if (m_pRecFilter)
    {
	KernelMode m;
	if (getStreamMode(0, m) == 0)
	{
	    // show compressed image only when recompress is enabled
	    if (m == RecKernel::Recompress)
	    {
		im = new avm::CImage(pix);
		for (unsigned i = 0; i < getFilterCount(); i++)
		{
		    Filter* fi = getFilterAt(i);
		    avm::CImage* new_im = fi->process(im, m_Streams[0].stream->GetPos());
		    im->Release();
		    im = new_im;
		}
		im->ToRGB();
		try
		{
		    avm::CImage* new_im = m_pRecFilter->process(im, 0);
		    if (new_im)
		    {
			im->Release();
			im = new_im;
		    }
		}
		catch (FatalError& e)
		{
		    QMessageBox::information(0, "Error", "Cannot recompress into this format");
		}
	    }
	}
    }

    //printf("BEFORE SETDEST\n"); im->GetFmt()->Print();
    _ctl->setDestPicture(im);
    im->Release();
    if (pix != im)
	pix->Release();
}

int RecKernel::loadConfig(const char* fn)
{
    ReadConfig cfg(fn);
//    cfg.Add("SrcFile", m_ReadFn);
    avm::string src = cfg.getString("SrcFile");
    printf("src: %s\n", src.c_str());
    openFile(src.c_str());
    avm::string dest = cfg.getString("DestFile");
    setDestFile(dest.c_str());

//    cfg.Add("VideoStreams", vs);
//    cfg.Add("AudioStreams", as);
    char s[128];
    for (unsigned i=0; i<m_Streams.size(); i++)
    {
	sprintf(s, "Stream %d start", i);
	m_Streams[i].startpos = cfg.getInt(s);
	sprintf(s, "Stream %d end", i);
	m_Streams[i].endpos = cfg.getInt(s);
	sprintf(s, "Stream %d mode", i);
	m_Streams[i].mode = (KernelMode)cfg.getInt(s);
	if (i < vs)
	{
	    sprintf(s, "Stream %d quality", i);
	    m_Streams[i].vi.quality = cfg.getInt(s);
	    sprintf(s, "Stream %d keyfreq", i);
	    m_Streams[i].vi.keyfreq = cfg.getInt(s);
	    sprintf(s, "Stream %d compressor", i);
	    m_Streams[i].vi.compressor = cfg.getInt(s);
	    sprintf(s, "Stream %d compressor name", i);
	    m_Streams[i].vi.cname = cfg.getString(s);
//	    m_Streams[i].stream->GetOutputFormat(&m_VideoFmt[i].header, sizeof m_VideoFmt[i].header);
	}
	else
	{
	    sprintf(s, "Stream %d fmt", i);
	    m_Streams[i - vs].ai.fmt = cfg.getInt(s);
	    sprintf(s, "Stream %d bitrate", i);
	    m_Streams[i - vs].ai.bitrate = cfg.getInt(s);
	}
    }
    int filters = cfg.getInt("Filters");
//    cfg.Add("Filters", m_FilterList.size());
    for(int i=0; i<filters; i++)
    {
	sprintf(s, "Filter %d", i);
	Filter* f = getFilter(cfg.getInt(s));
	if(!f)break;
	sprintf(s, "Filter %d info", i);
	f->load(cfg.getString(s));
	addFilter(f);
    }
    return 0;
}

int RecKernel::saveConfig(const char* fn)
{
    if (!m_ReadFiles.size())
	return 0;
    WriteConfig cfg(fn);
    cfg.add("SrcFile", m_ReadFn);
    cfg.add("DestFile", m_Filename);
    cfg.add("VideoStreams", (int)vs);
    cfg.add("AudioStreams", (int)as);

    char s[128];
    for (unsigned i = 0; i < m_Streams.size(); i++)
    {
	sprintf(s, "Stream %d start", i);
	cfg.add(s, (int)m_Streams[i].startpos);
	sprintf(s, "Stream %d end", i);
	cfg.add(s, (int)m_Streams[i].endpos);
	sprintf(s, "Stream %d mode", i);
	cfg.add(s, (int)m_Streams[i].mode);
	if(i < vs)
	{
	    sprintf(s, "Stream %d quality", i);
	    cfg.add(s, m_Streams[i].vi.quality);
	    sprintf(s, "Stream %d keyfreq", i);
	    cfg.add(s, m_Streams[i].vi.keyfreq);
	    sprintf(s, "Stream %d compressor", i);
	    cfg.add(s, (int)m_Streams[i].vi.compressor);
	    sprintf(s, "Stream %d compressor name", i);
	    cfg.add(s, m_Streams[i].vi.cname.c_str());
	}
	else
	{
	    sprintf(s, "Stream %d fmt", i);
	    cfg.add(s, m_Streams[i-vs].ai.fmt);
	    sprintf(s, "Stream %d bitrate", i);
	    cfg.add(s, m_Streams[i-vs].ai.bitrate);
	}
    }
    cfg.add("Filters", (int)m_FilterList.size());
    for (unsigned i = 0; i < m_FilterList.size(); i++)
    {
	sprintf(s, "Filter %d", i);
	cfg.add(s, m_FilterList[i]->id());
	sprintf(s, "Filter %d info", i);
	avm::string save=m_FilterList[i]->save();
	cfg.add(s, save);
    }
    return 0;
}
