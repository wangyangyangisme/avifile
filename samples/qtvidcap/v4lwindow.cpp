
#include "v4lwindow.h"
#include "v4lwindow.moc"

#include "avicapwnd.h"
#include "ccap.h"
#include "epgwindow.h"
#include "frequencies.h"
#include "picprop.h"
#include "timertable.h"
#include "vidconf.h"

#include <aviplay.h>
#include "v4lxif.h"

#include <avifile.h>
#include <avm_creators.h>
#include <avm_output.h>
#include <avm_cpuinfo.h>
#include <avm_except.h>
#include <avm_fourcc.h>
#include <videodecoder.h>
#include <utils.h>
#define DECLARE_REGISTRY_SHORTCUT
#include <configfile.h>
#undef DECLARE_REGISTRY_SHORTCUT

//#ifdef QT_VERSION
#include "fullscreen_renderer.h"

#include <renderer.h>

//#include <qglobal.h>
#include <qpopupmenu.h>
#include <qmessagebox.h>
#include <qpoint.h>
#include <qfiledialog.h>
#include <qlabel.h>
#include <qpainter.h>
#include <qcursor.h>
#include <qwindowdefs.h>
#include <qimage.h>
#include <qdom.h>
//#endif

#include "qtrenderer.h"

#ifndef WIN32
#include <pwd.h>
#endif
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>


static int rgb_method=0;

static inline void default_dimensions(int& w, int& h)
{
    if (RI("ColorMode", 0) == 1)
    {
	w = 320;
	h = 240;
    }
    else
    {
	w = 384;
	h = 288;
    }
}

V4LWindow::V4LWindow(v4lxif * pDev, QWidget * pParent)
    : QWidget(pParent, 0, 0),
    m_pDev(pDev), m_pRenderer(0), visibility(VisibilityFullyObscured),
    oc_count(0), m_eMode(Overlay), m_iPainter(0),
    m_pPicConfig(0), m_pTimerTable(0),m_pEpgWindow(0),
    m_pCapDialog(0), m_pCC(0), m_pPopup(0), m_pLabel(0),
    m_bForce4x3(true), m_bSubtitles(false),
      known_pos(false), m_bAutoRecord(false), m_bMoved(false),
      mc_switch(10),mc_tiles(3)
{
    avml(AVML_DEBUG, "V4lwindow opened\n");

    assert(m_pDev);
    m_pDev->setCapture(0);
    avml(AVML_DEBUG, "V4lwindow: after setcapture\n");
    fs_renderer=NULL;

    m_eMode=(Modes)RI("DisplayMode", 0);
    setMode(m_eMode);
    int norm=RI("ColorMode", 0);
    if(((norm==1) || (norm==3)) && m_pDev->hasvbi())//NTSC or AUTO
	m_bSubtitles=RI("Subtitles", 0);
    else
	m_bSubtitles=false;
    setupSubtitles();
    avml(AVML_DEBUG, "V4lwindow: after setupSubtitles\n");

    int w, h;
    default_dimensions(w, h);
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(timerStop()));
    connect(&m_cctimer, SIGNAL(timeout()), this, SLOT(updatesub()));
    //connect(&m_RatioTimer, SIGNAL(timeout()), this, SLOT(ratioTimerStop()));
    m_RatioTimer.start(200, FALSE);
    setBackgroundColor(QColor(0,0,0));
    avml(AVML_DEBUG, "V4lwindow: before setupDevice()\n");
    setupDevice();
    avml(AVML_DEBUG, "V4lwindow: after setupDevice()\n");
    scanXawtv();
    resize(w, h);

    last_station=-1;
    QString capstr=tr("Avicap - unknown channel");
    setCaption(capstr);
    if (fs_renderer)
      fs_renderer->setCaption(capstr.latin1());

    int plock=RI("Password-Locked",0);
    lock(plock);
    avml(AVML_DEBUG, "V4lwindow: end of constructor\n");
}

V4LWindow::~V4LWindow()
{
    avml(AVML_DEBUG, "V4lwindow: exiting V4lwindow\n");
    refresh_timer();
    if (m_eMode==AvicapRend || m_eMode==MultiChannel)
	m_eMode=Overlay;

    avicap_renderer_stop();
    multichannel_stop();

    WI("DisplayMode", m_eMode);
    WI("Subtitles", m_bSubtitles);
    delete m_pEpgWindow;
    delete m_pTimerTable;
    delete m_pDev;
    delete m_pPicConfig;
#if 0
    UserLog("exiting V4lwindow\n");
    if(user_logfh){
      fclose(user_logfh);
    }
}

void V4LWindow::UserLog(QString str){
  if(!RI("LogToFile",0)){
    return;
  }
    if(!user_logfh){
      struct passwd* pwent = getpwuid(getuid());
      QString homedir = pwent->pw_dir;
      QString filename=homedir+"/.avm/avicap-userlog.log";

      user_logfh=fopen(filename.latin1(),"a+");
    fputs("\n=============Logging started==============\n",user_logfh);
  }
#if QT_VERSION>=300
    fputs(QDateTime::currentDateTime().toString("yy/MM/dd hh:mm:ss ").latin1(),user_logfh);
#else
    fputs(QDateTime::currentDateTime().toString().latin1(),user_logfh);
#endif
  fputs(str.latin1(),user_logfh);
  fflush(user_logfh);

}

void V4LWindow::pLog(avm::string str){
#ifdef AVICAP_PARANOIA
  ParanoiaLog(str);
#endif

#endif

    avml(AVML_DEBUG, "exiting V4lwindow\n");
}

void V4LWindow::postResizeEvent(int w, int h)
{
    QEvent* e = new QResizeEvent(QSize(w, h), size());
    qApp->postEvent(this, e);
}

// Xawtv DGA refresh trick
// makes complete screen refresh by mapping unmapping fullscreen window
void V4LWindow::refresh_timer()
{
#if 0
    static long refresh_counter=0;
    if (refresh_counter%(25*60)==0)
	avml(AVML_DEBUG1, "V4lwindow: refresh timer\n");
    refresh_counter++;
#endif

    Display* dpy = x11Display();
    Window win = DefaultRootWindow(dpy);
    Screen* scr = DefaultScreenOfDisplay(dpy);
    int swidth = scr->width;
    int sheight = scr->height;
    XSetWindowAttributes xswa;
    unsigned long mask;
    Window tmp;

    xswa.override_redirect = True;
    xswa.backing_store = NotUseful;
    xswa.save_under = False;
    mask = (CWSaveUnder | CWBackingStore| CWOverrideRedirect );
    tmp = XCreateWindow(dpy, win, 0,0, swidth, sheight, 0,
			CopyFromParent, InputOutput, CopyFromParent,
			mask, &xswa);
    XMapWindow(dpy, tmp);
    XUnmapWindow(dpy, tmp);
    XDestroyWindow(dpy, tmp);
}

void V4LWindow::hidePopup()
{
    if (m_pPopup)
	m_pPopup->hide();
    //    m_pDev->setCapture(1);
    get_clips();
}

void V4LWindow::timerStop()
{
    if(m_eMode==Overlay)
    {
	//printf("SETCAPTURE\n");
	if (m_bMoved)
	{
	    m_bMoved = false;
	    refresh_timer();
	    get_clips();
	}
	m_pDev->setCapture(1);
    }
}

