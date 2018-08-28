/*
 * audio stream functions
 */

#include "ReadStreamA.h"
#include "avifmt.h"
#include "avm_creators.h"
#include "audiodecoder.h"
#include "avm_cpuinfo.h"
#include "utils.h"
#include "avm_output.h"
#include <string.h> // memcpy
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

AVM_BEGIN_NAMESPACE;

static const short mpeg_bitrates[6][16] =
{
    /* -== MPEG-1 ==- */
    /* Layer I   */
    { 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448 },
    /* Layer II  */
    { 0, 32, 48, 56, 64, 80,  96, 112, 128, 160, 192, 224, 256, 320, 384 },
    /* Layer III */
    { 0, 32, 40, 48, 56, 64,  80, 96, 112, 128, 160, 192, 224, 256, 320 },

    /* -== MPEG-2 LSF ==- */
    /* Layer I         */
    { 0, 32, 48, 56, 64, 80,  96, 112, 128, 144, 160, 176, 192, 224, 256 },
    /* Layers II & III */
    { 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160 },
    { 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160 },
};

static const int mpeg_sample_rates[4][3] =
{
    { 44100, 48000, 32000 }, // MPEG1
    { 22050, 24000, 16000 }, // MPEG2
    { 0, 0, 0 }, 	     // ERR
    { 11025, 12000, 8000 }   // MPEG2.5
};

/*
bits name              comments
--------------------------------------------------
12   sync              0xFFF  (12 bit is 0 for mpeg2.5 !)
1    version           1=mpeg1.0, 0=mpeg2.0
2    lay               4-lay = layerI, II or III
1    error protection  0=yes, 1=no
4    bitrate_index     see table below
2    sampling_freq     see table below
1    padding
1    extension         see table below
2    mode              see table below
2    mode_ext          used with "joint stereo" mode
1    copyright         0=no 1=yes
1    original          0=no 1=yes
2    emphasis          see table below
--------------------------------------------------
*/

#define MPEG_ID_MASK        0x00180000
#define MPEG_MPEG1          0x00180000
#define MPEG_MPEG2          0x00100000
#define MPEG_MPEG2_5        0x00000000

#define MPEG_LAYER_MASK     0x00060000
#define MPEG_LAYER_III      0x00020000
#define MPEG_LAYER_II       0x00040000
#define MPEG_LAYER_I        0x00060000
#define MPEG_PROTECTION     0x00010000
#define MPEG_BITRATE_MASK   0x0000F000
#define MPEG_FREQUENCY_MASK 0x00000C00
#define MPEG_PAD_MASK       0x00000200
#define MPEG_PRIVATE_MASK   0x00000100
#define MPEG_MODE_MASK      0x000000C0
#define MPEG_MODE_EXT_MASK  0x00000030
#define MPEG_COPYRIGHT_MASK 0x00000008
#define MPEG_HOME_MASK      0x00000004
#define MPEG_EMPHASIS_MASK  0x00000003

#define LAYER_I_SAMPLES       384
#define LAYER_II_III_SAMPLES 1152


Mp3AudioInfo::Mp3AudioInfo() : frame_size(0) {}

