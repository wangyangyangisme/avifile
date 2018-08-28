#include "aviplay_impl.h"
#include "avm_cpuinfo.h"
#include "avm_output.h"
#include "avm_creators.h"
#include "utils.h"

#include <sys/time.h> //needed by sys/resource.h in FreeBSD
#include <sys/resource.h> //set/getpriority
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h> //exit
#include <unistd.h> //close

#ifdef __NetBSD__
#include <sys/sched.h>
#else
#include <sched.h>
#endif

// has to go with mga_vid
#if	HAVE_SYS_IOCCOM_H
#include <sys/ioccom.h>
#endif
#include "mga_vid.h"


AVM_BEGIN_NAMESPACE;

// preffer video playing thread
static const int PRIORITY_ADD_VIDEO = 0;
static const int PRIORITY_ADD_AUDIO = 0;
static const float m_fDropLimit = -0.015;

float AviPlayer::getVideoAsync()
{
    if (m_lTimeStart == 0)
    {
	m_dFrameStart = (m_pVideostream) ? m_pVideostream->GetTime() : 0.;
	m_dLastFrameStart = m_dFrameStart;
	m_lTimeStart = longcount();
	AVM_WRITE("aviplay", 1, "AviPlayer::getVideoAsync(): resetting\n");
    }

    if (!m_pVideostream)
	return 0.;

    double actual_time;
    if (m_pAudioRenderer && !m_pAudioRenderer->Eof())
    {
	actual_time = m_pAudioRenderer->GetTime();
    }
    else
    {
	// for stream without audio track or when audio track has finished...
	// since we want to show real-time video we have to
        // compute async deviation from real time
	actual_time = m_dFrameStart + to_float(longcount(), m_lTimeStart);
    }

    //printf("ATIM  %f   %f\n", m_pVideostream->GetTime(), actual_time);
    return m_pVideostream->GetTime() - actual_time;
}

/*
 * check for synchronization event
 *
 * might block this task - it will await for broadcast from the main thread
 * returns: 0 - quit
 *          1 - ok could continue
 */
int AviPlayer::checkSync(ThreadId i)
{
    //printf("Thread: %d  m_bHangup: %d\n", i, m_bHangup);
    if (!m_bQuit && m_bHangup)
    {
	m_ThreadCond[i].Wait(m_ThreadMut[i]);
	AVM_WRITE("aviplay", 1, "Thread unlocked %d\n", i);
    }
    return (!m_bQuit);
}

/*
 * block all running threads - implemented as recursive lock
 */
int AviPlayer::lockThreads(const char *name)
{
    Locker locker(m_LockMutex);

    if (m_bQuit)
	return -1;

    if (++m_iLockCount > 1)
	return 0;

    m_bHangup = true;
    AVM_WRITE("aviplay", 1, "Waiting for main_thread to hang up (%s)...", ((name) ? name : "???"));

    if (m_pVideostream)
    {
	m_ThreadMut[THREAD_VIDEO].Lock();
	AVM_WRITE("aviplay", 1, " video");

	m_ThreadMut[THREAD_DECODER].Lock();
	AVM_WRITE("aviplay", 1, " decoder");
    }

    m_ThreadMut[THREAD_AUDIO].Lock();
    m_QueueMutex.Lock();

    AVM_WRITE("aviplay", 1, " audio");

    syncFrame();

    // now we have all threads locked
    m_bInitialized = false;

    AVM_WRITE("aviplay", 1, " OK!\n");
    return 0;
}

/*
 * unlock all waiting threads in cond_wait state
 */
