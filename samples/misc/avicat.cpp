/* $Id: avicat.cpp,v 1.22 2003/05/26 08:57:33 kabi Exp $ */

/*  avicut
 Copyright (C) 2002 Oliver Kurth <oku@masqmail.cx>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 Large chunks of code copied from
 avifile-0.6.0-20011109/samples/misc/avicat.cpp

 Thanks to Tom Pavel <pavel@alum.mit.edu>
 */

#include "avifile.h"
#include "aviplay.h"
#include "videoencoder.h"
#include "avm_creators.h"
#include "avm_except.h"
#include "version.h"

#include <stdio.h>
#include <stdlib.h> // exit
#include <unistd.h>		// for getopt()
//#include <ostream.h>
#include <limits.h>


template <class First, class Second> struct pair
{
    First first;
    Second second;

    pair(){};
    pair(First f, Second s) : first(f), second(s) {};
};


// hold on to audio samples until we have a chunk at least this big:
const int MinAudChunk = 128;

uint_t findAttrInfo(const avm::CodecInfo& cinfo, const avm::string& name,
		    avm::CodecInfo::Direction direction = avm::CodecInfo::Encode)
{
    uint_t i;
    const avm::vector<avm::AttributeInfo>& list =
	(direction == avm::CodecInfo::Encode) ? cinfo.encoder_info : cinfo.decoder_info;

    for(i = 0; i < list.size(); i++)
    {
	if(list[i].GetName() == name)
	    return i;
    }
    return avm::vector<const avm::AttributeInfo>::invalid;
}

uint_t findCodecInfo(fourcc_t fourcc,
		     avm::CodecInfo::Direction direction = avm::CodecInfo::Encode)
{
    uint_t i;
    const avm::vector<avm::CodecInfo>& list = video_codecs;

    for(i = 0; i < list.size(); i++){
	if(list[i].direction & direction){
	    if(list[i].fourcc == fourcc)
		return i;
	}
    }
    return avm::vector<const avm::CodecInfo>::invalid;
}

#if 0
IAviWriteStream* createVidOutStream(const IAviReadStream* inStr,
				    IAviWriteFile* outFile)
{
    BITMAPINFOHEADER bh;
    inStr->GetVideoFormatInfo((uint8_t*)&bh, sizeof(bh));
    int ftm = (int) (1000000. * inStr->GetFrameTime());
    ftm = 41708;
    IAviWriteStream* outStr =
	outFile->AddStream(AviStream::Video, (uint8_t*)&bh, sizeof(bh),
			   bh.biCompression, ftm);
    return outStr;
}

IAviWriteStream*
createAudOutStream (const IAviReadStream* inStr, IAviWriteFile* outFile)
{

    WAVEFORMATEX wf;
    inStr->GetAudioFormatInfo((uint8_t*)&wf, 0);
    IAviWriteStream* outStr =
	outFile->AddStream(AviStream::Audio, (uint8_t*)&wf, 18,
			   wf.wFormatTag,
			   wf.nAvgBytesPerSec,
			   wf.nBlockAlign);
    return outStr;
}
#endif

class AviCutter
{
protected:
    static const size_t AUDIO_BUFFER_SIZE = 512000;

    avm::vector<pair<int, int> > boundaries;
    avm::vector<avm::string> attributes;
    avm::IReadFile* inFile;
    avm::IWriteFile* outFile;

    avm::IReadStream* inVidStr;
    avm::vector<avm::IReadStream*> inAudStreams;

    avm::IWriteStream* outVideo;
    avm::vector<pair<WAVEFORMATEX*, avm::IWriteStream*> > outAudStreams;

    int64_t written_audio;
    int64_t written_frames;

    size_t buf_size;

    uint8_t *buf;
    uint8_t *aud_buf;
    double bpf;
    BITMAPINFOHEADER bh;

    avm::IVideoEncoder *vidEnc;
    void copyAudioFrames(int, int, bool);
    void createVideoEncoder(fourcc_t);
    void writeVideoFrame(avm::CImage *image);
    void copyVideoFrame();

public:
    AviCutter(avm::IWriteFile* outF,
	      avm::vector<pair<int, int> >& bounds,
	      avm::vector<avm::string>&attrs)
	: boundaries(bounds), attributes(attrs),
	outFile(outF), outVideo(NULL),
	written_audio(0), written_frames(0),
	buf_size(512000),
	vidEnc(NULL)
    {
	buf = new uint8_t[buf_size];
	aud_buf = new uint8_t[AUDIO_BUFFER_SIZE];

	if (bounds.size() == 0)
	{
	    pair<int,int> all(0, INT_MAX-1);
	    boundaries.push_back(all);
	}
    }

    ~AviCutter()
    {
	delete[] buf;
	delete[] aud_buf;
	if (vidEnc) delete vidEnc;
    }

