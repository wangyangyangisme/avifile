/********************************************************

	AviWriteStream/AviWriteFile class implementation
	Copyright 2000 Eugene Kuznetsov  (divx@euro.ru)

*********************************************************/

#define _LARGEFILE64_SOURCE
#include "AviWrite.h"
#include "audioencoder.h"
#include "avm_output.h"
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#if defined(__FreeBSD__) || defined(__NetBSD__)
#define lseek64 lseek
#define O_LARGEFILE 0
#endif


/*
audiopreloadtime; (ms)
audiopreloadsize; bytes


avm::vector<char*> x;


audio/video add_buffer()
{
    if (size + chunks > XX)
	syncfile();

    if (iskeyframe)



}


avi flush_buffer()
{
    lastvideo()
    while ()
    {
	audiochunk += audiochunksize;
	if (audiochunk > limit)
	{
	    write();


	}
    }


    addindex()
        write()
}

*/

AVM_BEGIN_NAMESPACE;

#define __MODULE__ "WriteFile"

IWriteFile::~IWriteFile() {}

AviWriteStream::AviWriteStream(AviWriteFile* file, int ckid,
			       IStream::StreamType type,
			       fourcc_t handler, int frame_rate,
			       int flags,
			       const void* format, uint_t format_size,
			       uint_t samplesize, int quality)
    :m_pFile(file), m_pcFormat(0), m_uiLength(0),  m_ckid(ckid), m_stop(0)
{
    m_type = type;
    memset(&m_Header, 0, sizeof(m_Header));
    switch (type)
    {
    case Video:
	if (format)
	{
	    const BITMAPINFOHEADER* bh = (const BITMAPINFOHEADER*) format;
	    m_Header.rcFrame.right = bh->biWidth;
	    m_Header.rcFrame.bottom = bh->biHeight;
	}
	m_Header.dwRate = 1000000;
	m_Header.dwScale = frame_rate;
	m_Header.fccType = streamtypeVIDEO;
	break;
    case Audio:
        m_Header.dwRate = frame_rate;
	printf("frame rate for audio: %d\n",frame_rate);

	m_Header.dwScale = format ?
	    ((const WAVEFORMATEX*) format)->nBlockAlign : samplesize;
	m_Header.fccType = streamtypeAUDIO;
	break;
    default:
	throw FATAL("Unsupported stream type");
    }

    if (format && format_size > 0)
    {
	m_pcFormat = new char[format_size];
	m_uiFormatSize = format_size;
	memcpy(m_pcFormat, format, format_size);
    }

    m_Header.fccHandler = handler;
    m_Header.dwFlags = flags;
    m_Header.dwLength = 0;
    m_Header.dwSampleSize = samplesize;
    m_Header.dwQuality = quality;
}

AviWriteStream::~AviWriteStream()
{
    delete[] m_pcFormat;
}

int AviWriteStream::AddChunk(const void* chunk, uint_t size, int flags)
{
    if (!chunk && size)
    {
	AVM_WRITE("AVI writer", "Invalid argument to AviWriteStream::AddChunk()\n");
	return -1;
    }

    m_pFile->AddChunk(chunk, size, m_ckid, flags);
    //AVM_WRITE("AVI writer", "Addchunk  %d   %d\n", m_Header.dwSampleSize, size);
    if (m_Header.dwSampleSize)
    {
        m_uiLength += size;
	m_Header.dwLength = m_uiLength / m_Header.dwSampleSize;
    }
    else
	m_Header.dwLength++;

    return 0;
}


/*
 *
 *  WriteFile
 *
 */
AviWriteFile::AviWriteFile(const char* name, int64_t limit, int flags, int mask)
    :m_pFileBuffer(0), m_lFlimit(limit), m_Filename(name), m_iFlags(flags),
    m_iMask(mask)
{
    init();
    //AVM_WRITE("AVI writer", "AviWriteFile::AviWriteFile()\n");
}