void V4LWindow::setMode(Modes m)
{
    avml(AVML_DEBUG, "changed display mode to %d\n", m);

    if((m==DeinterlacedPreview) && !freq.HaveMMXEXT())
	m=Preview;
    switch(m)
    {
    case Overlay:
	overlay();
	break;
    case Preview:
	preview();
	break;
    case DeinterlacedPreview:
	deinterlaced_preview();
	break;
    case AvicapRend:
	avicap_renderer();
	break;
    case MultiChannel:
        multichannel();
        break;
    }
}

void V4LWindow::moveEvent( QMoveEvent * e)
{
    if (m_eMode != Overlay || !isVisible())
	return;

    m_pDev->setCapture(0);
    int norm=RI("ColorMode", 0);
//    QPoint pnt=mapToGlobal(QPoint(0,0));
//    int field_width, field_height;
//    default_dimensions(field_width, field_height);
//    m_pDev->setCapAClip(pnt.x()+(384-field_width)/2, pnt.y()+(288-field_height)/2,
//	field_width, field_height);
//    m_pDev->setCapAClip(pnt.x(), pnt.y(), width(), height());
    setupPicDimensions();
    m_pDev->applyCapAClip(0);
    m_bMoved = true;
    if(m_timer.isActive())
	m_timer.stop();
    m_timer.start(200, TRUE);

//    if(!known_pos)
//	m_pDev->setCapture(1);
    known_pos=true;
    if(m_bAutoRecord)
    {
	captureAVI();
	m_bAutoRecord=false;
    }
}

void V4LWindow::setXawtv(int st)
{
    avml(AVML_DEBUG, "V4lwindow: setXawtv %d:%s\n",
	 st, xawtvstations[st].input.c_str());

    if (st < 0 || st >= getStations())
	return;
#if 0
    if (m_pCapDialog)
    {
	m_pCapDialog->stop();
	delete m_pCapDialog;
	m_pCapDialog = 0;
    }
#endif

    if (m_eMode == AvicapRend)
	m_CaptureTimer.stop();
    //    delete fs_renderer;
    //fs_renderer=NULL;

#if 0
    // old style channel-change
    delete m_pDev;
    delete m_pPicConfig;
    m_pPicConfig = 0;

    avm::string s = avm::string("v4lctl setstation \"");
    s += xawtvstations[st].sname.c_str();
    s += "\"";
    system(s.c_str());
#else
    //new channel change

    avm::string channelstr=xawtvstations[st].channel;

    int cl=0;
    while(!( chanlists[cl].name==NULL || chanlists[cl].name==freqtable )){
	cl++;
    }
    if(chanlists[cl].name==NULL){
	printf("not found\n");
    }
    else{
      //printf("found channellist %d\n",cl);
     
      const struct CHANLIST *myclist=chanlists[cl].list;
      int myccount=chanlists[cl].count;

      int scount=0;
      while(!(scount>=myccount || myclist[scount].name==channelstr )){
	scount++;
      }

      if(scount>=myccount){
	printf("channel %s not found\n",channelstr.c_str());
      }
      else{
	m_pDev->setAudioMute(true);
	//m_pDev->setAudioVolume(0);

	int freq=myclist[scount].freq;
	//printf("channel %s freq %d\n",channelstr.c_str(),freq);

	//m_pDev->setCapture(0);
	//sleep(1);
	unsigned long lfreq=(unsigned long) freq*16/1000;
	lfreq+=xawtvstations[st].finetune;
	m_pDev->setFreq(lfreq);
	//sleep(1);
	//m_pDev->setCapture(1);
      }
    }
#endif

    try
    {
      //m_pDev = new v4l1if(0, RS("CapDev", "/dev/video"));
	m_pDev->setChannelName(xawtvstations[st].input.c_str());
	get_clips();
	if(m_eMode!=MultiChannel){
	  //m_pDev->setAudioVolume(st*5);
	   m_pDev->setAudioMute(false);
	}
	last_station=st;
	QString locked;
	if (RI("Password-Locked",0))
	    locked=tr(" locked");

	QString capstr = tr( "Avicap: " ) + QString::number(st+1)+" - "+QString(xawtvstations[st].sname.c_str()) + locked;
	setCaption( capstr );
	if (fs_renderer)
	    fs_renderer->setCaption( capstr.latin1() );
    	avml(AVML_DEBUG1, "V4lwindow: channel change -> %d\n", st);

    }
    catch (FatalError& e)
    {
	e.PrintAll();
	m_pDev = 0;
    }
    if (m_eMode == AvicapRend){
	// avicap_renderer();
	m_CaptureTimer.start(40);
    }
}

static avm::string get_keyword(const char* s)
{
    const char* e = strchr(s, '=');
    if (e)
    {
	e++;
	while (*e && isspace(*e)) e++;
	s = e;
	while (*e && !isspace(*e)) e++;
    }
    else
        e = s;
    return avm::string(s, e - s);
}

/*
 *
 *
 *  Simple parser of Xawtv configure file
 *
 */
void V4LWindow::scanXawtv()
{
    avml(AVML_DEBUG1, "reading Xawtv file\n");

    struct passwd* pwent = getpwuid(getuid());
    avm::string xawtvcnf = pwent->pw_dir;
    xawtvcnf += "/.xawtv";
    FILE* fl = fopen(xawtvcnf.c_str(), "rb");
    int x = 0;
    xawtvstations.clear();

    if (fl != NULL)
    {
	xawtvstation xawdefaults;
	xawtvstation xs;

	while (!feof(fl))
	{
	    char b[500];
	    fgets(b, sizeof(b) - 1, fl);
	    b[sizeof(b) - 1] = 0;
	    char* s = strchr(b, '#');
	    if (s)
		*s = 0;
	    if ((s = strchr(b, '[')))
	    {
		s++;
		char* e = strchr(s, ']');
		avm::string sname = avm::string(s, e - s);
		if (strcasecmp(sname.c_str(), "global") == 0
		    || strcasecmp(sname.c_str(), "launch") == 0)
		    continue;
		//printf("stname %s\n", sname.c_str());

		xs.sname = sname;

		if (strcasecmp(sname.c_str(), "defaults") == 0)
		{
		    xawdefaults = xs;
                    continue;
		}

		if (!getStations())
		    // assumption - default comes in the begining
		    xawdefaults = xs;
		xawtvstations.push_back(xs);
		xs = xawdefaults;
	    }
	    else if ((s = strstr(b, "key")))
	    {
		avm::string k = get_keyword(s);
		if (k.size())
		{
		    xs.key = toupper(k[0]);
		    if (getStations())
			xawtvstations.back().key = xs.key;
		}
	    }
	    else if ((s = strstr(b, "input")))
	    {
		xs.input = get_keyword(s);
		if (xs.input.size())
		{
		    if (getStations())
			xawtvstations.back().input = xs.input;
		}
	    }
	    else if ((s = strstr(b, "freqtab")))
	    {
                freqtable = get_keyword(s);
		printf("freqtab is %s\n", freqtable.c_str());
	    }
	    else if ((s = strstr(b, "channel")))
	    {
		avm::string channelstr = get_keyword(s);
		if (channelstr.size()) {
		    int ss = getStations();
		    xawtvstations[ss-1].channel = channelstr;
		    //printf("channel %s for %s\n", channelstr.c_str(), xawtvstations[ss-1].sname.c_str());
		}
	    }
	    else if ((s = strstr(b, "fine")))
	    {
		avm::string finestr = get_keyword(s);
		if (finestr.size()) {
		    int ss = getStations();
		    xawtvstations[ss-1].finetune = atoi(finestr.c_str());;
		    printf("finetune %d for %s\n",
			   xawtvstations[ss-1].finetune, xawtvstations[ss-1].sname.c_str());
		}
	    }
	}
    }
}