    int Open(avm::string& filename)
    {
	unsigned int i;

	inFile = avm::CreateReadFile(filename.c_str());
	if (!inFile)
	    return -1;

	inVidStr = inFile->GetStream(0, avm::IStream::Video);

	if(outVideo == NULL && inVidStr)
	{
	    inVidStr->GetVideoFormat(&bh, sizeof(bh));
	    uint_t maxis = bh.biWidth * bh.biHeight * 4;
	    if (maxis > buf_size)
	    {
		buf_size = maxis;
		delete[] buf;
		buf = new uint8_t[buf_size];
	    }
	    outVideo = outFile->AddStream(inVidStr);
	}

	for(i = 0; i < inFile->AudioStreamCount(); i++)
	{
	    inAudStreams.push_back(inFile->GetStream(i, avm::IStream::Audio));

	    printf("detected %d audio streams\n", inAudStreams.size());

	    if (outAudStreams.size() <= i)
	    {
		avm::IReadStream* inAudStr = inAudStreams[i];
		uint_t s = inAudStr->GetAudioFormat();
		WAVEFORMATEX* wf = (WAVEFORMATEX*) new char[s];
		inAudStr->GetAudioFormat(wf, s);
		bpf = inVidStr->GetFrameTime() * wf->nAvgBytesPerSec;

		pair<WAVEFORMATEX*, avm::IWriteStream*>
		p(wf, outFile->AddStream(inAudStr));
		outAudStreams.push_back(p);
	    }
	    else
	    {
		/* check if formats match */
	    }
	}
        return 0;
    }

    void Close()
    {
	delete inFile;
	inVidStr = 0;
	inAudStreams.clear();
    }

    void Copy();
};

/* life would be much easier if we could create the encoder and then
 set the attribute...
 */

void AviCutter::createVideoEncoder(fourcc_t fourcc)
{
    if (vidEnc == NULL)
    {
	const avm::CodecInfo& info = video_codecs[findCodecInfo(fourcc)];

	avm::vector<avm::string>::iterator it;
	for(it = attributes.begin(); it != attributes.end(); it++){
	    char name[21], val[21];
	    sscanf(it->c_str(), "%20[^=]=%s", name, val);
	    printf("setting '%s' = '%s'\n", name, val);

	    uint_t index = findAttrInfo(info, name);
	    if(index != avm::vector<avm::AttributeInfo>::invalid){
		const avm::AttributeInfo& ainfo = info.encoder_info[index];
		switch(ainfo.kind){
		case avm::AttributeInfo::Integer:
		    avm::CodecSetAttr(info, name, atoi(val));
		    break;
		case avm::AttributeInfo::String:
		    avm::CodecSetAttr(info, name, val);
		    break;
		case avm::AttributeInfo::Select:
		    avm::CodecSetAttr(info, name, val);
		    break;
		}
	    }
	}
	avm::BitmapInfo bh1(bh.biWidth, bh.biHeight, 24);
	vidEnc = avm::CreateEncoderVideo(fourcc, bh1, NULL);
    }
}

void AviCutter::copyAudioFrames(int frame, int max_frame, bool reseek)
{
    for (unsigned i = 0; i < inAudStreams.size(); i++)
    {
	avm::IReadStream* inAudStr = inAudStreams[i];

	if (reseek)
	{
	    double time = inVidStr->GetTime();
	    inAudStr->SeekTime(time);
	}

	if (!inAudStr->Eof())
	{
	    int64_t excess = int64_t(bpf*(written_frames+1) - written_audio);
            int64_t to_read = int64_t(bpf*(max_frame - frame));

	    if (to_read > AUDIO_BUFFER_SIZE)
		to_read = AUDIO_BUFFER_SIZE;
	    to_read = to_read - (to_read%outAudStreams[i].first->nBlockAlign);

	    while ((excess>0) && (!inAudStr->Eof()))
	    {
		int flags = 0;
		uint_t bytes_read, samp_read;
		inAudStr->ReadDirect(aud_buf, (excess > to_read) ? excess : to_read,
				     to_read, samp_read, bytes_read, &flags);
		written_audio += bytes_read;
		excess -= bytes_read;
		//printf("ADDSAMP  %d\n", bytes_read);
		outAudStreams[i].second->AddChunk(aud_buf, bytes_read, flags);
	    }
	}
    }
}

void AviCutter::writeVideoFrame(avm::CImage *image)
{
    int is_kf;
    uint_t size;
    int hr = vidEnc->EncodeFrame(image, buf, &is_kf, &size);
    if(hr == 0)
	outVideo->AddChunk(buf, size, is_kf);
    else
	fprintf(stderr, "hr != 0\n");
    written_frames++;
}

void AviCutter::copyVideoFrame()
{
    int flags = 0;
    uint_t bytes_read, samp_read;
    inVidStr->ReadDirect(buf, buf_size, 1, samp_read, bytes_read, &flags);
    outVideo->AddChunk(buf, bytes_read, flags);
    written_frames++;
}