void AviPlayer::unlockThreads()
{
    Locker locker(m_LockMutex);

    if (--m_iLockCount > 0)
        return;

    // pausetest
    m_iLockCount = 0;
    m_lTimeStart = 0;
    m_bHangup = false;
    m_bInitialized = true;

    AVM_WRITE("aviplay", 1, "Unlock threads\n");

    m_QueueCond.Broadcast();
    m_QueueMutex.Unlock();

    m_ThreadCond[THREAD_AUDIO].Broadcast();
    m_ThreadMut[THREAD_AUDIO].Unlock();

    if (m_pVideostream)
    {
	m_ThreadCond[THREAD_DECODER].Broadcast();
	m_ThreadMut[THREAD_DECODER].Unlock();

	m_ThreadCond[THREAD_VIDEO].Broadcast();
	m_ThreadMut[THREAD_VIDEO].Unlock();
    }
}

// returns true when some frames have been dropped
// called with m_QueueMutex LOCKED!
bool AviPlayer::dropFrame()
{
    if (m_bVideoAsync || !m_bVideoDropping
	|| to_float(longcount(), m_lTimeStart) < 0.3)
	return false;

    bool ret = false;
    // here we try to eliminate all the frames
    // until next key-frame with reasonable close time
    //printf("enter drop: async=%f\n", getVideoAsync());
    framepos_t orig = m_pVideostream->GetPos();
    framepos_t kpos = orig;
    bool locked = false;
    // do not skip frames if we have full buffer
    // in this case the problem is with slow rendering
    while (m_pVideostream->GetBuffering() <= 1)
    {
	framepos_t npos = m_pVideostream->GetNextKeyFrame(kpos + 1);
	if (npos == m_pVideostream->ERR || kpos >= npos)
            break;
	double curtime = (m_pAudioRenderer) ? m_pAudioRenderer->GetTime() :
	    m_dFrameStart + to_float(longcount(), m_lTimeStart);
	double nt = m_pVideostream->GetTime(npos) - curtime;
	if (nt < 0.1)
	{
	    kpos = npos;
            continue; // next kf needed
	}
	if (nt > 0.3)
            npos = kpos;

	if (npos == orig)
            break;

	AVM_WRITE("aviplay", 1, "AviPlayer::dropFrame()\n"
		  "  async %f  new_vtime: %f  cur_atime: %f  diff %f   %d - %d\n",
		  getVideoAsync(), m_pVideostream->GetTime(npos), curtime, nt, npos, kpos);

	//printf("o: %d   k: %d  n: %d\n", orig, kpos, npos);

	if (!locked)
	{
            // stop decoder for proper frame skip
	    m_bDropping = true;
	    m_QueueMutex.Unlock();
	    m_ThreadMut[THREAD_DECODER].Lock();
	    m_QueueMutex.Lock();
	    m_bDropping = false;
	    locked = true;
	}
	framepos_t posbef = m_pVideostream->GetPos();

	kpos = m_pVideostream->SeekToKeyFrame(npos);
	if (kpos != npos)
	{
	    AVM_WRITE("aviplay", 0, "AviPlayer::dropFrame()  logical fault compare %d  %d  %d  %d "
		      "before %d\n", npos, kpos, orig, m_pVideostream->GetPos(), posbef);
	    //abort();
            kpos = orig;
	}

	//if (nt > 0.1 || kpos >= npos || npos == m_pVideostream->ERR)
	break;
    }

    if (kpos > orig && kpos != m_pVideostream->ERR)
    {
	ret = true;
	// and finaly we we have keyframe which is
	// is not to ahead of audio
	AVM_WRITE("aviplay", 0, "AviPlayer::dropFrame() skipped  %d frames  ( %d, %d )\n",
		     (kpos - orig), orig, kpos);
	while (orig++ < kpos)
	{
	    m_Drop.insert(100.0);
	    m_iFrameDrop++;
	}
    }
    //printf("DDD  %d   %f   %d\n", ret, getVideoAsync(), m_pVideostream->GetBuffering());
    if (!ret)
    {
	uint_t bm = 2 - (m_bVideoBuffered == 0);
        while (m_pVideostream->GetBuffering() > bm
	       && getVideoAsync() < 0.0)
	{
	    CImage *im = m_pVideostream->GetFrame(!m_bVideoBuffered);
	    if (!im)
                break;
	    im->Release();
	    m_Drop.insert(100.0);
	    m_iFrameDrop++;
	    AVM_WRITE("aviplay", 1, "Dropped video frames: %d  atime: %f   vtime: %f  bufs: %d\n",
		      m_iFrameDrop,
		      ((m_pAudioRenderer) ? m_pAudioRenderer->GetTime() : 0.0),
		      m_pVideostream->GetTime(),
		      m_pVideostream->GetBuffering());
	    ret = true;
	}
    }

    if (locked)
    {
	m_ThreadCond[THREAD_DECODER].Broadcast();
	m_ThreadMut[THREAD_DECODER].Unlock();
    }

    return ret;
}

