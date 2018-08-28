
#include "avicapwnd.h"
#include "avicapwnd.moc"

#include "v4lwindow.h"
#include "ccap.h"
#include "capproc.h"

#include <avm_except.h>
#include <avm_cpuinfo.h>
#include <avm_fourcc.h>
#include <avm_output.h>
#include <utils.h>

#define DECLARE_REGISTRY_SHORTCUT
#include <configfile.h>
#undef DECLARE_REGISTRY_SHORTCUT

#include <qpushbutton.h>
#include <qlabel.h>
#include <qtabwidget.h>
#include <qtimer.h>
#include <qframe.h>
#include <qpainter.h>
#include <qmessagebox.h>
#include <qdatetime.h>

#include <math.h>
#include <stdio.h>
#include <stdio.h>
#include <libgen.h>
//#include <sys/vfs.h>

extern int free_diskspace(avm::string path);
extern QString find_best_dir();
extern int find_keepfree(QString dirname);


/*
 *  Constructs a AviCapDialog which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 */
AviCapDialog::AviCapDialog( V4LWindow* pWnd ,CaptureConfig *conf)
    : AviCapDlg( 0 ),
    m_pWnd(pWnd), m_pProgress(0), m_bStarted(false)
{
    my_config=conf;
    last_dropped_enc=0;
    last_dropped_cap=0;
    last_synchfix=0;
    last_synchfix_time=0;
    use_dirpool=false;
    i_am_mini=false;

    connect(m_pMiniButton, SIGNAL(clicked()), this, SLOT(mini()));
    connect(MiniButton, SIGNAL(clicked()), this, SLOT(mini()));
    connect(CloseButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(StartButton, SIGNAL(clicked()), this, SLOT(start()));
    connect(SegmentButton, SIGNAL(clicked()), this, SLOT(segment()));
    SegmentButton->setEnabled(false);
    ProgressText->setText( tr( "Press START to begin capturing" ) );
    m_pTimer = new QTimer;
    m_pCapproc = 0;
    m_pCC = pWnd->getcc();
    if (m_pCC)
	m_pCC->addref();
}

/*
 *  Destroys the object and frees any allocated resources
 */
AviCapDialog::~AviCapDialog()
{
    delete my_config;
    delete miniwidget; // thats not a child of avicapdialog
}


QString make_dirpool_file(QString filename)
{

	  QDate cdate=QDate::currentDate();
	  QTime ctime=QTime::currentTime();
	  QString timestr=QString().sprintf("-%02d%02d%02d-%02d%02d%02d.avi", \
				     cdate.year()%1000,cdate.month(),cdate.day(), \
				     ctime.hour(),ctime.minute(),ctime.second());
	if(filename==""){
	  filename="avicap"+timestr;
	}
	else if(filename.length()>4 && filename.right(4)!=".avi"){
	  //doesnt end on .avi
	  filename=filename+timestr;
	}
	else{
	  // no change in filename
	}
	QString fullname;
	QString pathname=find_best_dir();
	if(pathname==""){
	  fullname="";
	}
	else{
	  // make it easy for now
	  fullname=pathname+"/"+filename;
	}

	return fullname;
}
void AviCapDialog::start()
{
    m_eOldMode = (int)m_pWnd->getMode();
    if (m_eOldMode != V4LWindow::Overlay)
    {
	printf("setting overlay mode\n");
	m_pWnd->setMode(V4LWindow::Overlay);
	avm_usleep(50000);
    }
    disconnect(StartButton, SIGNAL(clicked()), this, SLOT(start()));
    StartButton->setText( tr( "Stop" ) );
    connect(StartButton, SIGNAL(clicked()), this, SLOT(stop()));

    if (!my_config)
    {
	my_config = new CaptureConfig();
	my_config->load();
    }

    if (my_config->filename[0]=='/' || my_config->filename[0]=='.'){
	// relative or absolute path, no modifications
	use_dirpool=false;
    }
    else{
	use_dirpool=true;

	QString filename=QString(my_config->filename.c_str());
	QString fullname=make_dirpool_file(filename);
	if (!fullname.length())
	{
	    avml(AVML_DEBUG1, "no more free space - not recording\n");
	    m_pCapproc=0;
	    return;
	}

	my_config->setFilename(fullname.latin1());
    }//else of absolute/rel path

    try
    {
	m_pCapproc = new CaptureProcess(m_pWnd->m_pDev, my_config, m_pCC);
	avml(AVML_DEBUG1, "capture process started\n");
    }
    catch (FatalError& e)
    {
	QMessageBox::critical(this, "Error", e.GetDesc());
	disconnect(StartButton, SIGNAL(clicked()), this, SLOT(stop()));
	connect(StartButton, SIGNAL(clicked()), this, SLOT(start()));
	m_pCapproc = 0;
	avml(AVML_DEBUG1, "critical error when starting capture\n");
	return;
    }
    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(updateGraphs()));
    m_pTimer->start(500);
    if (RI("IsSegmented", 1))
	SegmentButton->setEnabled(true);
    m_pProgress=new CaptureProgress;
    ProgressText->setAlignment( Qt::AlignVCenter );
    m_pCapproc->setMessenger(m_pProgress);
    m_bStarted=true;
}