AviWriteFile::~AviWriteFile()
{
    //AVM_WRITE("AVI writer", "AviWriteFile::~AviWriteFile()\n");
    if (!m_iStatus)
	return;
    try
    {
	// Stop encoders on all opened streams so it may flush
	// all buffered data
	for (unsigned i = 0; i < m_Streams.size(); i++)
	{
	    if (m_Streams[i]->m_stop)
	    {
		switch (m_Streams[i]->GetType())
		{
		case AviWriteStream::Video:
		    ((AviVideoWriteStream*)m_Streams[i])->Stop();
		    break;
		case AviWriteStream::Audio:
		    ((AviAudioWriteStream*)m_Streams[i])->Stop();
		    break;
		default:
		    break;
		}
	    }
	}
	finish();
    }
    catch (...) {}

    for (unsigned i = 0; i < m_Streams.size(); i++)
	delete m_Streams[i];
    m_Streams.clear();
}

void AviWriteFile::init()
{
  segment_flag=false;
  segmenting_name="";

    m_iStatus = 0;
    memset(&m_Header, 0, sizeof(m_Header));
    m_Header.dwFlags = m_iFlags;
    m_Index.clear();
    for (unsigned i = 0; i < m_Streams.size(); i++)
    {
	m_Streams[i]->m_Header.dwLength = 0;
	m_Streams[i]->Start();
    }

    m_pFileBuffer = new FileBuffer(m_Filename.c_str(),
				   O_WRONLY | O_CREAT | O_TRUNC | O_LARGEFILE,
				   m_iMask);
}

void AviWriteFile::finish()
{
    uint_t videoendpos = m_pFileBuffer->lseek(0, SEEK_CUR);
    if (videoendpos & 1)
    {
        // padding
	int junk = -1;
	m_pFileBuffer->write(&junk, 1);
	videoendpos++;
    }

    WriteHeaders();

    write_le32(ckidAVINEWINDEX);
    write_le32(m_Index.size() * sizeof(AVIINDEXENTRY));
    m_pFileBuffer->write(&m_Index[0], m_Index.size() * sizeof(AVIINDEXENTRY));
    //printf("INDEX SIZE %d\n", m_Index.size()*sizeof(AVIINDEXENTRY));
    videoendpos=m_pFileBuffer->lseek(0, SEEK_END);
    m_pFileBuffer->lseek(4, SEEK_SET);
    write_le32(videoendpos-8);

    delete m_pFileBuffer;
    m_pFileBuffer = 0;

    for (unsigned i = 0; i < m_Streams.size(); i++)
	m_Streams[i]->Stop();
}

IWriteStream* AviWriteFile::AddStream(IStream::StreamType type,
				      const void* format,
				      uint_t format_size,
				      fourcc_t handler, int frame_rate,
				      uint_t samplesize, int quality,
				      int flags)
{
    int ckid = MAKEAVICKID((type==AviWriteStream::Video)
			   ? cktypeDIBcompressed : cktypeWAVEbytes,
			   m_Streams.size());

    AviWriteStream* r =
	new AviWriteStream(this, ckid, type, handler, frame_rate, flags,
			   format, format_size,
			   samplesize, quality);
    m_Streams.push_back(r);
    return r;
}