void AviPlayer::setQuality()
{
    if (!m_bQualityAuto || m_iMaxAuto < 0)
	return;

    IRtConfig* rt = GetRtConfig(VIDEO_CODECS);
    if (rt)
    {
        static const char* ppstr = "postprocessing";
	int val;
	const CodecInfo& info = GetCodecInfo();
	if (CodecGetAttr(info, "maxauto", &m_iMaxAuto) < 0)
	    return;

	rt->GetValue(ppstr, &val);
	float as = getVideoAsync();
	uint_t bs;
	int bf = m_pVideostream->GetBuffering(&bs);

        //if (!m_dVframetime)
	//printf("As: %f  Max  %d   PP: %d  b: %d    %f   %f\n", as, m_iMaxAuto, val, bf, m_fDecodingTime, m_dVframetime);

	// some tricky checks here -
	//  it must work with buffering or even just with one buffer
        //  and should prevent large drops with high postprocessing levels
	if (val < m_iMaxAuto && as > m_fDropLimit
	    && (m_fDecodingTime < m_dVframetime * (1.0 - (3 - bf) / 10.0)))
	{
	    rt->SetValue(ppstr, val + 1);
	    //printf("+++++++++++\n");
	}
	else if (val > 0
		 && ((m_fDecodingTime > m_dVframetime * ((bf >= 2) ? 1.1 : 0.7))
		     || val > m_iMaxAuto
		     || as < m_fDropLimit)
		)
	{
	    //printf("As: %f  Max  %d   PP: %d  b: %d    %f   %f\n", as, m_iMaxAuto, val, bf, m_fDecodingTime, m_dVframetime);
	    if (as < m_fDropLimit || m_fDecodingTime > m_dVframetime * ((bf < 2) ? 1.0 : 1.4))
	    {
		val = 1;
	    }
	    rt->SetValue(ppstr, val - 1);
	    //printf("-----------\n");
	}
    }
}