// MP3 header needs 40 bytes
int Mp3AudioInfo::Init(const char* buf, int fast)
{
    header = avm_get_be32(buf);

    //AVM_WRITE("audio reader", "HDR %x    %.4s\n", header, (char*)&header);
    // CHECK if it's mpeg & Get the sampling frequency
    int frequency_index = (header >> 10) & 3; // MPEG_FREQUENCY_MASK
    layer = 4 - (header >> 17) & 3;
    mode = (MPEG_MODE) (3 - (header >> 19) & 3); // MPEG_ID_MASK
    if ((header & 0xffe00000) != 0xffe00000
	|| frequency_index > 2 || layer > 3 || mode == 2)
	return 0; // this is not mpeg header

    sample_rate = mpeg_sample_rates[mode][frequency_index];

    //AVM_WRITE("audio reader", "HDR %x    m:%d  fi:%d   sr:%d\n", header, mode, frequency_index, sample_rate);
    // Get stereo mode index
    stereo_mode = (STEREO_MODE)((header & MPEG_MODE_MASK) >> 6);
    num_channels = (stereo_mode == MODE_MONO) ? 1 : 2;
    // Get layer
    samples_per_frame = LAYER_I_SAMPLES;
    if (layer > 1)
	samples_per_frame = LAYER_II_III_SAMPLES;

    start_offset = 40;
    if (fast)
	return start_offset;

    if (mode == MPEG2)
	samples_per_frame /= 2;
    else if (mode == MPEG2_5)
	samples_per_frame /= 4;

    // Check for VBR stream (Xing header)
    int b = 4;
    if (header & MPEG_ID_MASK)
	b += (stereo_mode != MODE_MONO) ? 32 : 17; // mpeg1 mpeg2
    else
	b += (stereo_mode != MODE_MONO) ? 17 : 9; // mpeg2_5
    xing = (avm_get_le32(buf + b) == mmioFOURCC('X', 'i', 'n', 'g'));
    bitrate = (xing) ? 0 : GetBitrate();

    //printf ("%d %d    l:%d  %d   ch:%d\n", sample_rate, bitrate, layer, samples_per_frame, num_channels);
    frame_size = GetFrameSize();
    if (xing)
	start_offset += frame_size + 4;

    return start_offset;
}
int Mp3AudioInfo::GetBitrate()
{
    int bitrate_idx = (header & MPEG_BITRATE_MASK) >> 12;
    int layer_idx = layer + ((layer > 0 && mode == MPEG1) ? -1 : 2);
    return mpeg_bitrates[layer_idx][bitrate_idx] * 1000;
}
int Mp3AudioInfo::GetFrameSize()
{
    int tmp_bitrate = GetBitrate();

    if (!tmp_bitrate)
	return 0;

    int ret = (layer == 1) ? 48 : 144;

    if (mode == MPEG2 || mode == MPEG2_5)
	ret /= 2;
    //ret /= 4;
    //AVM_WRITE("audio reader", "%d  %d  %d \n", ret, tmp_bitrate, sample_rate);
    ret = ret * tmp_bitrate / sample_rate;
    if (header & MPEG_PAD_MASK)
	ret++;

    return ret;
}
void Mp3AudioInfo::PrintHeader()
{
    const char* sf[] = { "MPEG1", "MPEG2", "ERR", "MPEG2.5" };
    const char* sm[] = { "Stereo", "JointStereo", "DualChannel", "Mono" };
    if (frame_size)
	AVM_WRITE("audio reader", "%s Layer-%d %dHz %dkbps %s %s"
		  "(%d,%d,%d)\n",
		  sf[mode], layer, sample_rate, bitrate / 1000,
		  sm[stereo_mode], xing ? "Xing " : "",
		  samples_per_frame, start_offset, frame_size);
}