IWriteStream* AviWriteFile::AddStream(IReadStream* pStream)
{
    AviWriteStream* r = 0;
    IStream::StreamType type = pStream->GetType();
    uint_t format_size = 0;
    char* format = 0;
    int flags = 0;
    int frame_rate;
    int ckid;

    StreamInfo* si = pStream->GetStreamInfo();
    uint_t samplesize = si->GetSampleSize();
    int quality = si->GetQuality();
    fourcc_t handler = si->GetFormat();

    switch (type)
    {
    case IStream::Video:
	format_size = pStream->GetVideoFormat();
	if (!format_size)
	    goto end;
	format = new char[format_size];
	pStream->GetVideoFormat(format, format_size);
        ckid = MAKEAVICKID(cktypeDIBcompressed, m_Streams.size());
	frame_rate = (unsigned int)(1000000.*pStream->GetFrameTime());
	break;
    case IStream::Audio:
        format_size = pStream->GetAudioFormat();
	if (!format_size)
	    goto end;
	format = new char[format_size];
	pStream->GetAudioFormat(format, format_size);
	ckid = MAKEAVICKID(cktypeWAVEbytes, m_Streams.size());
	frame_rate = si->GetAudioSamplesPerSec();
	break;
    default:
        return 0;
    }

    r = new AviWriteStream(this, ckid, type, handler, frame_rate, flags,
			   format, format_size,
			   samplesize, quality);
    delete[] format;
    m_Streams.push_back(r);
end:
    delete si;
    return r;
}

IVideoWriteStream* AviWriteFile::AddVideoStream(const CodecInfo& ci,
						const BITMAPINFOHEADER* srchdr,
						int frame_rate, int flags)
{
    int ckid = MAKEAVICKID(cktypeDIBcompressed, m_Streams.size());

    AviVideoWriteStream* r =
	new AviVideoWriteStream(this, ckid, ci, srchdr,
				frame_rate, flags);
    m_Streams.push_back(r);
    return r;
}

IVideoWriteStream* AviWriteFile::AddVideoStream(fourcc_t fourcc,
						const BITMAPINFOHEADER* srchdr,
						int frame_rate, int flags)
{
    const CodecInfo* pci = CodecInfo::match(fourcc, CodecInfo::Video,
					    0, CodecInfo::Encode);
    if (!pci)
	throw FATAL("No known video codecs for this fourcc");
    return AddVideoStream(*pci, srchdr, frame_rate, flags);
}

IVideoWriteStream* AviWriteFile::AddVideoStream(const VideoEncoderInfo* vi,
						int frame_rate, int flags)
{
    const CodecInfo* pci = CodecInfo::match(CodecInfo::Video, vi->cname);
    if (!pci)
	throw FATAL("No known video codecs for this VideoEncoderInfo");
    return AddVideoStream(*pci, &vi->header, frame_rate, flags);
}

IAudioWriteStream* AviWriteFile::AddAudioStream(const CodecInfo& ci,
						const WAVEFORMATEX* fmt,
						int bitrate, int flags)
{
    int ckid = MAKEAVICKID(cktypeWAVEbytes, m_Streams.size());

    AviAudioWriteStream* r =
	new AviAudioWriteStream(this, ckid, ci, fmt, bitrate, flags);

    m_Streams.push_back(r);
    return r;
}

IAudioWriteStream* AviWriteFile::AddAudioStream(fourcc_t fourcc,
						const WAVEFORMATEX* fmt,
						int bitrate, int flags)
{
    const CodecInfo* pci = CodecInfo::match(fourcc, CodecInfo::Audio,
					    0, CodecInfo::Encode);
    if (!pci)
	throw FATAL("No known audio codecs for this fourcc");
    return AddAudioStream(*pci, fmt, bitrate, flags);
}


void AviWriteFile::AddChunk(const void* chunk, uint_t size, uint_t ckid, int flags)
{
    if ( (m_lFlimit && GetFileSize() > m_lFlimit) || segment_flag)
    {
	//AVM_WRITE("AVI writer", "Size %lld   limit %lld\n", GetFileSize(), m_lFlimit);

        if( ( ((ckid  >> 16) & 0xffff) == ('c' << 8 | 'd') ) && flags){
          printf("segmenting\n");
          Segment();
        }
    }

    if (m_Index.size() == 0)
    {
	const int junk_size = 0x800;
	char* junk = new char[junk_size];
	memset(junk, 0, junk_size);
	m_pFileBuffer->write(junk, junk_size);
	delete[] junk;
	m_iStatus = 1;
    }

    uint_t offset = m_pFileBuffer->lseek(0, SEEK_CUR);

    // let's make some fun
    if (offset > 0xffff0000) throw FATAL("Unsupported AVI file size!");

    write_le32(ckid);
    write_le32(size);
    if (chunk)
    {
	m_pFileBuffer->write(chunk, size);
	if (size & 1)
	    m_pFileBuffer->write(chunk, 1);
    }

    //AVM_WRITE("AVI writer", "CKID 0x%x  %5d   %8d   %8d %d\n", ckid, flags, size, m_lFlimit, GetFileSize());
    AVIINDEXENTRY entry;
    entry.ckid = ckid;
    entry.dwFlags = flags;
    entry.dwChunkOffset = offset - 0x7fc;
    entry.dwChunkLength = size;
    m_Index.push_back(entry);
    if (m_Index.size() % 1000 == 1)
	WriteHeaders();
}