void* AviPlayer::videoThread()
{
    const ThreadId th = THREAD_VIDEO;
    m_ThreadMut[th].Lock();
    AVM_WRITE("aviplay", 1, "Thread video started\n");

#ifdef SCHED_FIFO
    changePriority("Video player", 0, SCHED_FIFO);
#endif
    int mga_vid = open("/dev/mga_vid", O_RDONLY);
    if (mga_vid == -1)
	mga_vid = open("/dev/misc/mga_vid", O_RDONLY);
#define MINWAIT 0.0049
    float minWait = MINWAIT + 0.0001f;
    float lastCopyTime = 0.0;
    int framecnt = 0;
    double dLastEofTime = 0.0;
    int lastirq = 0;
    m_lLastVideoSync = longcount();
    m_dLastFrameStart = m_pVideostream->GetTime();

    if (mga_vid != -1)
    {
	if (ioctl(mga_vid, MGA_VID_IRQ_ON, 0) < 0)
	//if (ioctl(mga_vid, MGA_VID_IRQ2_ON, 0) < 0)
	{
            AVM_WRITE("aviplay", "Can't use MGA_VID device for VBI (vsync) synchronization (incompatible)\n");
            // not supported
	    close(mga_vid);
            mga_vid = -1;
	}
	else
	{
	    minWait = 0.02; // better precision with interrupt handler
	    AVM_WRITE("aviplay", "Using MGA_VID device for VBI (vsync) synchronization\n");
	}
    }

    // video synchronization
    while (checkSync(th))
    {
	//printf("V %d  %f  %f   as:%f   %d\n", m_pVideostream->Eof(), m_dLastFrameStart, dLastEofTime, getVideoAsync(), m_pAudioRenderer->Eof());
	if (m_pVideostream->Eof())
	{
	    if (m_dLastFrameStart != dLastEofTime)
		AVM_WRITE("aviplay", "Video stream eof\n");
	    m_Drop.clear();
            dLastEofTime = m_dLastFrameStart;
	    m_ThreadCond[th].Wait(m_ThreadMut[th], 1.0);
	    continue;
	}

	float async = getVideoAsync();
	//printf("ASYNC %f   time: %f   %f\n", async, m_pVideostream->GetTime(), m_pAudioRenderer->GetTime());
	if (async > minWait)
	{
	    if (mga_vid >= 0)
	    {
		int buf[3];
		read(mga_vid, &buf, sizeof(buf));
		// always wait for at least one VBI event
		// if (getVideoAsync() <= minWait) { //AVM_WRITE("aviplay", "VID: Diff: %d  %d  async:%f\n", buf[0] - lastirq, buf[1], getVideoAsync()); lastirq = buf[0]; }
	    }
	    else
	    {
		if (async > 0.5f)
		    async = 0.4f;        // max sleep time

		int64_t t1 = longcount();
		m_ThreadCond[th].Wait(m_ThreadMut[th], async - MINWAIT);
		float oversleep = to_float(longcount(), t1);
		m_fLastSleepTime += oversleep;
		if ((oversleep - async) > 0.016)
		{
		    AVM_WRITE("aviplay", 1, " Sleep too long!!!  real: %f    expected: %f  async: %f   over: %f\n", m_fLastSleepTime, async, getVideoAsync(), oversleep);
		}
	    }
	    continue; // let's check the status after sleep
	}
#undef MINWAIT
	m_QueueMutex.Lock();
	if (m_bVideoBuffered)
	{
	    uint_t bufsize;
	    int buffull = m_pVideostream->GetBuffering(&bufsize);
	    //printf("BUFFfull %f %d  : %d\n", getVideoAsync(), buffull, bufsize);
	    if (buffull < 1)
	    {
		m_ThreadMut[th].Unlock();
		///AVM_WRITE("aviplay", "wait...\n");
		m_QueueCond.Broadcast();
		m_QueueCond.Wait(m_QueueMutex);
		//buffull = m_pVideostream->GetBuffering(&bufsize);
		//printf("AfterwaitBUFFfull %d  : %d\n", buffull, bufsize);
		m_QueueMutex.Unlock();
		m_ThreadMut[th].Lock();
		continue;
	    }
	}
	else // !m_bVideoBuffered
	    setQuality();

	m_lLastTimeStart = longcount();
	//printf("BUFFfull %d\n", m_pVideostream->GetBuffering());
	CImage *im = m_pVideostream->GetFrame(!m_bVideoBuffered);
	//printf("NEWFT  t:%f  p:%d  %d\n", m_pVideostream->GetTime(), m_pVideostream->GetPos(), m_pVideostream->GetBuffering());

        // read time for the next frame
	double nf = m_pVideostream->GetTime() - m_dLastFrameStart;
	m_dVframetime = nf;

	if (!m_bVideoBuffered)
	{
	    //avm_usleep(40000); // debug drop
	    m_fDecodingTime = to_float(longcount(), m_lLastTimeStart);
	}

	if (im)
	{
	    //printf("DRATM  %f\n", (lastCopyTime + m_fLastSyncTime));
	    // FIXME - this bool is stupid
	    // condition: - no drop if there is just small async
	    // or if its odd frame or rendering time is quite small
	    if (!m_bVideoMute
		&& (async > -0.1 || !m_bDropping
		    || (framecnt++ & 1)
		    ))//|| (lastCopyTime + m_fLastSyncTime) < 0.007))
	    {
		//printf("D %f\n", lastCopyTime + m_fLastSyncTime);
		m_Quality.insert(im->GetQuality() * 100.0);
		m_Drop.insert(0.0);
		m_iFramesVideo++;
		const subtitle_line_t* sl = GetCurrentSubtitles();
		// unlock here - as it could stop decoding process
		// because of block in xfree, but we do not need to
		// send signal as decoder will be waken by drop in that case
                // invoked in audio thread
		m_QueueMutex.Unlock();
		int64_t copys = longcount();
		for (unsigned i = 0; i < m_VideoRenderers.size(); i++)
		{
		    m_VideoRenderers[i]->Draw(im);
		    if (HasSubtitles())
			m_VideoRenderers[i]->DrawSubtitles(sl);
		    //m_VideoRenderers[i]->Sync(); // syncFrame() executed elsewhere
		}
		lastCopyTime = to_float(longcount(), copys); // copy time
		m_bCallSync = true;
		// copy image to graphics card
		syncFrame();
		m_QueueMutex.Lock();
	    }
	    else
	    {
		m_Drop.insert(100.0);
		m_iFrameDrop++;
		//printf("Ignore %f  as: %f - %f\n", m_Drop.average(), async, getVideoAsync());
	    }

	    im->Release();
	}
	m_dLastFrameStart += m_dVframetime; // this will not accumulate error
	if (getVideoAsync() < m_fDropLimit)
	    dropFrame();

	m_QueueCond.Broadcast();
	m_QueueMutex.Unlock();
	m_fLastSleepTime = 0.0;
    }

    if (mga_vid != -1)
    {
	//ioctl(mga_vid, MGA_VID_IRQ_OFF, 0);
        close(mga_vid);
    }

    AVM_WRITE("aviplay", 1, "Thread video finished\n");
    return 0;
}