ReadStreamA::ReadStreamA(IMediaReadStream* stream)
    :ReadStream(stream), m_pAudiodecoder(0), m_uiMinSize(0), m_bIsMp3(false)
{
    // FIXME - asf reading needs to know also about VideoStreams
    // currently this isn't possible to use it this way
    // Flush();
    WAVEFORMATEX* wf = (WAVEFORMATEX*) m_pFormat;

    // fix incorrect header
    if (wf->wFormatTag == 0x55 && (wf->cbSize != 12
				   || m_uiFormatSize < sizeof(MP3WAVEFORMATEX)))
    {
	if (m_uiFormatSize < sizeof(MP3WAVEFORMATEX))
	    AVM_WRITE("audio reader", "WARNING: fixing bad WAVEFORMAT header for MP3 audio track (sz:%d != %d)\n", m_uiFormatSize, sizeof(MP3WAVEFORMATEX));
        else
	    AVM_WRITE("audio reader", "WARNING: fixing bad WAVEFORMAT header for MP3 audio track (cb:%d != 12)\n", wf->cbSize);
	m_uiFormatSize = sizeof(MP3WAVEFORMATEX);
	m_pFormat = new char[m_uiFormatSize];
	memcpy(m_pFormat, wf, sizeof(WAVEFORMATEX));
	delete[] wf;
        wf = (WAVEFORMATEX*) m_pFormat;
	MP3WAVEFORMATEX* mwf = (MP3WAVEFORMATEX*) m_pFormat;

	mwf->wf.cbSize = sizeof(MP3WAVEFORMATEX) - sizeof(WAVEFORMATEX);//12
	mwf->wID = 1;			//
	mwf->fdwFlags = 2;		// These values based on an old Usenet post!!
	mwf->nBlockSize = 0;		// this value COMPUTE later...
	mwf->nFramesPerBlock = 1;	//
	mwf->nCodecDelay = 1393;	// 0x571 for F-IIS MP3 Codec
    }
    m_uiBps = wf->nAvgBytesPerSec;
    m_uiSampleSize = wf->nBlockAlign;

    switch (wf->wFormatTag)
    {
    case 0x55:
    case 0x50:
	{
	    Mp3AudioInfo ai;
	    Mp3AudioInfo last;
	    char b[8];
	    uint_t bf = 0;
	    uint_t rb = 0;
	    int maxloop = 3000;
            memset(&last, 0, sizeof(last));
	    while (ai.frame_size == 0)
	    {
		if (bf)
		{
		    memmove(b, b + 1, bf);
		    bf--; // next byte
		}
		uint_t samp, bytes = 4 - bf;
                //printf("BYTES %d %d %d\n", bytes, bf, samp);
		if (ReadDirect(b + bf, bytes, bytes, samp, bytes) < 0)
		    //|| !bytes)
		    break;
		//printf("READ %d  %d\n", bytes, samp);
		bf += bytes;
                rb += bytes;
		//printf("RB %d  %d   0x%x\n", rb, bytes, *(int*)b);
		if (!ai.Init(b, true))
		    continue;

		//ai.PrintHeader();
		if (rem_size > 36 && rem_local >= 4)
		    ai.Init(rem_buffer + rem_local - 4); // full init
		else
		{
		    ai.xing = 0;
		    ai.bitrate = ai.GetBitrate();
		    ai.frame_size = ai.GetFrameSize();
		}
		// now we have found 4 bytes which might be mp3 header
                // let's do some more checking
		//printf("BFSCAN  %d   %d  %d\n", bf, rem_size, rem_local);
		if ((ai.frame_size - 4) == rem_size && rem_local >= 4)
		{
		    // heuristic 1:
		    // looks like we have found reasonable good mp3 header
		    // it's matching block boundary of the input frame
		    //printf("YES 1st. match\n");
		    break;
		}
		//printf("AAA  %d  %d\n", ai.frame_size, rem_size);
		if ((ai.frame_size - 4) < rem_size)
		{
		    // we could check if the next packet has the same
		    // characteristic
		    Mp3AudioInfo bi;
		    if (bi.Init(rem_buffer + rem_local + ai.frame_size - 4, true)
			&& bi.sample_rate == ai.sample_rate
			&& bi.layer == ai.layer)
		    {
			//printf("YES 2nd. match\n");
                        break;
		    }
                    ai.frame_size = 0;
		    continue; // failed - try to get next mp3 header
		}
                // this is unsolved if we get here with to short packet
		//ai.PrintHeader();
		if (maxloop--)
		{
		    //last.PrintHeader();
		    if (last.sample_rate == ai.sample_rate
			&& last.layer == ai.layer)
			break;
                    last = ai;
		    ai.frame_size = 0;
		}
		else
		    AVM_WRITE("audio reader", "failed to easily identify mp3 header! (%d, %d, %d) \n"
			      ,rem_size, rem_local, ai.frame_size);

		//int ss = 0; for (uint_t x = 0; x < bytes; x++) ss += b[x];
		//AVM_WRITE("audio reader", "HR  %d    lb %d   samp: %d    %d  sum  %d\n", hr, bytes, samp, bf, ss);
	    }
	    if (ai.frame_size)
	    {
		if (rem_local >= 4)
		{
		    // restore last mp3 header
		    rem_local -= 4; // so unread
		    rem_size += 4;
		    rb -= 4;
		}
		ai.PrintHeader();
		wf->nChannels = ai.num_channels;
		wf->nSamplesPerSec = ai.sample_rate;
		wf->nBlockAlign = 1; // Windows likes that this way...
		//printf("BLOCKALIGN %d  %d\n", wf->nBlockAlign, ai.frame_size);
                m_uiSampleSize = ai.frame_size;

		if (ai.layer == 3)
		{
		    if (wf->wFormatTag != 0x55)
		    {
			AVM_WRITE("audio reader", "WARNING: fixing bad MP3 Layer3 tag 0x%x -> 0x55\n", wf->wFormatTag);
			wf->wFormatTag = 0x55; // for layer3
		    }// FIXME ai.xing
		    if (!ai.xing && stream->GetSampleSize())
		    {
			if (ai.bitrate)
			{
			    // hard to calculate for VBR - so check only CBR
			    int bt = m_uiBps - ai.bitrate / 8;
			    if (bt < 0) bt = -bt; //labs
			    // do bitrate fix - but only if the difference
			    // is huge
			    // I've seen stream where packets where
                            // 16kbps 24kbps 16kbps --> ~20kbps
			    if (bt > 1000)
			    {
				if (stream->FixAvgBytes(ai.bitrate / 8) == 0)
				{
				    AVM_WRITE("audio reader", "WARNING: fixing wrong avg bitrate %dB/s -> %dB/s\n", m_uiBps, ai.bitrate / 8);
				    m_uiBps = wf->nAvgBytesPerSec = ai.bitrate / 8;
				}
			    }
			}
			//printf("SAMELESIZE %d\n", stream->GetSampleSize());
			int bs = ((MP3WAVEFORMATEX*)wf)->nBlockSize - ai.frame_size;
                        if (bs < 0) bs = -bs;
			if (bs > 256)
			{
			    AVM_WRITE("audio reader", "WARNING: fixing bad MP3 block size %d -> %d\n",
				      ((MP3WAVEFORMATEX*)wf)->nBlockSize, ai.frame_size);
			    ((MP3WAVEFORMATEX*)wf)->nBlockSize = ai.frame_size;
			}
		    }
		}
		else
		{
		    if (wf->wFormatTag != 0x50)
		    {
			AVM_WRITE("audio reader", "WARNING: fixing bad MP3 Layer2 tag 0x%x - should be 0x50\n", wf->wFormatTag);
			wf->wFormatTag = 0x50; // for layer2
		    }
		}
		if (rb > 0)
		    AVM_WRITE("audio reader", "junk size at the begining:  time:%.2fs  pos:%u (%ub)\n", GetTime(), GetPos(), rb);
	    }
	}
        if (wf->wFormatTag == 0x55)
	    m_bIsMp3 = true;
	break;
    case 0x2000:
	if (wf->nSamplesPerSec > 48000)
	    wf->nSamplesPerSec = 48000;
	break;
    }
}