int64_t AviWriteFile::GetFileSize() const
{
    return m_pFileBuffer->lseek(0, SEEK_CUR);
}

int AviWriteFile::Reserve(uint_t size)
{
    return -1;
}

int AviWriteFile::WriteChunk(fourcc_t fourcc, void* data, uint_t size)
{
    return -1;
}

void AviWriteFile::setSegmentName(avm::string new_name)
{
  segmenting_name=new_name;
}

void AviWriteFile::SegmentAtKeyframe()
{
  segment_flag=true;
}

int AviWriteFile::Segment()
{
  avm::string newfilename;

  if(segmenting_name==""){
     newfilename=m_Filename;
  }
  else{
    newfilename=segmenting_name;
  }

    if (newfilename.size() < 6)
	newfilename = avm::string("_____")+newfilename;

    avm::string::size_type st = newfilename.find(".avi");
    if (st == avm::string::npos)
	newfilename += ".000.avi";
    else
    {
	if (newfilename[st-4]=='.')
	{
	    char* newval=&newfilename[st-3];
	    int i = atoi(newval) + 1;
	    if (i >= 1000)
		i = 0;
	    char s[4];
	    sprintf(s, "%03d", i);
	    memcpy(newval, s, 3);
	}
	else
	    newfilename.insert(st, ".000");
    }
    segment_flag=false;
    segmenting_name="";
    finish();

    m_Filename=newfilename;

    init();
    return 0;
}

