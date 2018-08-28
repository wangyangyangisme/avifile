#include "recwnd.h"
#include "recwnd.moc"

#include "recompressor.h"

#include <qprogressbar.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qtabwidget.h>

#if QT_VERSION > 220
//#include <qthread.h>
#endif

#include <stdio.h>

Progress::Progress(unsigned int size)
    :m_pValues(0), m_pValuesFlags(0), m_uiPos(0),
    m_uiSize(size), m_bValidMax(false)
{
}

Progress::~Progress()
{
    delete[] m_pValues;
    delete[] m_pValuesFlags;
}

void Progress::insert(unsigned int v, int flags)
{
    if (!m_pValues)
    {
	m_pValues = new unsigned int[m_uiSize];
	m_pValuesFlags = new int[m_uiSize];
        memset(m_pValues, 0, sizeof(unsigned int) * m_uiSize);
    }

    if (m_pValues[m_uiPos] == m_uiMax)
	m_bValidMax = false;

    m_pValues[m_uiPos] = v;
    m_pValuesFlags[m_uiPos] = flags;
    m_uiPos = (m_uiPos + 1) % m_uiSize;
    // cout << "Insert " << v << ", " << flags << "  p: " << m_uiPos << endl;
}

unsigned int Progress::getMax()
{
    if (!m_bValidMax)
    {
	m_uiMax = 0;
	for (unsigned i = 0; i < m_uiSize; i++)
	    if (m_pValues[i] > m_uiMax)
		m_uiMax = m_pValues[i];
        m_bValidMax = true;
    }
    return m_uiMax;
}


/*
 *  Constructs a RecWindow which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
RecWindow::RecWindow( QWidget* parent,  RecKernel* kern )
    : RecWnd_p( parent, "Record window", true, 0 ), kernel(kern),
    m_Progress(PROGRESS_SIZE)
{
    progress = 1;
    elapsed = estimated = 1;
    fsize = 0;
    curVideoFrame = 0;
    curAudioSample = 0;
    estimated = 0;
    fsize = 0;
    videoSize = audioSize = 0;
    totalAudioSamples = totalVideoFrames = 0;
    kernel->setCallback(this);
    if (kernel->startRecompress())
	accept();
}

/*
 *  Destroys the object and frees any allocated resources
 */
RecWindow::~RecWindow()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 * public slot
 */
void RecWindow::cancelProcess()
{
    kernel->stopRecompress();
    accept();
}
/*
 * public slot
 */
void RecWindow::pauseProcess()
{
    if (kernel->pauseRecompress() != 0)
	m_pPause->setText( tr( "&Resume" ) );
    else
	m_pPause->setText( tr( "&Pause" ) );
}
/*
 * public slot
 */
void RecWindow::currentChanged(QWidget* w)
{
#if QT_VERSION >= 300
    //cout << "INDEX " << m_pTabWidget->indexOf(w) << endl;
#endif
}


void RecWindow::finished()
{
    //QThread::postEvent(this, new QEvent(QEvent::Type(QEvent::User+1)));
    qApp->postEvent(this, new QEvent(QEvent::Type(QEvent::User+1)));
#if QT_VERSION > 220
    qApp->wakeUpGuiThread();
#endif
}

void RecWindow::setNewState(double progress, double elapsed, int64_t fsize)
{
    this->progress=progress;
    this->elapsed=elapsed;
    this->estimated = (progress > 0.0001) ? elapsed / progress : 0.0;
    this->fsize=fsize;

    //QThread::postEvent(this, new QEvent(QEvent::User));
    qApp->postEvent(this, new QEvent(QEvent::User));
#if QT_VERSION > 220
    qApp->wakeUpGuiThread();
#endif
    //printf("seupdate2\n");
/*
    if (progress > 0) qApp->sendPostedEvents();

#if QT_VERSION >= 300
    qApp->flush();
#else
    qApp->flushX();
    #endif
    */
}

void RecWindow::setTotal(framepos_t vtotal, framepos_t atotal, double vtm, double atm)
{
    //printf("SETTOTAL  vt:%d  at:%d   vtm:%f  atm%f\n", vtotal, atotal, vtm, atm);
    totalVideoFrames = vtotal;
    totalAudioSamples = atotal;
    startVideoTime = vtm - 1.0;
    startAudioTime = vtm - 1.0;
}

void RecWindow::addVideo(framepos_t vframe, unsigned int vsize, double vtm, bool keyframe)
{
    curVideoFrame = vframe;
    curVideoTime = vtm;
    videoSize += vsize;
    m_Progress.insert(vsize, keyframe);
}

void RecWindow::addAudio(framepos_t asample, unsigned int asize, double atime)
{
    curAudioSample = asample;
    audioSize += asize;
    curAudioTime = atime;
}

bool RecWindow::event(QEvent* e)
{
    if (e->type()==QEvent::User)
    {
        //printf("update1\n");
	return update();
    }
    if (e->type()==QEvent::Type(QEvent::User+1))
    {
	m_pPause->hide();
        m_pStop->setText( tr( "&Close" ) );
        update();
	//accept();
#if QT_VERSION > 220
	qApp->wakeUpGuiThread();
#endif
	//return true;
    }
    return QWidget::event(e);
}