void* AviPlayer::audioThread()
{
    const ThreadId th = THREAD_AUDIO;
    m_ThreadMut[th].Lock();
    AVM_WRITE("aviplay", 1, "Thread audio started\n");

    changePriority("Audio decoder ", PRIORITY_ADD_AUDIO);

    bool paused = false;
    while (checkSync(th))
    {
	if (!m_pAudioRenderer)
	{
	    m_ThreadCond[th].Wait(m_ThreadMut[th], 0.1);
            continue;
	}

	float async = getVideoAsync();
	float buftime = m_pAudioRenderer->GetBufferTime();

	if (!m_bVideoAsync && async < -0.5
	    && to_float(longcount(), m_lTimeStart) > 1.0
	    && (!m_pVideostream || !m_pVideostream->Eof()))
	{
	    if (!paused && async < -10.0)
	    {
		// ok even with dropping we could not stay in sync
		// so for a while turn of audio track and wait
                // until video will match audio time
		AVM_WRITE("aviplay", 1, "Thread audio large async time: %f  buftime: %f - pause\n", async, buftime);
                    // if the video stream has finished sooner than audio
		m_pAudioRenderer->Pause(true);
		paused = true;
	    }
	    if (m_pVideostream && m_QueueMutex.TryLock() == 0)
	    {
		dropFrame();
		m_QueueCond.Broadcast();
		m_QueueMutex.Unlock();
	    }
	}
	else if (paused && !m_pAudioRenderer->Eof()
		 && (async > 0.0
		     || (!m_pVideostream || m_pVideostream->Eof())))
	    paused = (m_pAudioRenderer->Pause(false) != 0);

	//AVM_WRITE("aviplay", "atime:  %f    btime: %f\n", async, buftime);
	int r = m_pAudioRenderer->Extract();
	if (r < 0)
	{
	    if (m_pAudioRenderer->Eof())
	    {
		// FIXME FIXME - this is a big mess - fix
		if (!paused)
		{
		    m_pAudioRenderer->Pause(true);
		    paused = true;
		    AVM_WRITE("aviplay", "Audio stream eof\n");
		}
	    }

	    // either we have enough prebuffered sound or audio renderer
            // is unable to extract new samples - wait for a while
	    m_ThreadCond[th].Wait(m_ThreadMut[th], 0.1);
	}
	else if (m_pAudioRenderer->GetBufferTime() - buftime > 0.02)
	{
	    // just a small trick - so the buffers will not be filled
	    // so fast when seeking - it's better for ffrwd
	    //AVM_WRITE("aviplay", "JO  %f  %f\n", buftime, to_float(longcount(), m_lTimeStart));
	    m_ThreadCond[th].Wait(m_ThreadMut[th], 0.01);
	}
    }

    AVM_WRITE("aviplay", 1, "Thread audio finished\n");
    return 0;
}