bool V4LWindow::event(QEvent* e)
{
  //    printf("EVENT %d\n", e->type());
    switch (e->type())
    {
    case Q_MyCloseEvent:
	emit overlay();
	break;
    default:
	return QWidget::event(e);  // parent call
    }
    return true;
}


void V4LWindow::sendQt(QEvent* e)
{
	qApp->postEvent(this, e);
	// event deleted by Qt
#if QT_VERSION > 220
	// qApp->wakeUpGuiThread();
#endif
}

void V4LWindow::makePopup()
{
  //  avm::Locker lock(m_Mutex);

    if (m_pPopup && m_pPopup->isVisible())
    {
	m_pPopup->hide();
	return;
    }
    delete m_pPopup;
    m_pPopup = new QPopupMenu;
    int norm = RI("ColorMode", 0);
    m_pPopup->insertItem(tr("Configure"), this, SLOT(config()));

    if (getStations())
    {
	QPopupMenu* p = new QPopupMenu();
	connect(p, SIGNAL(activated(int)), this, SLOT(changeChannel(int)));

	char cbuf[100];
	for (unsigned i = 0; i < xawtvstations.size(); i++){
	    sprintf(cbuf,"%-2d %s",i+1,xawtvstations[i].sname.c_str());
	    p->insertItem(cbuf, i);
	}
	m_pPopup->insertItem(tr("Xawtv stations"), p);
    }

    m_pPopup->insertItem(tr("Picture properties"), this, SLOT(picprop()));
    m_pPopup->insertItem(tr("Timertable"), this, SLOT(timertable()));
    m_pPopup->insertItem(tr("Electronic Program Guide"), this, SLOT(epgwindow()));
    m_pPopup->insertItem(tr("Capture AVI"), this, SLOT(captureAVI()));
    m_pPopup->insertItem(tr("Capture BMP"), this, SLOT(captureBMP()));
    //    m_pPopup->insertItem("Capture JPG", this, SLOT(captureJPG()));
    m_pPopup->insertSeparator();
    int iItem=m_pPopup->insertItem(tr("Force 4x3 ratio"), this, SLOT(force4x3()));
    m_pPopup->setCheckable(true);
    m_pPopup->setItemChecked(iItem, m_bForce4x3);
    if(((norm==1) || (norm==3)) && m_pDev->hasvbi())//NTSC or AUTO
    {
	iItem=m_pPopup->insertItem(tr("Show subtitles"), this, SLOT(showsub()));
	m_pPopup->setCheckable(true);
	m_pPopup->setItemChecked(iItem, m_bSubtitles);
    }
    else
	m_bSubtitles=false;
    //    m_pPopup->setChecked(m_bForce4x3);
    m_pPopup->insertSeparator();
    int iModeItems[5];
    iModeItems[0]=m_pPopup->insertItem(tr("&Overlay"), this, SLOT(overlay()));
    m_pPopup->setCheckable(true);
    m_pPopup->setItemChecked(iModeItems[0], false);
    iModeItems[1]=m_pPopup->insertItem(tr("&Preview"), this, SLOT(preview()));
    m_pPopup->setCheckable(true);
    m_pPopup->setItemChecked(iModeItems[1], false);
    if(freq.HaveMMXEXT())
    {
	iModeItems[2]=m_pPopup->insertItem(tr("Deinterlace"), this, SLOT(deinterlaced_preview()));
	m_pPopup->setCheckable(true);
	m_pPopup->setItemChecked(iModeItems[2], false);
    }
    else
	iModeItems[2]=-1;

    iModeItems[3]=m_pPopup->insertItem(tr("exp. SDL/FS renderer"), this, SLOT(avicap_renderer()));
    m_pPopup->setCheckable(true);
    m_pPopup->setItemChecked(iModeItems[3], false);

    iModeItems[4]=m_pPopup->insertItem(tr("MultiChannel"), this, SLOT(multichannel()));
    m_pPopup->setCheckable(true);
    m_pPopup->setItemChecked(iModeItems[3], false);


    switch(getMode())
    {
    case Overlay:
	m_pPopup->setItemChecked(iModeItems[0], true);
	break;
    case Preview:
	m_pPopup->setItemChecked(iModeItems[1], true);
	break;
    case DeinterlacedPreview:
	//	if(iModeItems[2]>=0)
	m_pPopup->setItemChecked(iModeItems[2], true);
	break;
    case AvicapRend:
	m_pPopup->setItemChecked(iModeItems[3], true);
	break;
    case MultiChannel:
        m_pPopup->setItemChecked(iModeItems[4], true);
	break;
    }
#if 0
    m_pPopup->insertSeparator();
    m_pPopup->insertItem(tr("Fullscreen"), this, SLOT(fullscreen()));
    //#ifdef HAVE_LIBXXF86VM
    m_pPopup->insertItem("Maximize", this, SLOT(maximize()));
    //#endif
    //#endif
#endif

    m_pPopup->insertSeparator();
    m_pPopup->insertItem(tr("&Quit"), this, SLOT(close()));

#if 0
    Window root_return, child_return;
    int root_x_return, root_y_return;
    int win_x_return, win_y_return;
    unsigned int mask_return;

    XQueryPointer(x11Display(), handle(), &root_return, &child_return,
		  &root_x_return, &root_y_return,
		  &win_x_return, &win_y_return, &mask_return);
#endif
    QPoint cpos=QCursor::pos();
    //popup(QPoint(root_x_return, root_y_return));
    m_pPopup->exec(cpos);
}

void V4LWindow::avicap_renderer_stop()
{
    m_avicapTimer.stop();
    delete fs_renderer;
    fs_renderer=NULL;

}
void V4LWindow::avicap_renderer_close()
{
    QEvent *me=new QEvent(Q_MyCloseEvent);

    sendQt(me);

}

void V4LWindow::avicap_renderer_popup()
{

    QMouseEvent *me=new QMouseEvent(QEvent::MouseButtonRelease,QCursor::pos(),Qt::RightButton,Qt::RightButton);

    sendQt(me);
    //    makePopup();


}

void V4LWindow::mouseReleaseEvent( QMouseEvent * e)
{
    if (!(e->button() & Qt::RightButton))
	return;

    makePopup();
}