ReadStreamA::~ReadStreamA()
{
    StopStreaming();
}

void ReadStreamA::Flush()
{
    ReadStream::Flush();
    if (m_pAudiodecoder)
	m_pAudiodecoder->Flush();
}

// FIXME - when m_pAudiodecoder is running we may supply real data
uint_t ReadStreamA::GetAudioFormat(void* wf, uint_t size) const
{
    //printf("DATE  %p  %d\n", wf, size);
    //for (int i = 0; i < m_format_size; i++)
    //    printf("### %d  %x\n", i, ((unsigned char*)m_format)[i]);
    if (wf)
	memcpy(wf, m_pFormat, (size < m_uiFormatSize) ? size : m_uiFormatSize);
    return m_uiFormatSize;
}

uint_t ReadStreamA::GetFrameSize() const
{
    return m_uiMinSize;
}

uint_t ReadStreamA::GetOutputFormat(void* format, uint_t size) const
{
    if (!m_pAudiodecoder)
	return 0;
    if (!format || size < sizeof(WAVEFORMATEX))
	return sizeof(WAVEFORMATEX);

    return m_pAudiodecoder->GetOutputFormat((WAVEFORMATEX*)format);
}

framepos_t ReadStreamA::GetPos() const
{
    if (m_uiSampleSize)
    {
	uint_t sub = rem_size / m_uiSampleSize;
	if (sub > m_uiLastPos)
	    sub = m_uiLastPos;
        m_uiLastPos - sub;
    }
    return m_uiLastPos;
}

double ReadStreamA::GetTime(framepos_t pos) const
{
    //printf("GETTIMA %f  %d  %d\n", m_dLastTime, rem_size, pos);
    //AVM_WRITE("audio reader", "GETTIME   %d   %f   %d\n", pos, smtime, rem_size);
    //printf("ret %f   %f\n", smtime, rem_size/(double)dwBps);
    if (pos == ERR)
    {
	double smtime = m_dLastTime;
	if (m_uiBps > 0)
	{
	    //AVM_WRITE("audio reader", "AudioGetTime remsize %f   time: %f  pos: %d\n", rem_size/(double)dwBps, smtime, pos);
	    // rem_size must be kept small otherwice
	    // there will be sync problems with VBR audio
	    smtime -= rem_size / (double)m_uiBps;
	    if (smtime < 0.0)
		smtime = 0.0;
	}
        return smtime;
    }
    return m_pStream->GetTime(pos);
}

bool ReadStreamA::IsStreaming() const
{
    return (m_pAudiodecoder != NULL);
}