void* AviPlayer::decoderThread()
{
    const ThreadId th = THREAD_DECODER;

    m_ThreadMut[th].Lock();
    // thread started only with (stream != NULL)
    AVM_WRITE("aviplay", 1, "Thread decoder started\n");

    changePriority("Video decoder ", PRIORITY_ADD_VIDEO);

    if (geteuid() == 0)
	AVM_WRITE("aviplay", "!!!Running video decoder as ROOT - Dangerous!!!\n");

    while (checkSync(th))
    {
	// if we are after seek - leave some time for the user
	// if he will want to seek more there is no need to
        // precache frames at this point
	if (m_bVideoBuffered && !m_bDropping && m_lTimeStart && to_float(longcount(), m_lTimeStart) > 0.05)
	{
	    m_QueueMutex.Lock();
            uint_t bs;
	    uint_t b = m_pVideostream->GetBuffering(&bs);
	    //AVM_WRITE("aviplay", "Bdecoder  %d  %d  %f\n", b, bs, to_float(longcount(), m_lTimeStart));
	    // smart condition - do not stop with one-buffer
	    if (b >= (bs - 1) && b > 0)
	    {
		//int64_t tin = longcount();
		m_ThreadMut[th].Unlock();
		//AVM_WRITE("aviplay", "MUTEXWAIT  %d\n", b);
		m_QueueCond.Broadcast();
		m_QueueCond.Wait(m_QueueMutex);
		m_QueueMutex.Unlock();
		m_ThreadMut[th].Lock();
		//float td = to_float(longcount(), tin);

		//b = m_pVideostream->GetBuffering(&bs);
		//AVM_WRITE("aviplay", "Bdecoder  %d  %d\n", b, bs);
		//AVM_WRITE("aviplay", "FREETM %f\n", td);
		continue; // recheck m_bVideoBuffered
	    }
	    setQuality();
	    m_QueueMutex.Unlock();

	    int64_t tin = longcount();
	    //avm_usleep(40000); // debug drop
	    //AVM_WRITE("aviplay", "readin %f\n", m_pVideostream->GetTime());
	    int hr = m_pVideostream->ReadFrame(true);
	    m_fDecodingTime = to_float(longcount(), tin);
	    //AVM_WRITE("aviplay", "readout  %d   %f   %f\n", hr, m_pVideostream->GetTime(), m_fDecodingTime);

	    m_QueueMutex.Lock();
	    m_QueueCond.Broadcast();
	    m_QueueMutex.Unlock();

	    if (hr >= 0)
		continue; // frame is OK -> next frame
	}

	m_ThreadCond[th].Wait(m_ThreadMut[th], 0.1);
    }

    AVM_WRITE("aviplay", 1, "Thread decoder finished\n");
    return 0;
}