void AviCutter::Copy()
{
    if (inVidStr == 0)
	return;

    avm::vector<pair<int, int> >::iterator it_bound = boundaries.begin();
    //printf("INVIDEOF %d\n", inVidStr->Eof());
    while (!inVidStr->Eof() && it_bound != boundaries.end())
    {
	//printf("SEEEK  %d  %d\n", it_bound->first, it_bound->second);
	inVidStr->SeekToKeyFrame(it_bound->first);
	int next_kf_pos = inVidStr->GetNextKeyFrame(it_bound->first);
	bool need_aud_resync = true;
	bool have_encoder = false;

	for(int frame = inVidStr->GetPos(); frame < it_bound->second; frame++)
	{
	    if (! inVidStr->Eof()) {
		if (frame < next_kf_pos){
		    if (!have_encoder){
			if (vidEnc == NULL)
			    createVideoEncoder(bh.biCompression);
			if (vidEnc)
			{
			    inVidStr->SetDirection(1);
			    inVidStr->StartStreaming();
			    vidEnc->Start();
			    have_encoder = true;
			}
		    }
		    avm::CImage *image = inVidStr->GetFrame(true);

		    if (frame >= it_bound->first){
			copyAudioFrames(frame, next_kf_pos, need_aud_resync);
			need_aud_resync = false;

			if (vidEnc)
			{
			    writeVideoFrame(image);
			    printf("\rreencoded frame %d (%d)", frame, (int)written_frames); fflush(stdout);
			}
		    }
		}else{
		    copyAudioFrames(frame, it_bound->second, need_aud_resync);
		    need_aud_resync = false;

		    copyVideoFrame();
		    printf("\rcopied frame %d (%d)", frame, (int)written_frames); fflush(stdout);
		}
		if((frame+1 == next_kf_pos) || (frame+1 == it_bound->second)){
		    if(have_encoder){
			inVidStr->StopStreaming();
			vidEnc->Stop();
		    }
		}
	    }
	    else break;
	}

	it_bound++;
    }
}

void Usage(const char *arg0)
{
    printf("Usage: %s [-o outfile] [-a codec-attribute] [-b start,end "
	   "[-b start,end] ...] file1 [file2] ...\n", arg0);
    printf("Warning: Do not use with VBR audio streams.\n");
    printf("Warning: Do not mix concatenating and cutting at once.\n");
}

void parseBounds(avm::vector<pair<int, int> >& boundaries, avm::vector<avm::string>& bound_args)
{
    for(unsigned i = 0; i < bound_args.size(); i++){
	avm::string str = bound_args[i];
	unsigned int pos = str.find(',');
	if (pos != avm::string::npos)
	{
	    pair<int, int> pr;
	    pr.first = atoi(str.substr(0, pos).c_str());
	    pr.second = atoi(str.substr(pos+1).c_str());

	    if(pr.second > pr.first){
		boundaries.push_back(pr);
	    }else{
		fprintf(stderr, "second value must be greater than first: %s\n", str.c_str());
	    }
	}else{
	    fprintf(stderr, "need a comma between bounds: %s\n", str.c_str());
	}
    }
}

int main (int argc, char* argv[])
try
{
    unsigned int i;
    int debug = 0;
    const char* outFn = "out.avi";
    avm::vector<pair<int, int> > boundaries;
    avm::vector<avm::string>bound_args;
    avm::vector<avm::string>attributes;
    avm::vector<avm::string>inFiles;
    int seg_size = 0x7F000000;

    // Standard AVIlib sanity check:
    if (GetAvifileVersion() != AVIFILE_VERSION) {
	fprintf(stderr,
		"This binary was compiled for Avifile ver. %d, , but the library is ver. %d.\nAborting\n",
		AVIFILE_VERSION,
		GetAvifileVersion()
	       );
	return 0;
    }

    int ch;
    while ((ch = getopt(argc, argv, "a:b:dho:s:7")) != EOF) {
	switch ((char)ch) {
	case 'a':
	    attributes.push_back(optarg);
	    break;
	case 'b':
	    bound_args.push_back(optarg);
	    break;
	case 'd':
	    ++debug;
	    break;
	case 'o':
	    outFn = optarg;
	    break;
	case 's':
	    seg_size = atoi(optarg);
	    break;
	case '7':
	    seg_size = 700L*1024L*1024L; // 700MB for a CD
	    break;
	case 'h':
	case '?':
	default:
	    Usage(argv[0]);
	    exit(0);
	}
    }

    argc -= optind;
    argv += optind;
    if (argc < 1)
	Usage(argv[0]);

    int arg;
    for(arg = 0; arg < argc; arg++)
	inFiles.push_back(argv[arg]);

    parseBounds(boundaries, bound_args);

    // Do the real work
    avm::IWriteFile* outFile = avm::CreateWriteFile(outFn, seg_size);
    AviCutter cutter(outFile, boundaries, attributes);
    //printf("FILESIZE %d\n", inFiles.size());
    for(i = 0; i < inFiles.size(); i++)
	if (cutter.Open(inFiles[i]) == 0)
	{
	    cutter.Copy();
	    cutter.Close();
	}

    delete outFile;
}
catch(FatalError& error)
{
    error.Print();
}