void V4LWindow::config()
{
    avml(AVML_DEBUG1, "V4lwindow:: configure\n");
    //m_pPopup=0;
    m_pDev->setCapture(0);
    if (m_timer.isActive())
	m_timer.stop();
    m_timer.start(100, TRUE);

    VidConfig conf(this, m_pDev, this);
    //QPoint pnt = mapToGlobal(QPoint(0,0));
    //conf.move(pnt.x() + width() / 2, pnt.y() + height() / 4);
    conf.move(QCursor::pos());
    conf.exec();
    setupPicDimensions();
    setupDevice();
}

void V4LWindow::lock(bool lock){
    //locked or not?
    if(lock){
	avml(AVML_DEBUG1, "V4lwindow: locking system\n");
	//we have to lock system

	//make shure the epg info is available
	epgwindow();
	m_pEpgWindow->lock(true);
	changeChannel(last_station);
    }
    else{
	//unlock
	avml(AVML_DEBUG1, "V4lwindow: unlock system\n");
	if (m_pEpgWindow)
	    m_pEpgWindow->lock(false);
	//changeChannel(last_station);
    }
}

void V4LWindow::force4x3()
{
    hidePopup();
    m_bForce4x3 = !m_bForce4x3;
    if (m_bForce4x3)
	postResizeEvent(width(), height());
}

void V4LWindow::getOverlaySizeAndOffset(int *pw, int *ph, int* pdx, int* pdy)
{
    int dxpos, dypos;
    int max_w, max_h;
    *pw=width();
    *ph=height();
    if(m_bSubtitles)
	*ph-=80;
    if(*ph<=0)
    {
	printf("Oops: ph<0\n");
	return;
    }
    default_dimensions(max_w, max_h);
    max_w*=2;
    max_h*=2;
    dxpos=dypos=0;
    if(*pw>max_w)
    {
	dxpos=(*pw-max_w)/2;
	*pw=max_w;
    }
    if(*ph>max_h)
    {
	dypos=(*ph-max_h)/2;
	*ph=max_h;
    }
    if(pdx)*pdx=dxpos;
    if(pdy)*pdy=dypos;
}

static int get_palette(Display * dpy)
{
    int r;
    switch (avm::GetPhysicalDepth(dpy))
    {
    case 15:
	r = VIDEO_PALETTE_RGB555;
	break;
    case 16:
	r = VIDEO_PALETTE_RGB565;
	break;
    case 24:
	r = VIDEO_PALETTE_RGB24;
	break;
    case 32:
    default:
	r = VIDEO_PALETTE_RGB32;
	break;
    }
    return r;
}

void V4LWindow::setupPicDimensions(int* pdx, int* pdy)
{
    avml(AVML_DEBUG1, "V4lwindow: setting pic dimensions\n");

    avm::Locker lock(m_Mutex);
    int dxpos, dypos, pw, ph;
    QPoint pnt = mapToGlobal(QPoint(0,0));
    getOverlaySizeAndOffset(&pw, &ph, &dxpos, &dypos);
    if(m_eMode == Overlay)
	m_pDev->setCapAClip(pnt.x()+dxpos, pnt.y()+dypos, pw, ph);
    else if(m_eMode==AvicapRend){

    }
    else
    {
	if (m_pRenderer
	    && (pw != m_pRenderer->getWidth()
		|| (ph!=m_pRenderer->getHeight())))
	{
		delete m_pRenderer;
		m_pRenderer=0;
	}

	if (!m_pRenderer)
	{
	    m_pRenderer = new ShmRenderer(this, pw, ph, dxpos, dypos);

	} else
	    m_pRenderer->move(dxpos, dypos);
	m_pDev->grabSetParams(pw, ph, get_palette(x11Display()));
    }
    if(pdx)*pdx=dxpos;
    if(pdy)*pdy=dypos;
}


void V4LWindow::resizeEvent( QResizeEvent * e)
{
    QWidget::resizeEvent(e);
    int w=e->size().width();
    int h=e->size().height();

    if(m_eMode==MultiChannel){
      mc_curw=w;
      mc_curh=h;
      multichannel();
      return;
    }
//	printf("received resize event to %d,%d\n", w, h);
//    printf("resizeEvent: %d,%d -> %d,%d ( %d,%d )\n",
//	e->oldSize().width(), e->oldSize().height(),
//	e->size().width(), e->size().height(),
//	width(), height());
//    if(!isVisible())
//    {
//		printf("!visible\n");
//		return;
//    }
//    if(!known_pos)
//    {
//		printf("!known\n");
//		return;
//    }
//    if(m_bForce4x3 && w!=4*(h-(m_bSubtitles?80:0))/3)
//    {
//		printf("Recalc %d,%d\n", 4*(h-(m_bSubtitles?80:0))/3, h);
//		postResizeEvent(4*(h-(m_bSubtitles?80:0))/3, h);
//		resize(4*(h-(m_bSubtitles?80:0))/3, h);
//		return;
//    }
    if (m_pLabel)
    {
	m_pLabel->move(0, h-80);
	m_pLabel->resize(w, 80);
    }
    m_pDev->setCapture(0);
    setupPicDimensions();
    m_pDev->applyCapAClip(0);
    //    m_pDev->setCapture(1);
    if (m_timer.isActive())
	m_timer.stop();
    if(m_eMode==AvicapRend){
	avicap_renderer_resize_input(w,h);
    }

    m_timer.start(100, TRUE);
}

void V4LWindow::showEvent( QShowEvent * )
{
    if(m_eMode!=Overlay)
	return;
    if(!known_pos)
	return;
    //    avm_usleep(100000);
    //    QPoint pnt=mapToGlobal(QPoint(0,0));
    //    m_pDev->setCapAClip(pnt.x(), pnt.y(), width(), height());
    setupPicDimensions();
    m_pDev->applyCapAClip(0);
    m_pDev->setCapture(1);
    get_clips();
    //printf("showEvent()\n");
}

void V4LWindow::hideEvent( QHideEvent * qhe )
{
    //printf("hideEvent()\n");
    m_pDev->setCapture(0);
}

void V4LWindow::focusInEvent( QFocusEvent * )
{
    //if(!known_pos)return;
    //printf("focusInEvent()\n");
    get_clips();
}

void V4LWindow::focusOutEvent( QFocusEvent * )
{
    //printf("focusOutEvent()\n");
    get_clips();
}

void V4LWindow::paintEvent( QPaintEvent * qpe )
{
    //printf("paintEvent()  %d   %p    %d\n", m_bMoved, m_pRenderer, m_iPainter);
    if (--m_iPainter < 0)
    {
	m_bMoved = true;
	m_pDev->setCapture(0);
	if (m_timer.isActive())
	    m_timer.stop();
	m_timer.start(100, TRUE);
	m_iPainter = 1;
    }
}

void V4LWindow::captureAVI()
{
    avml(AVML_DEBUG1, "V4lwindow: captureAVI (manual)\n");
    captureAVI(NULL);
}

void V4LWindow::captureAVI(CaptureConfig *capconf)
{
    avml(AVML_DEBUG1, "V4lwindow: captureAVI (timertable)\n");
    if (m_pCapDialog)
	return;
    m_pCapDialog=new AviCapDialog(this,capconf);
    //    QPoint pnt=mapToGlobal(QPoint(0,0));
    //    m_pCapDialog->move(pnt.x(), pnt.y()+height()+40);
    m_pDev->setCapture(0);
    if(m_timer.isActive())
	m_timer.stop();
    m_timer.start(100, TRUE);
    m_pCapDialog->show();
    if(m_bAutoRecord)
	m_pCapDialog->start();
    //    m_pDev->setCapture(0);
    //    QMessageBox::information(this, "Sorry", "Not implemented yet!");
    //    m_pDev->setCapture(1);
}