static inline char* sprintfTimeString(char* buf, double tm)
{
    sprintf(buf, "%d:%.02d:%.02d", int(tm/3600),
	    int(tm/60)%60, int(tm)%60);
    return buf;
}

bool RecWindow::update()
{
    m_pProgress->setProgress(int(progress*1000));

#if QT_VERSION > 220
    if (m_pTabWidget->currentPageIndex() == 1)
	return updateGraphs();
#endif

    char s[256];
    char buf[20];
    double proj = fsize / progress;
    for (int i = 0; i < LAST_LABEL; i++)
    {
	switch (i)
	{
	case CURRENT_VIDEO_FRAME:
	    sprintf(s, "%d/%d", curVideoFrame, totalVideoFrames);
	    break;
	case CURRENT_AUDIO_SAMPLE:
	    sprintf(s, "%d/%d", curAudioSample, totalAudioSamples);
	    break;
	case VIDEO_DATA:
	    sprintf(s, "%d KB (%d kbps)", int(videoSize/1024LL),
		    int(videoSize * 8 / 1000LL/(curVideoTime - startVideoTime)));
            break;
	case AUDIO_DATA:
	    //printf("SURA %f    SA %f   lsize %lld\n", curAudioTime, startAudioTime, audioSize);
	    sprintf(s, "%d KB (%d kbps)", int(audioSize/1024LL),
		    int(audioSize * 8 / 1000LL/(curAudioTime - startAudioTime)));
	    break;
	case CURRENT_FILE_SIZE:
	    sprintf(s, "%lld", fsize);
	    break;
	case PROJECTED_FILE_SIZE:
	    if (proj > (10 * 1024 * 1024))
		sprintf(s, "%.3f MB", proj/(1024*1024LL));
	    else
		sprintf(s, "%d KB", int(proj/1024LL));
	    break;
	case VIDEO_RENDERING_RATE:
	    sprintf(s, "%.3f fps", curVideoFrame / elapsed);
	    break;
	case TIME_ELAPSED:
	    sprintfTimeString(s, elapsed);
	    break;
	case TOTAL_TIME_ESTIMATED:
	    sprintfTimeString(s, estimated);
	    break;
	}
	m_pText[i]->setText(s);
    }
    return true;
}

bool RecWindow::updateGraphs()
{
    // for this moment just some experimental code
    //cout << "******MAX " << m_Progress.getMax() << endl;
    char pcElapsed[20];
    char pcEstimated[20];
    sprintfTimeString(pcElapsed, elapsed);
    sprintfTimeString(pcEstimated, estimated);

    printf("%3.2f\%%, elapsed: %s, remaining: %s, "
	   "estimated file size: %d KB\n",
	   progress*100, pcElapsed, pcEstimated,
	   int(fsize/progress/1024LL));

#if 0
    char s[1024];
    static int call=0;
    int iHeight;
    int iWidth;
    int iMaxFrame=0;
    int i;
    call+=10;
    m_pProgress->lock();

// 2 because we have buffer of 50 frames
    2*m_pProgress->buffer_occupancies[(m_pProgress->buffer_head+m_pProgress->history_size-1)%m_pProgress->history_size]
    );

    QPainter* qp = new QPainter(Frame3_2);
    iHeight=Frame3_2->height();
    iWidth=Frame3_2->width();

    iMaxFrame= m_pProgress->getMax()

    if (iMaxFrame<10)
	iMaxFrame=10;
    else if (iMaxFrame<100)
	iMaxFrame=100;
    else if (iMaxFrame<1000)
	iMaxFrame=1000;
    else if (iMaxFrame<10000)
	iMaxFrame=10000;
    else if (iMaxFrame<100000)
	iMaxFrame=100000;
    else iMaxFrame=1000000;
    if(iMaxFrame<1000)
	sprintf(s, tr( "%d bytes" ), iMaxFrame);
    else if(iMaxFrame<1000000)
	sprintf(s, tr( "%d KB" ), iMaxFrame/1000);
    else
	sprintf(s, tr( "%d MB" ), iMaxFrame/1000000);
    MaxFrameSizeText->setText(s);
    for(i=iWidth; i>=0; i--)
    {
	int index = i-iWidth;
	if(index<-m_pProgress->video_frame_len)
	{
	    qp->setPen(QColor(0,0,0));
	    qp->moveTo(i, iHeight);
	    qp->lineTo(i, 0);
	    continue;
	}
	index += m_pProgress->video_frame_head;
	while(index<0)
	    index += m_pProgress->history_size;
	index %= m_pProgress->history_size;
	float t=(m_pProgress->video_frame_sizes[index] & ~0x40000000)/(float)iMaxFrame;
	if(t>1)t=1;
	if(m_pProgress->video_frame_sizes[index] & 0x40000000)
	    qp->setPen(QColor(255, 0, 0));
	else
	    qp->setPen(QColor(0, 255, 0));
	qp->moveTo(i, iHeight);
	qp->lineTo(i, iHeight*(1-t));
	if(t<1)
	{
	    qp->setPen(QColor(0, 0, 0));
	    qp->lineTo(i, 0);
	}
    }
finish:
    m_pProgress->unlock();
#endif
    return true;
}