int ReadStreamA::ReadFrames(void* buffer, uint_t bufsize, uint_t samples,
			    uint_t& samples_read, uint_t& bytes_read)
{
    if (!m_pAudiodecoder || !samples || bufsize < m_uiMinSize)
	return -1;

    uint_t srcsize = m_pAudiodecoder->GetSrcSize(bufsize);
    bool bHadPacket = false;
    //printf("MIN  %d   SRC %d    reml:%d  rems:%d    bs:%d\n", m_uiMinSize, srcsize, rem_local, rem_size, bufsize);
    if (m_bIsMp3 && rem_size >= 4)
    {
        // for vbr audio keep buffer minimal for the good timing
	Mp3AudioInfo ai;
	if (ai.Init(rem_buffer + rem_local, false))
	    srcsize = ai.frame_size * 2;
	//printf("RESULT %d  %d\n", srcsize, ai.frame_size);
    }

    // wrap the buffer around
    if (rem_local > rem_limit/2)
    {
	if (rem_size > 0)
	{
	    memcpy(rem_buffer, rem_buffer + rem_local, rem_size);
	    //printf("MEMCPY %d\n", rem_size);
	}
        rem_local = 0;
    }
    while (rem_size <= srcsize)
    {
	//printf("SRCSIZE  %d  %d  %d    sam:%d  loc:%d  lim:%d\n", bufsize, srcsize, rem_size, samples, rem_local, rem_limit);
	//AVM_WRITE("audio reader", "***********convert   %d -> %d    (rem: %d)  %f\n", srcsize, bufsize, rem_size);
	//AVM_WRITE("audio reader", "-----------bufsize: %d   rem_size: %d   srcsize: %d  samples: %d\n", bufsize, rem_size, srcsize, samples);
	if (m_pPacket)
	{
	    bHadPacket = true;
	    // check if the whole chunk fit into local buffer
	    uint_t ns = rem_local + rem_size + m_pPacket->size;
	    if (ns < srcsize)
		ns = srcsize;
	    if (ns > 2 * rem_limit)
	    {
		rem_limit = (ns > 50000U) ? ns : 50000U;
		char* temp_buffer = (char*)malloc(rem_limit * 2);
		if (rem_buffer)
		{
		    memcpy(temp_buffer, rem_buffer + rem_local, rem_size);
		    free(rem_buffer);
		    rem_local = 0;
		}
		rem_buffer = temp_buffer;
	    }

	    // add chunk to our buffer
	    memcpy(rem_buffer + rem_local + rem_size,
		   m_pPacket->memory + m_pPacket->read,
		   m_pPacket->size - m_pPacket->read);
	    rem_size += m_pPacket->size - m_pPacket->read;
	    m_pPacket->read = m_pPacket->size;
	}
	ReadPacket();
	//printf("AUDIOp %p\n", m_pPacket);
	//printf("AUDIO %lld\n", m_pPacket->timestamp);
	if (!m_pPacket)
	{
            if (!rem_size)
		m_iEof++;
	    break;
	}
	//printf("-----SAMPLES  %d  %d\n", rem_size, p->size);
    }

    if (m_bIsMp3)
    {
	while (rem_size > 4)
	{
	    Mp3AudioInfo ai;
	    // using AudioInfo is slower but also more safe...
	    // this will skip bad mp3 chunks for Layer-3 audio stream
	    if (ai.Init(rem_buffer + rem_local, true) > 0
		&& ai.sample_rate == ((WAVEFORMATEX*)m_pFormat)->nSamplesPerSec
		&& ai.layer == 3)
		break;
	    //uint32_t t = avm_get_be32(rem_buffer + rem_local); printf("SKIP %d   %d   %x\n", rem_size, srcsize, t);
	    // check next byte
	    rem_local++;
	    rem_size--;
	}
    }

    //for (int i = 0; i < 16; i++)  printf(" 0x%02x", (uint8_t) *(rem_buffer + rem_local + i)); printf("\n");
    //Mp3AudioInfo ai; ai.Init(rem_buffer + rem_local, false); printf("AI SIZE  %d\n", ai.frame_size);

    uint_t ocnt = 0;
    uint_t lr = 0;
    int hr = m_pAudiodecoder->Convert(rem_buffer + rem_local,
				      (srcsize < rem_size) ? srcsize : rem_size,
				      (char*)buffer, bufsize, &lr, &ocnt);
    //int ds = open("/tmp/xyz", O_WRONLY | O_CREAT | O_APPEND, 0666); write(ds, rem_buffer + rem_local, lr); close(ds);
    //int ws = open("/tmp/wav", O_WRONLY | O_CREAT | O_APPEND, 0666); write(ws, buffer, ocnt); close(ws);
    //printf("CONVSZE %d  %d   %d  rs:%d   bs:%d\n", lr, ocnt, hr, rem_size, bufsize);
    if (hr < 0 || (ocnt <= 0 && lr <= 0))
    {
	//printf("ERRORCONVSZE %d  %d   %d   rs: %d\n", lr, ocnt, hr, rem_size );
        uint_t bs = ((WAVEFORMATEX*)m_pFormat)->nBlockAlign;
	if (rem_size > srcsize && rem_size > bs)
	{
	    rem_size -= bs;
	    rem_local += bs;
	}
	else if (!bHadPacket)
	    rem_size = 0;
        ocnt = 0;
    }
    else
    {
	if (lr > rem_size)
	    lr = rem_size;
	rem_local += lr;
	rem_size -= lr;
    }

    //AVM_WRITE("audio reader", "locread:%6d outputsz:%7d convsz:%6d lr:%d  bs:%d\n", rem_local, ocnt, convsz, lr, rem_size);
#if 0
    unsigned char* p = (unsigned char*)buffer + convsz;
    for (int i = -32; i < 64; i++)
    {
	if (!(i % 8))
	    AVM_WRITE("audio reader", "\n%4d ", i);
	//AVM_WRITE("audio reader", "  0x%4.4x", abs(p[i] - p[i-2]) & 0xffff);
	AVM_WRITE("audio reader", "  0x%02X", p[i]);
    }
#endif

    //printf("PACKET %d  %d\n", bHadPacket, rem_size);
    //AVM_WRITE("audio reader", 1, "ReadStreamA::ReadFrames conv %d bytes to %d (%d)\n",
    //          (int)rem_local, (int)ocnt, (int)bufsize);

    bytes_read = ocnt;
    samples_read = bytes_read;
    if (m_uiSampleSize > 1)
        samples_read /= m_uiSampleSize;

    //AVM_WRITE("audio reader", 3, "ReadStreamA: new sample is %d\n", m_pStream->GetPos());
    return 0;
}