void V4LWindow::captureBMP()
{
    //    int w=width();
    //    int h=height();
    Modes eMode=m_eMode;
    overlay();
    unsigned int w, h;
    int x, y;
    m_pDev->getCapAClip(&x, &y, &w, &h);
    w &= ~3;
    m_pDev->grabSetParams(w, h, VIDEO_PALETTE_RGB24);
    unsigned char* cp=(unsigned char*)m_pDev->grabCapture( true );
    avm::CImage* im=new avm::CImage(cp, w, -h);

    m_pDev->setCapture(0);
    if(m_timer.isActive())
	m_timer.stop();
    m_timer.start(100, TRUE);
    QString qs=QFileDialog::getSaveFileName(QString::null, "*.bmp", this, "Save as BMP file");
    if(!qs.isNull())
	im->Dump(qs);
    delete im;
    setMode(eMode);
}

void V4LWindow::captureJPG()
{
    //    m_pDev->setCapture(0);
    QMessageBox::information(0, tr( "Sorry" ), tr(  "Not implemented yet!" ));
    //    m_pDev->setCapture(1);
}

void V4LWindow::changeChannel(int ch)
{
    if (ch == -1)
	ch = 0;

    if (m_pEpgWindow==NULL || !m_pEpgWindow->isBlocked(ch+1))
	setXawtv(ch);
    else
	avml(AVML_DEBUG1, "V4lwindow: channel %d blocked\n", ch);
}

void V4LWindow::avicap_renderer_keypress(int sym,int mod)
{
    //printf("sym=%d mod=%d\n",sym,mod);
    int key=0;
    if(sym==273){
	key=Qt::Key_Up;
    }
    else if(sym==274){
	key=Qt::Key_Down;
    }

    //printf("a1\n");
    keyPress(sym,key);
    //printf("a2\n");
}

void V4LWindow::keyPress(int k,int key)
{
  //avm::Locker lock(m_Mutex);
  
    switch(k)
    {
    case 'B':
	captureBMP();
	break;
    case 'C':
	config();
	break;
    case 'Q':
	close();
	break;
    case ']':
      multichannel_increase_tiles(true);
      break;
    case '[':
      multichannel_increase_tiles(false);
      break;
    case '+':
      multichannel_increase_speed(true);
      break;
    case '-':
      multichannel_increase_speed(false);
      break;
    }

    bool xawtv_key=false;

    for (unsigned i = 0; i < xawtvstations.size(); i++)
    {
	//printf("key  %d   %d\n", xawtvstations[i].key, k);
	if (xawtvstations[i].key == k){
	    if(m_pEpgWindow==NULL || !m_pEpgWindow->isBlocked(i+1)){
		setXawtv(i);
	    }
	    xawtv_key=true;
	}
    }

    //alternate keybindings
    if(!xawtv_key){
	if(k>='1' && k<='9'){
	    int chnr=k-48;
	    if(m_pEpgWindow==NULL || !m_pEpgWindow->isBlocked(chnr)){
		setXawtv(chnr-1);
	    }
	}
    }

    if(key==Qt::Key_Down){
	int station=last_station;
	do{
	    station--;
	    if(station<0){
		station=xawtvstations.size()-1;
	    }
	}while(station!=last_station && (m_pEpgWindow && m_pEpgWindow->isBlocked(station+1)));

	setXawtv(station);
    }
    else if(key==Qt::Key_Up){
	int station=last_station;
	do{
	    station++;
	    if(station>(int)xawtvstations.size()-1){
		station=0;
	    }
	}while(station!=last_station && (m_pEpgWindow && m_pEpgWindow->isBlocked(station+1)));

	setXawtv(station);
    }


}

void V4LWindow::keyPressEvent( QKeyEvent * e )
{
    int k = toupper(e->ascii());
    int key=e->key();

    keyPress(k,key);

}

void V4LWindow::setupDevice()
{
    //    int field_width, field_height;
    //    default_dimensions(field_width, field_height);
    //    v4l->setCapAClip(102,110,field_width,field_height);
    //    v4l->addCapAClip(0,248,384,288);
    //    v4l->applyCapAClip(0);
    avml(AVML_DEBUG1, "V4lwindow: setupDevice()\n");

    // I'm not sure why - but apply has to call this
    // method twice to get apllied Norm & Channel change
    // at the same time - if anyone knows how to fix this
    // let us know - avifile@prak.org  FIXME
    int i = 2;
    while (i--)
    {
	m_pDev->setChannelNorm(RI("ColorMode", 0));
	m_pDev->setChannel(RI("Channel", 1));
    }
    avml(AVML_DEBUG1, "V4lwindow: setupDevice() - channel is set\n");

    m_pDev->setAudioMute(0);
    m_pDev->setAudioVolume(65535);//?
    m_pDev->setAudioBalance(100);

    int amode;
    switch (RI("SndChannels", 0))
    {
    default:
    case 0:
	amode = VIDEO_SOUND_MONO;
	break;
    case 1:
	amode = VIDEO_SOUND_STEREO;
	break;
    case 2:
	amode = VIDEO_SOUND_LANG1;
	break;
    case 3:
	amode = VIDEO_SOUND_LANG2;
	break;
    }
    m_pDev->setAudioMode(amode);

    m_pDev->setPicBrightness(RI("Image\\Brightness", 0));
    m_pDev->setPicConstrast(RI("Image\\Contrast", 255));
    m_pDev->setPicColor(RI("Image\\Color", 255));
    m_pDev->setPicHue(RI("Image\\Hue", 0));

    int r = RI("Resolution", 0);

    avml(AVML_DEBUG1, "V4lwindow: setupDevice() starting capture\n");
    if (m_eMode == Overlay)
	m_pDev->setCapture(1);

    get_clips();
    avml(AVML_DEBUG1, "V4lwindow: setupDevice() done\n");
}


void V4LWindow::showsub()
{
    hidePopup();
    m_bSubtitles=!m_bSubtitles;
    avml(AVML_DEBUG1, "V4LWindow::showsub(): size %dx%d, set m_bSubtitles to %d\n",
	 width(), height(), (int)m_bSubtitles);
    setupSubtitles();
}

void V4LWindow::setupSubtitles()
{
    if(m_bSubtitles)
    {
	m_pCC=new ClosedCaption(m_pDev);
	m_pLabel=new QLabel(this);
	int w=width();
	int h=height();
	//	if(!m_bForce4x3)
	resize(w, h+80);
	m_pLabel->move(0, h);
	m_pLabel->resize(w, 80);
	m_pLabel->setAlignment(Qt::AlignCenter);
	m_pLabel->show();
	m_cctimer.start(500);
    }
    else
    {
	m_cctimer.stop();
	delete m_pCC;
	delete m_pLabel;
	m_pCC=0;
	m_pLabel=0;
	resize(width(), height() - 80);
    }
}

