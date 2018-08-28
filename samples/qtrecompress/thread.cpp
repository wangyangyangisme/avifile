#include "recompressor.h"
#include "filters.h"

#include <avm_except.h>
#include <avm_cpuinfo.h>
#include <videodecoder.h>
#include <videoencoder.h>
#include <utils.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h> // abs

void* RecKernel::recompressThreadStart(void* arg)
{
    return ((RecKernel*) arg)->recompressThread();
}

void* RecKernel::recompressThread()
{
    try
    {
	m_Mutex.Lock();
	m_Cond.Broadcast();
	m_Mutex.Unlock();

	avm::IWriteFile* wf =
	    avm::CreateWriteFile(m_Filename);

	avm::IWriteStream* a_rws = 0; //audio raw write stream
	avm::IWriteStream* v_rws = 0; //video raw write stream
	avm::IAudioWriteStream* aws = 0;
	avm::IVideoWriteStream* vws = 0;
	avm::IReadStream* ars = 0;
	avm::IReadStream* vrs = 0;

	/*
	 append - next stream
	 add next audio channel
         */

	if (as && m_Streams[as].mode != Remove)
	{
	    ars = m_Streams[as].stream;
            if (m_Streams[as].mode == Recompress)
	    {
		ars->StopStreaming();
		ars->StartStreaming();
		//printf("NONREMOVE  %d  %d\n", as, m_Streams[as].startpos);
	    }
	    ars->Seek(m_Streams[as].startpos);
	}

	if (vs && m_Streams[0].mode != Remove)
	{
	    vrs = m_Streams[0].stream;
	    vrs->Seek(m_Streams[0].startpos);
	}
	if (!vrs && !ars)
	    return 0; // nothing to do

	if (vrs && m_Streams[0].mode == Recompress)
	{
	    vrs->StopStreaming();
	    vrs->StartStreaming();
	    vrs->Seek(vrs->GetPrevKeyFrame(m_Streams[0].startpos));
	    while (vrs->GetPos()<m_Streams[0].startpos && !vrs->Eof())
		vrs->ReadFrame();
	}


	BITMAPINFOHEADER bh;
	unsigned int videoFrames = 0;
        double videoStartTime = 0.0;
	if (vrs)
	{
	    videoFrames = vrs->GetLength();
	    videoStartTime = vrs->GetTime();
            unsigned int sz;
	    switch (m_Streams[0].mode)
	    {
	    case Recompress:
		vrs->GetOutputFormat(&bh, sizeof(bh));
		try
		{
		    avm::VideoEncoderInfo vi = m_Streams[0].vi;

		    for (unsigned i = 0; i < getFilterCount(); i++)
		    {
			Filter* fi = getFilterAt(i);
			fi->adjust(bh);
		    }
                    vi.header = bh;
		    printf("Compresor start fourcc: %.4s\n",(char*)& vi.compressor);
		    vws = wf->AddVideoStream(&vi, (unsigned int) (1000000.*vrs->GetFrameTime()));
                    if (vws)
			vws->Start();

		}
		catch (FatalError& e)
		{
		    e.Print();
		    vws = 0;
		}
		break;
	    case Copy:
		sz = vrs->GetVideoFormat();
		if (sz > 0)
		{
                    char* f = new char[sz];
		    vrs->GetVideoFormat(f, sz);
		    v_rws = wf->AddStream(avm::IStream::Video, f, sz,
					  ((BITMAPINFOHEADER*)f)->biCompression,
					  (unsigned int) (1000000.*vrs->GetFrameTime()));
                    memcpy(&bh, f, sizeof(bh));
                    delete[] f;
		}
		break;
	    case Remove:
		break;
	    }
	}

	WAVEFORMATEX wfmtx;
	char* ext = 0;

	unsigned int audioSamples = 0;
	double audioStartTime = 0.0;
	memset(&wfmtx, 0, sizeof(wfmtx));
        uint_t avsz;
	if (ars)
	{
	    audioSamples = ars->GetLength();
            audioStartTime = ars->GetTime();
	    switch(m_Streams[as].mode)
	    {
	    case Recompress:
		ars->GetOutputFormat(&wfmtx, sizeof(wfmtx));
		aws = wf->AddAudioStream(m_Streams[as].ai.fmt, &wfmtx,
					 m_Streams[as].ai.bitrate);
		if (aws)
		    aws->Start();
		//a_rws=wf->AddStream(AviStream::Audio, (char*)&wfmtx, 18, wfmtx.wFormatTag,
		//     	wfmtx.nAvgBytesPerSec, wfmtx.nBlockAlign);
		break;
	    case Copy:
		avsz = ars->GetAudioFormat();
		ext = new char[avsz];
		ars->GetAudioFormat(ext, avsz);
		memcpy(&wfmtx, ext, sizeof(wfmtx));
		if (wfmtx.nBlockAlign < 1)
		    wfmtx.nBlockAlign = 1;
		a_rws = wf->AddStream(avm::IStream::Audio, ext,
				      18 + wfmtx.cbSize,
				      wfmtx.wFormatTag,
				      wfmtx.nAvgBytesPerSec,
				      wfmtx.nBlockAlign);
                printf("COPY audio stream  %p  %d\n", a_rws, wfmtx.nBlockAlign);
		delete[] ext;
		break;
	    case Remove:
                printf("COPY remove stream\n");
		break;
	    }
	}

	int comp_frame_size = bh.biWidth * labs(bh.biHeight) * 4;
	uint8_t* comp_frame=0;
	if (m_Streams[0].mode != Remove)
	    comp_frame = new uint8_t[comp_frame_size];
	double bpf = 0;
	uint_t zzsize = 44100;
	if(vrs)
	{
	    if(aws)
		bpf=vrs->GetFrameTime()*(wfmtx.nSamplesPerSec*wfmtx.nChannels*wfmtx.wBitsPerSample)/8;
	    else
		bpf=vrs->GetFrameTime()*wfmtx.nAvgBytesPerSec;
	    zzsize = (uint_t)((1.0/vrs->GetFrameTime()) / 2 * bpf);
	}
	char* zz = new char[zzsize * 2];

	//printf("VWS %p  %d  %d\n", vws, m_Streams[0].endpos, m_Streams[0].startpos);
	m_pRecCb->setTotal(m_Streams[0].endpos - m_Streams[0].startpos,
		      audioSamples, videoStartTime, audioStartTime);

	int64_t time_start = longcount();
	int64_t last_progress = 0;
        int64_t time_current = 0;
	framepos_t written_frames = 0;
	unsigned int written_audio = 0;
        bool nodata = false;
	while (!nodata && rec_status)
	{
	    if (pause_status)
	    {
                int64_t ts = longcount();
		avm_usleep(100000);
		time_start += (longcount() - ts);
		continue;
	    }
            nodata = true;
	    for (unsigned i = as; i < m_Streams.size(); i++)
	    {
		avm::IReadStream* ars = m_Streams[i].stream;
		if (!ars || ars->Eof() || ars->GetPos() >= m_Streams[as].endpos)
		    continue;
                nodata = false;
		/*
		 if (vrs && (vrs->Eof() || vrs->GetPos() >= m_Streams[0].endpos))
		 {
		 vrs = 0;
		 continue;
		 }
		 */
		uint_t samples_read, bytes_read;

		uint_t awritten = 0;
		if (aws)
		{
                    awritten = aws->GetLength();
		    int64_t excess = (int64_t) (bpf * (written_frames + 1) - written_audio);
		    //printf("ExcessA %lld, written audio %d, written frames %d, bpf %f\n", excess, written_audio, written_frames, bpf);
		    if (vrs)
		    {
			while ((excess>0) && (!ars->Eof()))
			{
			    ars->ReadFrames(zz, (excess>zzsize) ? excess : zzsize,
					    zzsize, samples_read, bytes_read);
			    written_audio+=bytes_read;
			    excess-=bytes_read;
			    aws->AddData(zz, bytes_read);
			}
		    }
		    else
		    {
			ars->ReadFrames(zz, zzsize, zzsize,
					samples_read, bytes_read);
			written_audio+=bytes_read;
			aws->AddData(zz, bytes_read);
		    }
                    awritten = aws->GetLength() - awritten;
		}
		else if (a_rws)
		{
                    // preload 1 frames
		    int64_t excess = (int64_t) (bpf*(written_frames + 1) - written_audio);
		    //printf("ExcessB %lld, written audio %d, written frames %d, bpf %f\n", excess, written_audio, written_frames, bpf);
		    uint_t wa = 0;
		    if (excess > 0)
		    {
			awritten = a_rws->GetLength();
			if (vrs)
			{
			    int flags=0;
			    while (wa < zzsize && !ars->Eof())
			    {
				ars->ReadDirect(zz + wa, zzsize - wa, zzsize - wa,
						samples_read, bytes_read, &flags);
				written_audio += bytes_read;
				wa += bytes_read;
			    }
			    a_rws->AddChunk(zz, wa, flags);
			    //printf("ADDCHUNK  %d    exc %d\n", wa, excess);
			}
			else
			{
			    // excess=m_Streams[vs].endpos-ars->GetTime();
			    // if(excess>sizeof zz)excess=sizeof zz;
			    if (m_Streams[as].endpos <= ars->GetPos())
				excess=0;
			    else
				excess=zzsize;

			    if (excess < wfmtx.nBlockAlign)
			    {
				ars = 0;
				continue;
			    }
			    if (excess < wfmtx.nBlockAlign)
			    {
				ars = 0;
				continue;
			    }
			    int flags=0;
			    ars->ReadDirect(zz, excess,
					    (excess/wfmtx.nBlockAlign)*wfmtx.nBlockAlign,
					    samples_read, bytes_read, &flags);
			    written_audio += bytes_read;
			    excess -= bytes_read;
			    a_rws->AddChunk(zz, bytes_read, flags);
			}
			awritten = a_rws->GetLength() - awritten;
                        //printf("AWRITTEN %d\n", awritten);
		    }
		}
		m_pRecCb->addAudio(ars->GetPos(), awritten, ars->GetTime());

	    }

	    if (vrs)
	    {
		uint_t vsize;
		int iskeyframe;
		bool show_progress = false;

		time_current=longcount();
		if (to_float(time_current, last_progress) > 1.0)
		{
		    last_progress = time_current;
		    show_progress = true;
		}

		if (vws)
		{
		    vrs->ReadFrame();
		    avm::CImage* ptr = vrs->GetFrame();

		    if (!ptr)
		    {
			printf("WARNING: zero frame\n");
			vws->AddFrame(0);
		    }
		    else
		    {
			avm::CImage* im = new avm::CImage(ptr);
			for (unsigned i = 0; i < getFilterCount(); i++)
			{
			    Filter* fi = getFilterAt(i);
			    avm::CImage* new_im = fi->process(im, vrs->GetPos());
			    im->Release();
			    im = new_im;
			}
                        im->ToRGB();
                        char *b;
			vws->AddFrame(im, &vsize, &iskeyframe, &b);
			if (show_progress && !iskeyframe)
			{
			    show_progress = false;
                            last_progress = 0;
			}
			if (_ctl && show_progress)
			{
			    _ctl->setSourcePicture(ptr);
			    avm::CImage* new_im = new avm::CImage(im);
			    if (new_im)
			    {
				if (m_pRecFilter && m_pRecFilter->vd)
				    m_pRecFilter->vd->DecodeFrame(new_im, b, vsize, 0, iskeyframe);
				_ctl->setDestPicture(new_im);
				new_im->Release();
			    }
			}
			im->Release();
                        ptr->Release();
		    }
		}
		else
		{
		    uint_t samples_read;
		    vrs->ReadDirect(comp_frame, comp_frame_size, 1,
				    samples_read, vsize, &iskeyframe);
                    //printf("SAMP  %d %d %d\n", vrs->GetPos(), samples_read, iskeyframe);
		    if (!samples_read)
		    {
			printf("ERROR: Failed to read video frame\n");
			vrs = 0;
                        continue;
		    }
		    else if (v_rws)
			v_rws->AddChunk(comp_frame, vsize, iskeyframe);
		}
		written_frames++;
		m_pRecCb->addVideo(written_frames, vsize, vrs->GetTime(), iskeyframe);
		if (show_progress)
		{
		    framepos_t mint = vrs->GetLength();
		    if (mint > m_Streams[0].endpos)
                        mint = m_Streams[0].endpos;
		    double percent=(vrs->GetPos()-m_Streams[0].startpos)/double(mint - m_Streams[0].startpos);
		    m_pRecCb->setNewState(percent,
					  to_float(time_current, time_start),
					  wf->GetFileSize());
		}
	    }

	    if (!vrs)
	    {
		time_current=longcount();
		if (to_float(time_current, last_progress) > 1.0)
		{
		    last_progress = time_current;
		    framepos_t mint = ars->GetLength();
		    if (mint > m_Streams[as].endpos)
			mint = m_Streams[as].endpos;
		    double percent=(ars->GetPos()-m_Streams[as].startpos)/double(mint - m_Streams[as].startpos);
		    m_pRecCb->setNewState(percent,
					  to_float(time_current, time_start),
					  wf->GetFileSize());
		}
	    }

	}
	m_pRecCb->setNewState(1.0,
			      to_float(time_current, time_start),
			      wf->GetFileSize());
	m_pRecCb->finished();
	delete[] zz;
	delete wf;
	return 0;

    }
    catch (FatalError& e)
    {
	e.PrintAll();
	return 0;
    }
}

int RecKernel::startRecompress()
{
    m_Mutex.Lock();
    rec_status=1;
    pause_status=0;
    rec_thread = new avm::PthreadTask(0, recompressThreadStart, this);
    /* waiting for recompress thread startup */
    m_Cond.Wait(m_Mutex);
    m_Mutex.Unlock();
    return 0;
}

int RecKernel::pauseRecompress()
{
    pause_status = !pause_status;
    return pause_status;
}

int RecKernel::stopRecompress()
{
    rec_status = 0;
    delete rec_thread;
    return 0;
}