int ReadStreamA::SkipTo(double pos)
{
    //printf("SKIPPOS %f   %f  %d  bps:%d\n", pos, GetTime(), m_uiSampleSize, dwBps);
    char* b = new char[8192];
    int i = 0;
    if (!m_uiSampleSize)
        return 0;
    while ((pos - GetTime()) > 0.001)
    {
	uint_t samples_read, bytes_read;
	int bt;
	if (m_uiBps > 0)
	{
	    int m = (int)((pos - GetTime()) * m_uiBps) / (int) m_uiSampleSize;
	    if (m > (int)(8192 / m_uiSampleSize))
		m = (8192 / m_uiSampleSize);

	    //printf("M %d  %d\n", m, m_uiSampleSize);
	    if (m > 0)
		bt = m * m_uiSampleSize; // rounded to sample size
	    else
		break;
	}
	else
	    bt = 2 * m_uiSampleSize;
	if (ReadDirect(b, bt, bt / m_uiSampleSize, samples_read, bytes_read) < 0
	    || !bytes_read)
	    break;
    }
    delete[] b;
    return 0;
}

int ReadStreamA::SeekTime(double timepos)
{
    double t = GetTime();
    if ((timepos - t) < 0.001 && (timepos - t) > -0.001 && timepos > 0.001)
    {
	//printf("---------SEEK DISCARD %f  %f\n", t, timepos);
	return 0;
    }
    return ReadStream::SeekTime(timepos);
}

int ReadStreamA::StartStreaming(const char* privname)
{
    if (m_pAudiodecoder)
	return 0; // already streaming

    m_pAudiodecoder = CreateDecoderAudio((WAVEFORMATEX*)m_pFormat, privname);

    if (!m_pAudiodecoder)
    {
	AVM_WRITE("audio reader", "Failed to initialize audio decoder for format 0x%x\n",
		  ((WAVEFORMATEX*)m_pFormat)->wFormatTag);
        return -1;
    }

    m_uiMinSize = m_pAudiodecoder->GetMinSize();
    Flush();
    return 0;
}

int ReadStreamA::StopStreaming()
{
    if (m_pAudiodecoder)
    {
	FreeDecoderAudio(m_pAudiodecoder);
	m_pAudiodecoder = 0;
	m_uiMinSize = 0;
    }
    return 0;
}

AVM_END_NAMESPACE;