void V4LWindow::startTimer()
{
    timertable();
    m_pTimerTable->startTimer(true);
}

void V4LWindow::timertable()
{
    if (!m_pTimerTable)
    {
	avml(AVML_DEBUG1, "V4lwindow: timertable started\n");
	m_pTimerTable=new TimerTable(this,m_pDev,this);
	QPoint pnt=mapToGlobal(QPoint(0,0));
	m_pTimerTable->move(pnt.x(), pnt.y()+height()+20);
    }

    if (m_pTimerTable->isVisible())
	m_pTimerTable->hide();
    else
	m_pTimerTable->show();
}

void V4LWindow::epgwindow()
{
    if (!m_pEpgWindow)
    {
	avml(AVML_DEBUG1, "V4lwindow: epgwindow started\n");
	timertable();
	m_pEpgWindow=new EpgWindow(this,m_pTimerTable);
	QPoint pnt=mapToGlobal(QPoint(0,0));
	m_pEpgWindow->move(pnt.x(), pnt.y()+height()+20);
	m_pTimerTable->setEpgWindow(m_pEpgWindow);
    }

    if (m_pEpgWindow->isVisible())
	m_pEpgWindow->hide();
    else
	m_pEpgWindow->show();
}


void V4LWindow::picprop()
{
    avml(AVML_DEBUG1, "V4lwindow: picprop()\n");

    if (!m_pPicConfig)
    {
	m_pPicConfig = new PicturePropDialog(m_pDev);
	QPoint pnt = mapToGlobal(QPoint(0,0));
	m_pPicConfig->move(pnt.x(), pnt.y()+height()+20);
    }

    if (m_pPicConfig->isVisible())
	m_pPicConfig->hide();
    else
	m_pPicConfig->show();
}

static int x11_error = 0;
typedef int (*handlerproc) (Display*, XErrorEvent*);
static int x11_error_dev_null(Display * dpy, XErrorEvent * event)
{
    x11_error++;
    //    if (debug > 1)
    //	fprintf(stderr," x11-error\n");
    return 0;
}

void V4LWindow::add_clip(int x1, int y1, int x2, int y2)
{
    if (oc_count >= (int) (sizeof(oc)/sizeof(oc[0])))
	return;
    if (oc[oc_count].x1 != x1 || oc[oc_count].y1 != y1 ||
	oc[oc_count].x2 != x2 || oc[oc_count].y2 != y2) {
	conf = 1;
    }
    oc[oc_count].x1 = x1;
    oc[oc_count].y1 = y1;
    oc[oc_count].x2 = x2;
    oc[oc_count].y2 = y2;
    oc_count++;
}

bool V4LWindow::x11Event( XEvent * e)
{
#if 0
    switch(e->type)
    {
    case VisibilityNotify:
	printf("VisibilityNotify\n");
	if (e->xvisibility.window == handle())
	{
	    visibility = e->xvisibility.state;
	    //	    get_clips();
	}
	break;
    case MapNotify:
	printf("MapNotify\n");
	wmap=1;
	break;
    case UnmapNotify:
	printf("UnmapNotify\n");
	wmap=0;
	break;
    case KeyPress:
    case KeyRelease:
    case ButtonPress:
    case ButtonRelease:
    case MotionNotify:
    case EnterNotify:
    case LeaveNotify:
    case FocusIn:
    case FocusOut:
    case KeymapNotify:
    case Expose:
    case GraphicsExpose:
    case NoExpose:
    case CreateNotify:
    case DestroyNotify:
    case MapRequest:
    case ReparentNotify:
    case ConfigureNotify:
    case ConfigureRequest:
    case GravityNotify:
    case ResizeRequest:
    case CirculateNotify:
    case CirculateRequest:
    case PropertyNotify:
    case SelectionClear:
    case SelectionRequest:
    case SelectionNotify:
    case ColormapNotify:
    case ClientMessage:
    case MappingNotify:
	//    printf("Event %d\n", e->type);
	break;
    }
    get_clips();

#endif
    return QWidget::x11Event(e);
}

void V4LWindow::get_clips()
{
    if (m_eMode != Overlay || !isVisible())
	return;
    //printf("GETCLIPS\n");

    int x1,y1,x2,y2,lastcount;
    XWindowAttributes wts;
    Window root, me, rroot, parent, *children;
    uint nchildren, i;
    int wx, wy;
    conf=0;
    QPoint pnt=mapToGlobal(QPoint(0,0));
    wx=pnt.x();
    wy=pnt.y();
    Display * dpy=x11Display();
    root=DefaultRootWindow(dpy);
    me = (Window)handle();
    m_pDev->setCapture(0);
    XSelectInput(dpy, me, ~0);
    XSelectInput(dpy, root, VisibilityChangeMask | ExposureMask);
    /*
     XGetWindowAttributes(dpy, me, &wts);
     wts.your_event_mask=~0;
     */
    //    XSetWindowAttributes xsw;
    //    xsw.event_mask=~0;
    //    XChangeWindowAttributes(dpy, me, CWEventMask, &xsw);
    //    GC gc=qt_xget_readonly_gc(false);
    //    XSetGraphicsExposures( dpy, gc, TRUE );
    //    XGCValues xcg;
    //    xcg.graphics_exposures=true;
    //    gc=XDefaultGC(dpy, me, GCGraphicsExposures, &xcg);

    /*
     if (wmap
     && visibility != VisibilityFullyObscured
     && visibility != VisibilityPartiallyObscured)
     {
     oc_count = 0;
     return;
     }
     */
    handlerproc old_handler = XSetErrorHandler(x11_error_dev_null);

    //    if (debug > 1)
    //	fprintf(stderr," getclips");
    lastcount = oc_count;
    oc_count = 0;

    if (wx<0)
	add_clip(0, 0, (uint)(-wx), height());
    if (wy<0)
	add_clip(0, 0, width(), (uint)(-wy));
    //    if ((wx+width()) > swidth)
    //	add_clip(swidth-wx, 0, width(), height());
    //    if ((wy+height()) > sheight)
    //	add_clip(0, sheight-wy, width(), height());

    // find parent
    for (;;) {
	XQueryTree(dpy, me, &rroot, &parent, &children, &nchildren);
	if (children)
	    XFree((char *) children);
	/* fprintf(stderr,"me=0x%x, parent=0x%x\n",me,parent); */
	if (root == parent || me == parent)
	    break;
	me = parent;
    }

    // now find all clippping parts
    XQueryTree(dpy, root, &rroot, &parent, &children, &nchildren);
    //    printf("%d children to check\n", nchildren);
    for (i = 0; i < nchildren && children[i] != me; i++)
	;/* empty */
    for (i++; i<nchildren; i++) {
	XGetWindowAttributes(dpy, children[i], &wts);
	if (!(wts.map_state & IsViewable))
	{
	    //	    printf("child %d, (%d,%d)x(%d,%d): skipping\n",
	    //		i, wts.x, wts.y, wts.width, wts.height);
	    continue;
	}
	x1=wts.x-wx;
	y1=wts.y-wy;
	x2=x1+wts.width+2*wts.border_width;
	y2=y1+wts.height+2*wts.border_width;
	if ((x2 < 0) || (x1 > (int)width()) ||
	    (y2 < 0) || (y1 > (int)height()))
	{
	    //	    printf("child %d, (%d,%d)x(%d,%d): not overlapping\n",
	    //		i, wts.x, wts.y, wts.width, wts.height);
	    continue;
	}
	if (x1<0)      	         x1=0;
	if (y1<0)                y1=0;
	if (x2>(int)width())  x2=width();
	if (y2>(int)height()) y2=height();
	add_clip(x1, y1, x2, y2);
    }
    XFree((char *) children);

    if (lastcount != oc_count)
	conf=1;

    if(conf)
    {
	m_pDev->resetCapAClip();
	int dx, dy;
	setupPicDimensions(&dx, &dy);
	for(int i=0; i<oc_count; i++)
	{
	    //	    printf("Adding clip (%d,%d)x(%d,%d)\n",
	    //		oc[i].x1, oc[i].y1, oc[i].x2, oc[i].y2);
	    m_pDev->addCapAClip(-dx+oc[i].x1, -dy+oc[i].y1, -dx+oc[i].x2, -dy+oc[i].y2);
	}
	//	printf("Applying %d clips\n", oc_count);
	m_pDev->applyCapAClip(oc_count);
	m_pDev->setCapture(1);

	//	if(m_timer.isActive())
	//	    m_timer.stop();
	//	m_timer.start(100, TRUE);
    }
    else
	m_pDev->setCapture(1);

    XSetErrorHandler(old_handler);
}