void AviWriteFile::WriteHeaders()
{
    //printf("**** Writing headers ****  %d\n", m_iStatus);;
    if (m_iStatus == 0)
	return;

    uint_t endpos = m_pFileBuffer->lseek(0, SEEK_END);
    if (m_pFileBuffer->lseek(0, SEEK_SET) != 0)
	return;

    m_Header.dwFlags |= AVIF_HASINDEX | AVIF_TRUSTCKTYPE;
    m_Header.dwPaddingGranularity = 0;
    m_Header.dwStreams = m_Streams.size();
    m_Header.dwTotalFrames = 0;
    for (unsigned i = 0; i < m_Streams.size(); i++)
    {
	if (m_Streams[i]->GetType() == AviWriteStream::Video)
	{
	    m_Header.dwTotalFrames = m_Streams[i]->GetLength();
	    m_Header.dwMicroSecPerFrame = m_Header.dwScale =  m_Streams[i]->m_Header.dwScale;
	    m_Header.dwWidth = m_Streams[i]->m_Header.rcFrame.right;
	    m_Header.dwHeight = labs(m_Streams[i]->m_Header.rcFrame.bottom);
            m_Header.dwRate = m_Streams[i]->m_Header.dwRate;
            m_Header.dwStart = m_Streams[i]->m_Header.dwStart;
	    printf("header: video stream %d mspframe=%d rate=%d start=%d\n",i,m_Header.dwMicroSecPerFrame,m_Header.dwRate,m_Header.dwStart);
	    break;
	}
    }
    if (m_Header.dwTotalFrames == 0)
	if (m_Streams.size())
	{
	    m_Header.dwTotalFrames = m_Streams[0]->GetLength();
	    m_Header.dwWidth = m_Header.dwHeight = 0;
	}

    write_le32(FOURCC_RIFF);
    write_le32(endpos - 8);
    write_le32(formtypeAVI);// Here goes chunk with all headers
    write_le32(FOURCC_LIST);

    // header pos = 0x10
    write_le32(0);//here header chunk size hdr_size will be
    int hdr_size = 12 + sizeof(AVIMainHeader);

    // Write AVIMainHeader
    write_le32(listtypeAVIHEADER);
    write_le32(ckidAVIMAINHDR);
    write_le32(sizeof(AVIMainHeader));

    write_le32(m_Header.dwMicroSecPerFrame);
    write_le32(m_Header.dwMaxBytesPerSec);
    write_le32(m_Header.dwPaddingGranularity);
    write_le32(m_Header.dwFlags);
    write_le32(m_Header.dwTotalFrames);
    write_le32(m_Header.dwInitialFrames);
    write_le32(m_Header.dwStreams);
    write_le32(m_Header.dwSuggestedBufferSize);
    write_le32(m_Header.dwWidth);
    write_le32(m_Header.dwHeight);

    write_le32(m_Header.dwScale);
    write_le32(m_Header.dwRate);
    write_le32(m_Header.dwStart);
    write_le32(m_Header.dwLength);

    for (unsigned j = 0; j < m_Streams.size(); j++)
    {
	int s = sizeof(AVIStreamHeader) + m_Streams[j]->m_uiFormatSize
	    + (m_Streams[j]->m_uiFormatSize & 1);
	hdr_size += 28 + s;
	write_le32(FOURCC_LIST);
	write_le32(20 + s);
        write_le32(listtypeSTREAMHEADER);
	write_le32(ckidSTREAMHEADER);
        write_le32(sizeof(AVIStreamHeader));
	m_pFileBuffer->write(&m_Streams[j]->m_Header, sizeof(AVIStreamHeader));

	write_le32(ckidSTREAMFORMAT);
#if 0
	if (m_Streams[j]->m_uiFormatSize == 30)
	{
            char b[200];
	    avm_wave_format(b, sizeof(b), (const WAVEFORMATEX*)m_Streams[j]->m_pcFormat);
            AVM_WRITE("AVI writer", "STORE  %p %s\n", m_Streams[j]->m_pcFormat, b);
	}
#endif
	if (m_Streams[j]->m_uiFormatSize)
	{
	    AVM_WRITE("AVI writer", 1, "WriteHdr %d\n", m_Streams[j]->m_uiFormatSize);
	    write_le32(m_Streams[j]->m_uiFormatSize);
	    m_pFileBuffer->write(m_Streams[j]->m_pcFormat, m_Streams[j]->m_uiFormatSize);
	    if (m_Streams[j]->m_uiFormatSize & 1)
		m_pFileBuffer->write(&m_pFileBuffer, 1);
	}
	else
	{
	// fake header
	    write_le32(4);
	    write_le32(0);
	}
    }

    if (hdr_size > 0x700)
	throw FATAL("Too large header. Aborting");

    int curpos = m_pFileBuffer->lseek(0, SEEK_CUR);
    write_le32(ckidAVIPADDING);
    write_le32(0x7F4 - (curpos + 8));

    m_pFileBuffer->lseek(0x7F4, SEEK_SET);
    write_le32(FOURCC_LIST);
    write_le32(endpos - 0x7FC);
    write_le32(listtypeAVIMOVIE);

    m_pFileBuffer->lseek(0x10, SEEK_SET);
    write_le32(hdr_size);
    m_pFileBuffer->lseek(0, SEEK_END);
}

IWriteFile* CreateWriteFile(const char* name, int64_t flimit,
			    IStream::StreamFormat fmt,
			    int flags, int mask)
{
    return new AviWriteFile(name, flimit, flags, mask);
}

#undef __MODULE__

AVM_END_NAMESPACE;