void AviPlayer::changePriority(const char* taskName, int addPriority, int schedt)
{            return ;
#if !defined(__NetBSD__) && !defined(__FreeBSD__)
    if (addPriority != 0)
    {
        if (!taskName)
            taskName = "";
#if 1
	int p = getpriority(PRIO_PROCESS, 0) + addPriority;
	if (p > 20)
	    p = 20;
	setpriority(PRIO_PROCESS, 0, p);
	AVM_WRITE("aviplay", "%s thread lowered priority to %d\n", taskName, getpriority(PRIO_PROCESS, 0));
#else
	// this code doesn't work on Linux!
	// it looks like pthread priority is completely ignored and the only
        // way is to call 'setpriority' as you see above

	int policy;
	struct sched_param sched;
	pthread_getschedparam(pthread_self(), &policy, &sched);
	sched.sched_priority += addPriority;
	pthread_setschedparam(pthread_self(), policy, &sched);
	AVM_WRITE("aviplay", "%s using priority: %d\n", taskName, sched.sched_priority);
#endif
    }

    if (schedt != 0 && m_iEffectiveUid == 0)
    {
        // we could use FIFO scheduler with root access privilegies
        seteuid(m_iEffectiveUid);
        setegid(m_iEffectiveGid);

	static const char * const sched_txt[] = {
	    "SCHED_OTHER", "SCHED_FIFO", "SCHED_RR"
	};
	struct sched_param schedRec;

	if (sched_getparam(0, &schedRec) != 0) {
	    perror("sched_getparam");
	    return;
	}

	schedRec.sched_priority = sched_get_priority_min(schedt);

	if (sched_setscheduler(0, schedt, &schedRec) != 0) {
	    perror("sched_setscheduler");
	    return;
	}

	AVM_WRITE("aviplay", "Video thread - using scheduler: %s\n", sched_txt[sched_getscheduler(0)]);
	setuid(getuid());
        setgid(getgid());
    }
#endif // __NetBSD__  & __FreeBSD__
}

void AviPlayer::syncFrame()
{
    if (m_pVideostream && !m_bVideoMute && m_bCallSync)
    {
	m_bCallSync = false;
        int64_t newsync = longcount();
	// blit image on screen
	for (unsigned i = 0; i < m_VideoRenderers.size(); i++)
	    m_VideoRenderers[i]->Sync();
	// splited into two parts so we do not create jumping images
	// even in the debug mode
	int64_t newVideo = longcount();
	float newDiff = to_float(newVideo, m_lLastVideoSync);
        m_fLastSyncTime = to_float(newVideo, newsync);
	{
	    // after check of ahead might be more presice time for this
	    // however we would have to play some with pausing
	    // so we currently place the image right  here
	    //double d = to_float(newVideo, lastVideo);
	    double newAudio = (m_pAudioRenderer) ? m_pAudioRenderer->GetTime() : 0.0;
	    //if (d < 0.027 || d > 0.043)
	    AVM_WRITE("aviplay", 1, "A-V sync: %+8.4f   sleep:%5.1f  Vd:%5.1f  Ad:%5.1f  VSync:%5.1f  %.3f\n",
		      m_dLastFrameStart - newAudio,
		      m_fLastSleepTime * 1000.0,
		      to_float(newVideo, m_lLastVideoSync) * 1000.0,
		      (newAudio - m_dLastAudioSync) * 1000.0,
		      m_fLastSyncTime * 1000.0,
		      m_dLastFrameStart
		     );

	    float d = newDiff - m_fLastDiff;
	    if (d < 0.0)
                d = -d;

	    m_dLastAudioSync = newAudio;
	}

	m_lLastVideoSync = newVideo;
	m_fLastDiff = newDiff;
    }
}

AVM_END_NAMESPACE;