void V4LWindow::updatesub()
{
    //    printf("updatesub()\n");
    if(!m_pCC)
	return;
    m_pCC->lock();
    char buf[256];
    strncpy(buf, m_pCC->getBuffer(), sizeof(buf) - 1);
    m_pCC->unlock();
    //    printf("%s\n", buf);
    assert(m_pLabel);
    assert(m_bSubtitles);
    m_pLabel->setText(buf);
}

void V4LWindow::overlay()
{
    show();
    avm::Locker lock(m_Mutex);
    printf("overlay()\n");
    m_eMode=Overlay;
    m_pDev->setCapture(1);

    //stop shm renderer
    m_CaptureTimer.stop();
    delete m_pRenderer;
    m_pRenderer=0;

    avicap_renderer_stop();
    multichannel_stop();
}

void V4LWindow::preview()
{
    int pw, ph, dx, dy;
    show();
    avm::Locker lock(m_Mutex);
    printf("preview()\n");
    getOverlaySizeAndOffset(&pw, &ph, &dx, &dy);
    m_eMode=Preview;

    avicap_renderer_stop();
    multichannel_stop();

    m_pDev->setCapture(0);
    m_pDev->grabSetParams(pw, ph, get_palette(x11Display()));
    connect(&m_CaptureTimer, SIGNAL(timeout()), this, SLOT(preview_display()));
    m_CaptureTimer.start(30);
    m_eMode=Preview;
    m_pDev->setCapture(0);
    delete m_pRenderer;

    printf("new shm renderer %d %d\n",pw,ph);

    m_pRenderer=new ShmRenderer(this, pw, ph, dx, dy);

    connect(&m_CaptureTimer, SIGNAL(timeout()), this, SLOT(preview_display()));
    m_CaptureTimer.start(40);
}

void V4LWindow::deinterlaced_preview()
{
    if(!freq.HaveMMXEXT())
	return preview();
    printf("deint_preview()\n");
    show();
    avm::Locker lock(m_Mutex);
    int pw, ph, dx, dy;
    getOverlaySizeAndOffset(&pw, &ph, &dx, &dy);

    avicap_renderer_stop();
    multichannel_stop();

    m_eMode=DeinterlacedPreview;
    m_pDev->setCapture(0);
    m_pDev->grabSetParams(pw, ph, get_palette(x11Display()));
    delete m_pRenderer;
    m_pRenderer = new ShmRenderer(this, pw, ph, dx, dy);
    connect(&m_CaptureTimer, SIGNAL(timeout()), this, SLOT(preview_display()));
    m_CaptureTimer.start(30);
}

void V4LWindow::avicap_renderer()
{
    int pw, ph, dx, dy;
    avm::Locker lock(m_Mutex);
    printf("avicap-renderer()\n");

    m_eMode=AvicapRend;

    //stop shm renderer
    m_CaptureTimer.stop();
    delete m_pRenderer;
    m_pRenderer=0;

    multichannel_stop();

    getOverlaySizeAndOffset(&pw, &ph, &dx, &dy);

    fsr_width=pw;
    fsr_height=ph;

    printf("New avicap- renderer %d %d\n",pw,ph);

    delete fs_renderer;
    fs_renderer = 0;

    void *dpy = XOpenDisplay(0);
    fs_renderer = new avm::AvicapRenderer(dpy);
    fs_renderer->setV4LWindow(this);
    fs_renderer->setSize(pw,ph);
    fs_renderer->setYUVrendering(true);

    m_pDev->setCapture(0);
    //m_pDev->grabSetParams(pw, ph, get_palette(x11Display()));

    Colorspaces colorspace=(enum Colorspaces)RI("Colorspace", 0);

    colorspace=cspYUY2;
    Colorspaces recording_colorspace=colorspace;

    if (colorspace==cspRGB24 && rgb_method==0){
	printf("rgb24 renderer - rgb24 to rgb24\n");
	m_pDev->grabSetParams(pw, ph, VIDEO_PALETTE_RGB24);
	fs_renderer->setYUVrendering(false);
    }
    else if(colorspace==cspRGB24 && rgb_method==1){
	printf("rgb24 renderer - grab rgb24 to yuv overlay,automatic conv.\n");
	m_pDev->grabSetParams(pw, ph, VIDEO_PALETTE_RGB24);
	//      recording_colorspace=cspYUY2;
    }
    else if(colorspace==cspRGB24 && rgb_method==2){
	printf("rgb24 renderer - grab rgb24 to yuv overlay, I do conv.\n");
	m_pDev->grabSetParams(pw, ph, VIDEO_PALETTE_RGB24);
	recording_colorspace=cspYUY2;
    }
    else if(colorspace==cspYV12){
	printf("yv12 renderer\n");
	m_pDev->grabSetParams(pw, ph, VIDEO_PALETTE_YUV420P);
    }
    else if(colorspace==cspYUY2){
	printf("yuy2 renderer\n");
	m_pDev->grabSetParams(pw, ph, VIDEO_PALETTE_YUV422);
    }
    else{
	printf("error - not this mode\n");
	return;
    }

    //connect(&m_CaptureTimer, SIGNAL(timeout()), this, SLOT(avicap_renderer_display()));
    //m_CaptureTimer.start(30);

    m_eMode=AvicapRend;
    //    m_pDev->setCapture(0);

    fs_renderer->setRecordingColorSpace(recording_colorspace);

    fs_renderer->createVideoRenderer();
    printf("after fsrend\n");

    connect(&m_avicapTimer, SIGNAL(timeout()), this, SLOT(avicap_renderer_display()));
    m_avicapTimer.start(40);

    hide();

}

void V4LWindow::multichannel_increase_speed(bool inc)
{
  if(inc)
    mc_switch/=2;
  else
    mc_switch*=2;

  if(mc_switch<1)
    mc_switch=1;

  multichannel();
}