void AviCapDialog::segment()
{
    if (m_pCapproc){
	avml(AVML_DEBUG1, "segmenting capture\n");
	m_pCapproc->segment();
    }
}

void AviCapDialog::stop()
{
    m_pTimer->stop();
    m_bStarted=false;
    delete m_pCapproc;
    delete m_pProgress;
    m_pCapproc=0;
    m_pProgress=0;
    disconnect(StartButton, SIGNAL(clicked()), this, SLOT(stop()));
    connect(StartButton, SIGNAL(clicked()), this, SLOT(start()));
    SegmentButton->setEnabled( false );
    StartButton->setText( tr( "Start" ) );
    m_pWnd->setMode((V4LWindow::Modes)m_eOldMode);
    avml(AVML_DEBUG1, "capture stopped\n");
}

void AviCapDialog::updateGraphs()
{
    QPainter* qp;
    int iHeight;
    int iWidth;
    int iMaxFrame=0;
    int i;

    m_pProgress->lock();

    QString s = QString().
	sprintf(tr("Capturing into file:\n %s\n") +
		tr("Video frame rate: %.2f frames/s\n") +
		tr("Video resolution: %dx%d\n"),
		m_pProgress->filename,
		m_pProgress->framerate,
		m_pProgress->xdim, m_pProgress->ydim);

    if (m_pProgress->audio_freq)
	s += QString().sprintf(tr("Audio: %d Hz, %d bit, %s\n"),
			       m_pProgress->audio_freq,
			       m_pProgress->audio_ssize,
			       (m_pProgress->audio_channels==1)?"mono":"stereo");
    else
	s += QString().sprintf(tr("No audio\n"));

    float elapsed_float=m_pProgress->enc_time/freq/1000.0;
    QTime elapsed = QTime(0,0,0,0);
    elapsed = elapsed.addSecs((int)elapsed_float);
#if QT_VERSION>=300
    QString elapsedstr = elapsed.toString("hh:mm:ss");
#else
    QString elapsedstr = elapsed.toString();
#endif

    int free_space = free_diskspace(m_pProgress->filename);

    if (use_dirpool && (((int)elapsed_float)%60==0)){
	char buffer[100];
	strcpy(buffer,m_pProgress->filename);
	char *dname=dirname(buffer);
	QString dirname=QString(dname);

	strcpy(buffer,m_pProgress->filename);
	char *bname=basename(buffer);
	QString filename=QString(bname);

#if 0
      printf("new segmenting name: %s\n",new_segmenting_name.latin1());
      m_pCapproc->setSegmentName(new_segmenting_name.latin1());
      //UserLog(QString().sprintf("new segmenting name %s\n",new_segmenting_name.latin1()));


      int keepfree=find_keepfree(dirname);

      printf("keepfree=%d  freespace=%d\n",keepfree,free_space);

      if(keepfree==0 || keepfree>free_space){
	printf("out of free space - segmenting to another dir\n");
	segment();
      }
#else
	QString fullname=make_dirpool_file(filename);
	if(fullname==""){
	    avml(AVML_WARN,
		 "error - no more free space in dirpool\n"
		 "stopping recording\n");
	    //stop recording
	    stop();
	    return;
	}
	QString new_segmenting_name=fullname;

	avml(AVML_DEBUG, "new segmenting name: %s\n",
	     new_segmenting_name.latin1());
	m_pCapproc->setSegmentName(new_segmenting_name.latin1());
	avml(AVML_DEBUG, "new segmenting name %s\n",
	     new_segmenting_name.latin1());

	int keepfree=find_keepfree(dirname);

	avml(AVML_DEBUG, "keepfree=%d  freespace=%d\n",
	     keepfree, free_space);

	if(keepfree==0 || keepfree>free_space){
	    printf("out of free space - segmenting to another dir\n");
	    segment();
	}
#endif
    }

    s+=QString().sprintf(
			 tr("\nElapsed: %s (%.2f s)\n")+
			 tr("Written video: \n")+
			 tr("   %d KB   %d frames (%d kbps) %.2f s\n")+
			 tr("Written audio: \n")+
			 tr("   %d KB    (%dkbps) %.2f s\n")+
			 tr("Synch fix: %d frames A:%.4f V:%.4f S:%.4f\n")+
			 tr("File size: %d KB  (%.1f MB)\n")+
			 tr("Free space on device: %d MB\n")+
			 tr("Frame drop in capture thread: %d frames\n")+
			 tr("Frame drop in encoder: %d frames\n")+
			 tr("Capture buffer usage: %d %%\n"),
			 elapsedstr.latin1(),
			 elapsed_float,
			 (int)(m_pProgress->video_bytes/1024),
			 (int) m_pProgress->video_frames,
			 (int)(m_pProgress->video_bytes * 8 / m_pProgress->video_time / 1000),
			 (double)m_pProgress->video_time,
			 (int)(m_pProgress->audio_bytes/1024),
			 int(m_pProgress->audio_bytes * 8 / m_pProgress->audio_time / 1000),(float) m_pProgress->audio_time,
			 m_pProgress->synch_fix,
			 (float)m_pProgress->audio_error,
			 (float)m_pProgress->video_error,
			 (float)m_pProgress->timestamp_shift,
			 (int)(m_pProgress->file_size/1024),
			 (int)((float)m_pProgress->file_size)/(1024.0*1024.0),
			 free_space,
			 m_pProgress->dropped_cap,
			 m_pProgress->dropped_enc,
			 int(100.*m_pProgress->buffer_occupancies[(m_pProgress->buffer_head+m_pProgress->HISTORY_SIZE-1)%m_pProgress->HISTORY_SIZE]/m_pProgress->max_frames)
			);
    ProgressText->setText(s);

#if 0
    struct rusage rus;

    int res1=getrusage(RUSAGE_SELF,&rus);

    long usertime=rus.ru_utime.tv_sec;
    long systime=rus.ru_stime.tv_sec;
#endif


    if ((((int)elapsed_float)%60)==0)
	avml(AVML_DEBUG, "capture is alive since %s\n", elapsedstr.latin1());

    if (last_dropped_enc!=m_pProgress->dropped_enc)
	avml(AVML_DEBUG, "dropped frames %s %s encoder=%3d (+%2d)\n",
	     m_pProgress->filename,elapsedstr.latin1(),
	     m_pProgress->dropped_enc,m_pProgress->dropped_enc-last_dropped_enc);

    if (last_dropped_cap!=m_pProgress->dropped_cap)
	avml(AVML_DEBUG, "dropped frames %s %s capture=%3d (+%2d)\n",
	     m_pProgress->filename, elapsedstr.latin1(),
	     m_pProgress->dropped_cap,m_pProgress->dropped_cap-last_dropped_cap);

    if (last_synchfix!=m_pProgress->synch_fix)
    {
	long timediff=((long)elapsed_float)-last_synchfix_time;
	QTime timed = QTime(0,0,0,0);
	timed = timed.addSecs((int)timediff);
	QString diffstr=timed.toString();

	avml(AVML_DEBUG, "synchfix %s %s synchfix=%3d (+%2d +%s)\n",
	     m_pProgress->filename,elapsedstr.latin1(),
	     m_pProgress->synch_fix,
	     m_pProgress->synch_fix-last_synchfix,diffstr.latin1());
	last_synchfix_time=(long)elapsed_float;
    }
    last_dropped_enc=m_pProgress->dropped_enc;
    last_dropped_cap=m_pProgress->dropped_cap;
    last_synchfix=m_pProgress->synch_fix;

    QString shorttext = QString().
	sprintf("%s %.fM %d+%d %d%%", elapsedstr.latin1(),
		((float)m_pProgress->file_size)/(1024.0*1024.0),
		m_pProgress->dropped_cap, m_pProgress->dropped_enc,
		int(100.*m_pProgress->buffer_occupancies[(m_pProgress->buffer_head+m_pProgress->HISTORY_SIZE-1)%m_pProgress->HISTORY_SIZE]/m_pProgress->max_frames));

    m_pMiniButton->setText(shorttext);
    miniwidget->setCaption(shorttext);

#if QT_VERSION > 220
    if(CaptureTab->currentPageIndex()!=1)
	goto finish;
#endif
    qp = new QPainter(Frame3_2);
    iHeight=Frame3_2->height();
    iWidth=Frame3_2->width();
    for(i=0; i<m_pProgress->video_frame_len; i++)
    {
	int index = m_pProgress->video_frame_head - i - 1;
	while(index<0)
	    index += m_pProgress->HISTORY_SIZE;
	index %= m_pProgress->HISTORY_SIZE;
	if((m_pProgress->video_frame_sizes[index] & ~0x40000000)>iMaxFrame)
	    iMaxFrame=m_pProgress->video_frame_sizes[index] & ~0x40000000;
    }
    if (iMaxFrame<10)
	iMaxFrame=10;
    else if (iMaxFrame<100)
	iMaxFrame=100;
    else if (iMaxFrame<3000)
	iMaxFrame=3000;
    else if (iMaxFrame<10000)
	iMaxFrame=10000;
    else if (iMaxFrame<30000)
	iMaxFrame=30000;
    else if (iMaxFrame<100000)
	iMaxFrame=100000;
    else iMaxFrame=1000000;
    if (iMaxFrame<1000)
	s=QString().sprintf(tr("%d bytes"), iMaxFrame);
    else if(iMaxFrame<1000000)
	s=QString().sprintf(tr("%d KB"), iMaxFrame/1024);
    else
	s=QString().sprintf(tr("%d MB"), iMaxFrame/(1024*1024));
    MaxFrameSizeText->setText(s);
    for (i=iWidth; i>=0; i--)
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
	    index += m_pProgress->HISTORY_SIZE;
	index %= m_pProgress->HISTORY_SIZE;
	float t=(m_pProgress->video_frame_sizes[index] & ~0x40000000)/(float)iMaxFrame;
	if (t>1)t=1;
	if (m_pProgress->video_frame_sizes[index] & 0x40000000){
	  //printf("avicapwnd keyframe\n");
	    qp->setPen(QColor(255, 0, 0));
	}
	else
	    qp->setPen(QColor(0, 255, 0));
	qp->moveTo(i, iHeight);
	qp->lineTo(i, (int) (iHeight*(1-t)));
	if (t<1)
	{
	    qp->setPen(QColor(0, 0, 0));
	    qp->lineTo(i, 0);
	}
    }
    delete qp;
    qp=new QPainter(Frame3);
    iHeight=Frame3->height();
    iWidth=Frame3->width();
    for(i=0; i<iWidth; i++)
    {
	int index = i-iWidth;
	if(index<-m_pProgress->buffer_len)
	{
	    qp->setPen(QColor(0,0,0));
	    qp->moveTo(i, iHeight);
	    qp->lineTo(i, 0);
	    continue;
	}
	index += m_pProgress->buffer_head;
	while(index<0)
	    index += m_pProgress->HISTORY_SIZE;
	index %= m_pProgress->HISTORY_SIZE;
	float t=m_pProgress->buffer_occupancies[index]/(double)m_pProgress->max_frames;
	if(t>1)t=1;
	qp->setPen(QColor(0, 255, 0));
	qp->moveTo(i, iHeight);
	qp->lineTo(i, (int) (iHeight*(1-t)));
	if(t<1)
	{
	    qp->setPen(QColor(0, 0, 0));
	    qp->lineTo(i, 0);
	}
    }
    delete qp;