void V4LWindow::multichannel_increase_tiles(bool inc)
{
  if(inc)
    mc_tiles++;
  else
    mc_tiles--;

  if(mc_tiles<1)
    mc_tiles=1;

  multichannel();
}

void V4LWindow::multichannel_stop()
{
  m_multichannelTimer.stop();
}

void V4LWindow::multichannel()
{
    int pw, ph, dx, dy;
    avm::Locker lock(m_Mutex);
    printf("multichannel()\n");

    if(m_eMode!=MultiChannel){
      m_eMode=MultiChannel;

      //stop shm renderer
      m_CaptureTimer.stop();
      delete m_pRenderer;
      m_pRenderer=0;

      avicap_renderer_stop();

      getOverlaySizeAndOffset(&pw, &ph, &dx, &dy);

      mc_oldw=pw;
      mc_oldh=ph;
      mc_curw=pw;
      mc_curh=ph;

      if(last_station<0){
	changeChannel(0);
      }
      mc_startchannel=last_station;
      mc_curchannel=0;
      mc_count=0;
    }
    
    mc_grabw=mc_curw/mc_tiles;
    mc_grabh=mc_curh/mc_tiles;

    printf("tiles=%d speed=%d\n",mc_tiles,mc_switch);
    m_pDev->setCapture(0);
    m_pDev->grabSetParams(mc_grabw, mc_grabh, get_palette(x11Display()));

    m_eMode=MultiChannel;

    connect(&m_multichannelTimer, SIGNAL(timeout()), this, SLOT(multichannel_display()));
    m_multichannelTimer.start(40);

}

void V4LWindow::maximize()
{
}

void V4LWindow::fullscreen()
{
}

void V4LWindow::avicap_renderer_resize_input(int w,int h)
{
    int pw, ph, dx, dy;
    getOverlaySizeAndOffset(&pw, &ph, &dx, &dy);
    printf("avicap-renderer resize input w=%d h=%d pw=%d ph=%d()\n",w,h,pw,ph);

    avicap_renderer();
}



void V4LWindow::avicap_renderer_display()
{
    avm::Locker lock(m_Mutex);
    //  printf("avicap_renderer_display()\n");
    const char* z=m_pDev->grabCapture(false);
    int pw, ph, dx, dy;
    //getOverlaySizeAndOffset(&pw, &ph, &dx, &dy);
    pw=fsr_width;
    ph=fsr_height;



    //    printf("pw=%d ph=%d\n",pw,ph);

    avm::CImage *im;
    //Colorspaces colorspace=(enum Colorspaces)RI("Colorspace", 0);
    Colorspaces colorspace=cspYUY2;
    BitmapInfo bi;
    if(colorspace==cspRGB24 && rgb_method==0){
	im=new avm::CImage((const uint8_t *)z,pw,ph);
    }
    else if(colorspace==cspRGB24 && rgb_method==1){
	//convert rgb24 to yuv automatic
	im=new avm::CImage((const uint8_t *)z,pw,ph);
    }
    else if(colorspace==cspRGB24 && rgb_method==2){
	//convert rgb24 to yuv, do-it-yourself
	const avm::CImage *im0=new avm::CImage((const uint8_t *)z,pw,ph);
	bi=BitmapInfo(pw,ph,fccYUY2);
	im=new avm::CImage(im0,&bi);
    }
    else if(colorspace==cspYV12){
	//      printf("yuv renderer\n");
	bi=BitmapInfo(pw,ph,fccYV12);

	im=new avm::CImage(&bi,(const uint8_t *)z,false);
    }
    else if(colorspace==cspYUY2){
	//    printf("yuv renderer\n");
	bi=BitmapInfo(pw,ph,fccYUY2);
	im=new avm::CImage(&bi,(const uint8_t *)z,false);
    }
    else{
	printf("error - not this mode\n");
	return;
    }

    fs_renderer->drawFrame(im);
}

void V4LWindow::preview_display()
{
    avm::Locker lock(m_Mutex);
    //     printf("preview_display()\n");
    const char* z=m_pDev->grabCapture(false);
    QPainter qp(this);
    m_pRenderer->draw(&qp, z, (m_eMode==DeinterlacedPreview));

#if 0
    //little bit of osd
    if(last_station>=0 && last_station<xawtvstations.size()){
	qp.setPen(QColor(0,255,0));
	qp.drawText(30,30,xawtvstations[last_station].sname.c_str());
    }
#endif

    m_pRenderer->sync();


    /*
     int pw, ph, dx, dy;
     getOverlaySizeAndOffset(&pw, &ph, &dx, &dy);

     int bpl=pw*4;
     QImage* i=new QImage(pw, ph, 32);
     for(int y=0; y<ph; y++)
     {
     QRgb *line = (QRgb *)i->scanLine(y);
     memcpy(line, z+bpl*y, bpl);
     }
     QPainter p(this);
     p.drawImage(dx, dy, *i);
     delete i;
     */
}

void V4LWindow::multichannel_display()
{
    avm::Locker lock(m_Mutex);
    //     printf("preview_display()\n");
    char* z=m_pDev->grabCapture(false);
    QPainter qp(this);

    int pw, ph, dx, dy;
    getOverlaySizeAndOffset(&pw, &ph, &dx, &dy);

    Display *dpy = x11Display();
    //Visual *vis = (Visual*)dev->x11Visual();

    int bit_depth = avm::GetPhysicalDepth(dpy);

    const QImage pix=QImage((uchar *)z,mc_grabw,mc_grabh,bit_depth,NULL,0,QImage::IgnoreEndian);
    
    int max_channels=mc_tiles*mc_tiles;

    int ys=mc_curchannel/mc_tiles;
    int xs=mc_curchannel%mc_tiles;
    int xo=xs*mc_grabw;
    int yo=ys*mc_grabh;

    qp.drawImage(xo,yo,pix);

    mc_count++;

    if(mc_count>mc_switch){
      mc_count=0;
      mc_curchannel++;
      if(mc_curchannel>=max_channels){
	mc_curchannel=0;
      }
      // printf("multichannel: %d\n",mc_startchannel+mc_curchannel);
      changeChannel(mc_startchannel+mc_curchannel);
      m_pDev->setAudioMute(true);    
      //m_pDev->setAudioVolume(0);
    
    }

}

void V4LWindow::ratioTimerStop()
{
    if(!m_bForce4x3)
	return;
    int w = width();
    int h = height()-(m_bSubtitles ? 80 : 0);
    if(w != 4*h/3)
    {
	printf("ratio time: subtitles %d, resizing %d,%d -> %d,%d\n",
	       (int)m_bSubtitles,
	       w, h, 4*h/3, h);
	resize(4*h/3, h + (m_bSubtitles ? 80 : 0));
    }
    //    if(m_bForce4x3 && w!=4*(h-(m_bSubtitles?80:0))/3)
    //    {
    //		printf("Recalc %d,%d\n", 4*(h-(m_bSubtitles?80:0))/3, h);
    //		postResizeEvent(4*(h-(m_bSubtitles?80:0))/3, h);
    //		resize(4*(h-(m_bSubtitles?80:0))/3, h);
    //		return;
    //    }
}