finish:
    m_pProgress->unlock();
    int finished;
    avm::string error;
    m_pCapproc->getState(finished, error);
    if(finished)
    {
	printf("Finished\n");
	stop();
	if(error.size())
	    QMessageBox::critical(this, "Error", error.c_str());
    }
}

bool AviCapDialog::close()
{
    m_pWnd->unregisterCapDialog();
    // no need to delete child widgets, Qt does it all for us
    if (m_bStarted)
	stop();
    delete m_pTimer;
    delete m_pCapproc;
    delete m_pProgress;
    if (m_pCC)
	m_pCC->release();
    return QWidget::close();
}

void AviCapDialog::mini()
{
    i_am_mini=!i_am_mini;

    if(i_am_mini){
	this->hide();
	miniwidget->show();
    }
    else{
	this->show();
	miniwidget->hide();
    }
#if 0
    setMaximumSize(80,40);
    setBaseSize(80,40);
    setGeometry(x(),y(),80,40);
    //QSizePolicy pol(QSizePolicy::Minimum,QSizePolicy::Minimum,false);
    //setSizePolicy(pol);
    //m_pMiniButton->setSizePolicy(pol);
    m_pMiniButton->setMaximumSize(50,20);
    //miniwidget->setSizePolicy(pol);
    miniwidget->setMaximumSize(60,30);
    m_pMiniButton->adjustSize();
    miniwidget->adjustSize();
    adjustSize();
    showNormal();
    updateGeometry();
#endif
}
