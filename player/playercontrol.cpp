/********************************************************

	Main AVI player widget
	Copyright 2000-2002 Zdenek Kabelac  (kabi@users.sf.net)
	original author: Eugene Kuznetsov (divx@euro.ru)

*********************************************************/

#include "playercontrol.h"
#include "playercontrol.moc"

#include "decoder_config.h"
#include "configdialog_impl.h"

#define DECLARE_REGISTRY_SHORTCUT
#include "configfile.h"
#undef DECLARE_REGISTRY_SHORTCUT

#include "avm_output.h"
#include "avm_creators.h"
#include "avm_except.h"
#include "renderer.h"
#include "utils.h"
#include "version.h"
#include "videodecoder.h"

#include "pixmapbutton.h"

#include <qcursor.h>
#include <qdragobject.h>
#include <qfiledialog.h>
#include <qgroupbox.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qmessagebox.h>
#include <qpainter.h>
#include <qpopupmenu.h>
#include <qprogressbar.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qtimer.h>

#ifdef HAVE_LIBSDL
#include <SDL_keysym.h>
#endif

#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>


const char* g_pcProgramName = "aviplay";
static const int MAX_VBAR = 100;
static const int MAX_MBAR = 1000;



QPlayerWidget::QPlayerWidget(PlayerControl* c)
    : QWidget( 0, 0, WDestructiveClose ), m_pControl(c)
{
    QPixmap pm;
    pm.load( PIXMAP_PATH "/test.png" );
    setIcon( pm );

    QVBoxLayout* top_vbl = new QVBoxLayout( this, 0 );
    top_vbl->setMargin( 2 );
    top_vbl->setSpacing( 2 );

    QBoxLayout* line1_hbl = new QHBoxLayout( top_vbl, 1 );

    b_open = new QavmPixmapButton( "open", this );
    line1_hbl->addWidget( b_open );

    b_pause = new QavmPixmapButton( "pause", this );
    line1_hbl->addWidget( b_pause );

    b_play = new QavmPixmapButton( "play", this );
    line1_hbl->addWidget( b_play );

    b_stop = new QavmPixmapButton( "stop", this );
    line1_hbl->addWidget( b_stop );

    line1_hbl->addSpacing( 4 );

    m_pScroller = new QSlider( 0, MAX_MBAR, MAX_MBAR / 10, 0,
			      QSlider::Horizontal, this );
    m_pScroller->setTickmarks( QSlider::Both );
    m_pScroller->setTickInterval( MAX_MBAR / 10 );
    m_pScroller->setMaximumHeight( 18 );
    m_pScroller->setSteps( 5, MAX_MBAR/10 );
    line1_hbl->addWidget( m_pScroller );

    line1_hbl->addSpacing( 4 );

    b_about = new QavmPixmapButton( "about", this );
    line1_hbl->addWidget( b_about );

    QBoxLayout* line2_hbl = new QHBoxLayout( top_vbl, 1 );
    b_mute = new QavmPixmapButton( "mute", this );
    line2_hbl->addWidget( b_mute );

    line2_hbl->addSpacing( 4 );

    vbar = new QSlider( 0, MAX_VBAR, 5, MAX_VBAR, QSlider::Horizontal, this );
    vbar->setMaximumHeight( 18 );
    vbar->setMaximumWidth( 80 );
    //vbar->setTickmarks(QSlider::Above);
    line2_hbl->addWidget( vbar, 20 );

    line2_hbl->addSpacing( 2 );

    pbar = new QProgressBar( this );
    pbar->setTotalSteps( 100 );
    pbar->setMaximumHeight( 18 );
    pbar->setMaximumWidth( 80 );
    pbar->setProgress( 0 );
    //pbar->setIndicatorFollowsStyle( FALSE );
    line2_hbl->addWidget( pbar, 1 );

    line2_hbl->addSpacing( 2 );

    m_pLStatus = new QLabel( this );
    line2_hbl->addWidget( m_pLStatus );
    line2_hbl->addStretch( 10 );

    m_pLTime = new QLabel( this );
    line2_hbl->addWidget( m_pLTime );
    //line2_hbl->setStretchFactor( m_pLTime );
}

bool QPlayerWidget::event(QEvent* e)
{
    //printf("EVENT %d\n", e->type());
    switch (e->type())
    {
    case Q_MenuEvent:
	m_pControl->menuSlot();
        break;
    case Q_MyPosEvent:
	m_pControl->updatePos();
        break;
    case Q_MyCloseEvent:
	if (!m_pControl->m_Dialog.TryLock()) {
            m_pControl->m_Dialog.Unlock();
	    close();
	}
        break;
    case Q_MyRefreshEvent:
	m_pControl->refresh();
        break;
    case Q_MyOpenEvent:
	m_pControl->openSlot();
        break;
    case Q_MyAboutEvent:
	m_pControl->aboutMovie();
        break;
    case Q_MyConfigEvent:
	m_pControl->showconfSlot();
        break;
    case Q_MyResizeEvent:
	m_pControl->resizePlayer(((QMyResizeEvent*) e)->m_x,
				 ((QMyResizeEvent*) e)->m_y);
        break;
#if QT_VERSION < 220
    case QEvent::Accel:
#endif
    case QEvent::KeyPress:
	m_pControl->keyPressEvent((QKeyEvent*)e);
        break;
    case QEvent::Quit:
    case QEvent::Close: // Have to be checked here - for safe destruction
        m_pControl->endPlayer();
    default:
	return QWidget::event(e);  // parent call
    }
    return true;
}

/*
 *  Player Control
 */
PlayerControl::PlayerControl()
    : QObject( 0, 0 )
{
    m_pMW = 0;
    player = 0;
    popup = 0;
    m_pAboutPopup = 0;
    m_pOpenPopup = 0;
    m_iResizeCount = 1;
    m_iLastVolume = -1;
    m_bIsTracking = false;
    m_bInReseek = false;
    m_bKeyProcessed = false;
    m_bUnpause = false;
    m_dStartTime = 0.0;
    m_pTimer = 0;
    // Here we can set an icon for the application.
    // Does anyone know how to draw an AVI player? :) No? Well,
    // then let's leave this for a while.

    m_TimerStarter = new QTimer(this);
    QObject::connect( m_TimerStarter, SIGNAL(timeout()), this, SLOT(starterSlot()));

    //return;

#if QT_VERSION >= 300
    //QStyle& origth = qApp->style();
    avm::string usedefth = RS("theme", "default");
    if (usedefth != "default")
	qApp->setStyle( usedefth.c_str() );
    // FIXME me - rememeber original style somehow
#endif
    m_pTimer = new QTimer(this);
    QObject::connect(m_pTimer, SIGNAL(timeout()), this, SLOT(updatePos()));

    m_pMW = new QPlayerWidget( this );

    connect(m_pMW->m_pScroller, SIGNAL(sliderReleased()), this, SLOT(reseekSlot()));
    connect(m_pMW->m_pScroller, SIGNAL(sliderPressed()), this, SLOT(sliderSlot()));
    connect(m_pMW->m_pScroller, SIGNAL(sliderMoved(int)), this, SLOT(movedSlot(int)));
    connect(m_pMW->m_pScroller, SIGNAL(valueChanged(int)), this, SLOT(changedSlot(int)));

    //connect(m_pScroller, SIGNAL(sliderPressed()), this, SLOT(sliderSlot()));
    //connect(scroller, SIGNAL(sliderMoved(int)), this, SLOT(movedSlot(int)));
    connect(m_pMW->vbar, SIGNAL(valueChanged(int)), this, SLOT(volumeChanged(int)));

    connect(m_pMW->b_pause, SIGNAL(clicked()), this, SLOT(pauseSlot()));
    connect(m_pMW->b_stop, SIGNAL(clicked()), this, SLOT(stopSlot()));
    connect(m_pMW->b_play, SIGNAL(clicked()), this, SLOT(playSlot()));
    connect(m_pMW->b_about, SIGNAL(clicked()), this, SLOT(aboutSlot()));
    connect(m_pMW->b_mute, SIGNAL(clicked()), this, SLOT(muteChanged()));
    connect(m_pMW->b_open, SIGNAL(clicked()), this, SLOT(openpopupSlot()));

    m_pMW->setAcceptDrops(TRUE);
    m_pMW->resize(400, 30);
    m_pMW->show();
}

PlayerControl::~PlayerControl()
{
    delete m_pTimer;
    delete m_TimerStarter;
}

void PlayerControl::sendQt(QEvent* e)
{
    if (m_pMW)
    {
	qApp->postEvent(m_pMW, e);
	// event deleted by Qt
#if QT_VERSION > 220
	qApp->wakeUpGuiThread();
#endif
    }
}

void PlayerControl::sendKeyEvent(int qtkey, int qtascii, int qtstate)
{
    m_bKeyProcessed = true;
    sendQt(new QKeyEvent(QEvent::KeyPress, qtkey, qtascii, qtstate));
}

void PlayerControl::keySlot(int sym, int mod)
{
#ifdef HAVE_LIBSDL
    //printf("Key %d   mod %d   process: %d\n", sym, mod, m_bKeyProcessed);
    //if (m_bKeyProcessed) return;
    // queue new key only if the previous one has been processed
#define SDL_UNSAFE  0x40000000
    // SDL -> QT key translation
    static const struct keytr {
	int sdlkey;
	int qtkey;
	int qtascii;
	int qtstate;
    } sdlqttable[] =
    {
	{ SDLK_RETURN, Qt::Key_Return },
	{ SDLK_BACKSPACE, Qt::Key_Backspace },
	{ SDLK_LEFT, Qt::Key_Left },
	{ SDLK_PAGEDOWN, Qt::Key_PageDown },
	{ SDLK_RIGHT, Qt::Key_Right },
	{ SDLK_PAGEUP, Qt::Key_PageUp },
	{ SDLK_UP, Qt::Key_Up },
	{ SDLK_DOWN, Qt::Key_Down },
	{ SDLK_HOME, Qt::Key_Home },
	{ SDLK_END, Qt::Key_End },
	{ SDLK_INSERT | SDL_UNSAFE, Qt::Key_Insert },
	{ SDLK_DELETE | SDL_UNSAFE, Qt::Key_Delete },
#if QT_VERSION > 220
	{ SDLK_KP_PLUS, Qt::Key_Plus, 0, Qt::Keypad },
	{ SDLK_KP_MINUS, Qt::Key_Minus, 0, Qt::Keypad },
#endif
	{ SDLK_1 | SDL_UNSAFE, Qt::Key_1, '1' },
	{ SDLK_2 | SDL_UNSAFE, Qt::Key_2, '2' },
	{ SDLK_3 | SDL_UNSAFE, Qt::Key_3, '3' },
	{ SDLK_4 | SDL_UNSAFE, Qt::Key_3, '4' },
	{ SDLK_5 | SDL_UNSAFE, Qt::Key_3, '5' },
	{ SDLK_6 | SDL_UNSAFE, Qt::Key_3, '6' },
	{ SDLK_7 | SDL_UNSAFE, Qt::Key_3, '7' },
	{ SDLK_a | SDL_UNSAFE, Qt::Key_A, 'A' },
	{ SDLK_z | SDL_UNSAFE, Qt::Key_Z, 'Z' },
	{ SDLK_f | SDL_UNSAFE, Qt::Key_F, 'F' },
	{ SDLK_m | SDL_UNSAFE, Qt::Key_M, 'M' },
	{ SDLK_n | SDL_UNSAFE, Qt::Key_N, 'N' },
	{ SDLK_o, Qt::Key_O, 'O' },
	{ SDLK_l, Qt::Key_L, 'L' },
	{ SDLK_i, Qt::Key_I, 'I' },
	{ SDLK_LEFTBRACKET, Qt::Key_BraceLeft, '[' },
	{ SDLK_RIGHTBRACKET, Qt::Key_BraceRight, ']' },
	{ SDLK_F1, Qt::Key_F1 },
	{ SDLK_F2, Qt::Key_F2 },
	{ SDLK_F3, Qt::Key_F3 },
	{ SDLK_F4, Qt::Key_F4 },
	{ SDLK_F5, Qt::Key_F5 },
	{ SDLK_F6, Qt::Key_F6 },
	{ SDLK_F7, Qt::Key_F7 },
	{ SDLK_F8, Qt::Key_F8 },
	{ SDLK_F9, Qt::Key_F9 },
	{ SDLK_F10, Qt::Key_F10 },
	{ 0 }
    };

    int i = 0;
    while (sdlqttable[i].sdlkey
	   && (sdlqttable[i].sdlkey & ~(SDL_UNSAFE)) != sym)
	i++;

    if (sdlqttable[i].sdlkey)
    {
	if (sdlqttable[i].sdlkey & SDL_UNSAFE)
	{
	    sendKeyEvent(sdlqttable[i].qtkey,
			 sdlqttable[i].qtascii,
			 sdlqttable[i].qtstate);
	}
	else
	{
	    // testing thread stability & safety
	    // code path for actions which may be
            // implemented directly without Qt passthrough
            QKeyEvent e(QEvent::KeyPress,
			sdlqttable[i].qtkey,
			sdlqttable[i].qtascii,
			sdlqttable[i].qtstate);
	    keyPressEvent(&e);
	}
    }
#undef SDL_UNSAFE
#endif
}

void PlayerControl::menuSlot()
{
    if (!popup)
    {
	popup = new QPopupMenu();
#if 1
	//popup->insertTearOffHandle();
	QPopupMenu* p = new QPopupMenu();
	//connect( p, SIGNAL( activated( int ) ), this, SLOT( zoom_mode( int ) ) );
	if (p)
	{
	    p->insertItem( tr( "Size 2x    &3" ), this, SLOT( zoom_2x() ) );
	    p->insertItem( tr( "Size 1x    &2" ), this, SLOT( zoom_1x() ) );
	    p->insertItem( tr( "Size 0.5x  &1" ), this, SLOT( zoom_0_5x() ) );
	    p->insertItem( tr( "Size +10%  &A" ), this, SLOT( zoom_plus() ) );
	    p->insertItem( tr( "Size -10%  &Z" ), this, SLOT( zoom_minus() ) );
	    p->insertItem( tr( "Crop Top     " ), this, SLOT( crop_top() ) );
	    p->insertItem( tr( "Crop Bottom  " ), this, SLOT( crop_bottom() ) );
	    p->insertItem( tr( "Crop Left    " ), this, SLOT( crop_left() ) );
	    p->insertItem( tr( "Crop Right   " ), this, SLOT( crop_right() ) );
	    p->insertItem( tr( "Crop 16:9  &6" ), this, SLOT( crop_16_9() ) );
	    p->insertItem( tr( "Crop 4:3   &4" ), this, SLOT( crop_4_3() ) );
            popup->insertItem( tr( "Zoom" ), p);
	}

	p = new QPopupMenu();
	//connect( p, SIGNAL( activated( int ) ), this, SLOT( zoom_mode( int ) ) );
	if (p)
	{
	    p->insertItem( tr( "Original" ), this, SLOT( ratio_orig() ) );
	    p->insertItem( tr( "16:9" )    , this, SLOT( ratio_16_9() ) );
	    p->insertItem( tr( "4:3" )     , this, SLOT( ratio_4_3() ) );
            popup->insertItem( tr( "Aspect Ratio" ), p);
	}
	popup->insertSeparator();
	popup->insertItem( tr( "&Fullscreen" ), this, SLOT( fullscreen() ), CTRL+Key_F );
	popup->insertItem( tr( "&Maximize" ), this, SLOT( maximize() ) );

	const avm::vector<avm::IVideoRenderer*>& vvr = player->GetVideoRenderers();
	if (vvr.size() > 0)
	{
	    modes = new QPopupMenu();

	    connect(modes, SIGNAL(activated(int)),
		    this, SLOT(maximizeMode(int)));

	    unsigned i = 0;
	    while (i < vvr.size())
	    {
		const avm::vector<avm::VideoMode>& vvm = vvr[i]->GetVideoModes();
		unsigned j = 0;
		while (j < vvm.size())
		{
		    modes->insertItem(vvm[j].name.c_str(), (i << 8) + j);
		    j++;
		}
		i++;
	    }
	    popup->insertItem( tr( "Maximize Modes" ), modes);
	}
#endif
	popup->insertSeparator();
	popup->insertItem( tr( "&Properties" ), this, SLOT(aboutMovie()));

        bool is = false;
	if (player->GetRtConfig(player->AUDIO_CODECS)
	    || player->GetCodecInfo().decoder_info.size())
	{
            is = true;
	    popup->insertSeparator();
	    popup->insertItem( tr("&Audio Decoder config"), this, SLOT( audioDecoderConfig() ) );
	}
	if (player->GetRtConfig(player->VIDEO_CODECS)
	    || player->GetCodecInfo().decoder_info.size())
	{
	    if (!is)
                popup->insertSeparator();
            is = true;
	    popup->insertItem( tr("&Video Decoder config"), this, SLOT( videoDecoderConfig() ) );
	}
	if (player->GetRtConfig(player->VIDEO_RENDERER))
	{
	    if (!is)
                popup->insertSeparator();
	    popup->insertItem( tr("Video &Renderer config"), this, SLOT( rendererConfig() ) );
	}

	popup->insertItem( tr("P&layer config"), this, SLOT( sendConfig() ) );

	popup->hide();
    }

    if (popup->isVisible())
    {
	//printf("VISIBLE\n");
	popup->hide();
    }
    else
    {
	//printf("INVISIBLE\n");
	//popup->move( QCursor::pos() );
	//popup->show();
	popup->exec( QCursor::pos() );
    }
}

void PlayerControl::zoom(double scale)
{
    if (player)
    {
	player->Zoom(0, 0, 0, 0); // disable any zoom
        if (scale == 2.0)
	    m_iResizeCount = 2;
	else if (scale == 1.0)
	    m_iResizeCount = 1;
        else
	    m_iResizeCount = 0;

	int h = (int) (scale * player->GetHeight() + 0.5);
	int w = player->GetWidth();
	avm::StreamInfo* si = player->GetVideoStreamInfo();
	if (si)
	{
	    float a = si->GetAspectRatio();
	    w = (int)((a > 0) ? h * a : scale * w + 0.5);
	    delete si;
	    player->Resize(w, h);
	}
    }
}

void PlayerControl::zoom_mode(int mode)
{
    int x = 0;
    int y = 0;
    int w = player->GetWidth();
    int h = player->GetHeight();
    int wo = w, ho = h;
    int vw, vh;
    if (!w || !h)
        return; // no picture

    player->Get(player->QUERY_VIDEO_WIDTH, &vw,
		player->QUERY_VIDEO_HEIGHT, &vh, 0);

    float r = vw / (float)vh;
    float s = vw / w;
    //printf("W %d  H %d     %d x %d  %f\n", vw, vh, w, h, r);
    switch (mode)
    {
    case ZOOM_PLUS:
	w = ((vw * 11) / 10 + 7) & ~7;
        h = (int) (w / r + 0.5);
	break;
    case ZOOM_MINUS:
	w = ((vw * 9) / 10 + 7) & ~7;
        h = (int) (w / r + 0.5);
	break;
    case CROP_4_3:
        setRatio(x, y, w, h, 4, 3);
        break;
    case CROP_16_9:
        setRatio(x, y, w, h, 16, 9);
        break;
    }

    ho = h * vw / wo;
    wo = w * vw / wo;
    switch (mode)
    {
    case ZOOM_PLUS:
    case ZOOM_MINUS:
        printf("zoom  %d x %d\n", w, h);
	player->Resize(w, h);
	break;
    case CROP_16_9:
    case CROP_4_3:
        printf("crop  %d %d   %d x %d\n", x, y, w, h);
	player->Zoom(x, y, w, h);
	player->Resize(wo, ho);
	break;
    }
}

void PlayerControl::ratio(int r)
{
    int w = player->GetWidth();
    int h = player->GetHeight();

    switch (r)
    {
    case RATIO_ORIG:
	break;
    case RATIO_16_9:
	w = 16 * h / 9;
	break;
    case RATIO_4_3:
	w = 4 * h / 3;
	break;
    }
    player->Resize(w, h);
}

void PlayerControl::zoomWH(int new_w, int new_h, int ori_w, int ori_h)
{
    printf("zoomWH  %dx%d\n", new_w, new_h);

    if (ori_w > ori_h)
	new_h = (int) (new_w * ori_h / (double) ori_w + 0.5);
    else
	new_w = (int) (new_h * ori_w / (double) ori_h + 0.5);
    player->Resize(new_w, new_h);
    fullscreen();
}

void PlayerControl::fullscreen()
{
    if (player)
    {
        apip.fullscreen = !apip.fullscreen;
	player->ToggleFullscreen(false);
    }
}

void PlayerControl::maximize()
{
    if (player)
    {
        apip.maximize = !apip.maximize;
	player->ToggleFullscreen(true);
    }
}

void PlayerControl::maximizeMode(int renderer_mode)
{
    if (renderer_mode < 0)
	return;

    const avm::vector<avm::IVideoRenderer*>& vvr = player->GetVideoRenderers();
    unsigned renderer = (renderer_mode >> 8);
    if (renderer < vvr.size())
    {
        unsigned mode = renderer_mode & 0xff;
	const avm::vector<avm::VideoMode>& vvm = vvr[renderer]->GetVideoModes();
	if (mode < vvm.size())
	{
	    int ori_w, ori_h;
	    int new_w = vvm[mode].width;
	    int new_h = vvm[mode].height;

	    if (vvr[renderer]->GetSize(ori_w, ori_h) < 0)
	    {
		ori_w = player->GetWidth();
		ori_h = player->GetHeight();
	    }
	    zoomWH(new_w, new_h, ori_w, ori_h);
	}
    }
}

void PlayerControl::showconfSlot()
{
    if (m_Dialog.TryLock())
	return;
    if (!player)
        initPlayer(apip);
    // Qt is buggy - will crash if
    // player quits and this window is opened
    ConfigDialog_impl dilg(this);
    delete popup;
    popup = 0;
    dilg.exec();
    readConfig();
    m_Dialog.Unlock();
}

void PlayerControl::decoderConfig(int type)
{
    if (!player || !player->IsValid() || m_Dialog.TryLock())
	return;

    avm::IRtConfig* rt = player->GetRtConfig(type);
    QCodecConf qdlg(player->GetCodecInfo(type), rt);
    if (qdlg.exec() == QDialog::Accepted)
    {
	m_bInReseek = true;
	if (!rt)
	{
            qdlg.valueChanged(0);
	    player->Restart();
	}
	//this call blocks until main processing thread
	//is stopped, so it can safely restart decoder.
	//Operations with _in_reseek and mutex guarantee that
	//we won't receive a deadlock.
	m_bInReseek = false;
    }
    m_Dialog.Unlock();
}

void PlayerControl::rendererConfig()
{
    if (!player || !player->IsValid() || m_Dialog.TryLock())
	return;

    avm::IRtConfig* rt = player->GetRtConfig(player->VIDEO_RENDERER);
    if (rt)
    {
	const avm::vector<avm::AttributeInfo>& ai = rt->GetAttrs();
	//printf("RtSize %d\n", ai.size());
	//for(unsigned i = 0; i < ai.size(); i++)
	//    printf("%s  %s\n", ai[i].GetName(), ai[i].GetAbout());

	QRendererConf qdlg( ai, rt );
	qdlg.exec();
    }
    m_Dialog.Unlock();
}


// this method should be used for all volume changes
// for common interpretation
void PlayerControl::volumeChanged(int vpos)
{
    if (player)
	player->Set(player->AUDIO_VOLUME, vpos * 1000 / MAX_VBAR, 0);
}

void PlayerControl::muteChanged()
{
    if (m_pMW)
    {
	int p = m_pMW->vbar->value();
	int n = m_iLastVolume;
	if (p != 0) {
	    m_iLastVolume = p;
	    n = 0;
	}
	m_pMW->vbar->setValue(n);
    }
}

void PlayerControl::pauseSlot()
{
    m_bUnpause = false;
    if (player)
	player->Pause(!player->IsPaused());
}

void PlayerControl::stopSlot()
{
    if (player)
	player->Stop();
}

void PlayerControl::changedSlot(int cpos)
{
    if (player && m_pMW)
    {
	bool wrapAround = cpos < (m_pMW->m_pScroller->minValue() + 1)
	    && last_slider > (m_pMW->m_pScroller->maxValue() - 1);
	//printf("WRAP %d %d\n", wrapAround, last_slider);
	// when paused or when mouse wheel movement...
	int minchange = (player->IsPaused()) ? 0 : 2;
	if (!m_bSettingScroller
	    && (!m_bIsTracking || player->IsPaused())
	    && !wrapAround && abs(last_slider - cpos) > minchange)
	{
            double t = cpos * m_dLengthTime / MAX_MBAR;
	    player->ReseekExact(t + m_dStartTime);
	    //printf("reseek %f %f %d\n", t+m_dStartTime,  m_dLengthTime, cpos);
	}
	last_slider = cpos;
    }
}

void PlayerControl::nextPage()
{
    if (player && player->NextKeyFrame() == 0)
	updatePos();
}

void PlayerControl::prevPage()
{
    if (player && player->PrevKeyFrame() == 0)
	updatePos();
}

void PlayerControl::playSlot()
{
    if (player)
    {
	if (!player->IsPlaying() && !player->IsPaused())
	{
	    player->Start();
	    m_dStartTime = player->GetTime();
	}
	else
	    player->Play();
        updatePos();
    }
}

void PlayerControl::openSlot()
{
    if (m_Dialog.TryLock())
	return;

    avm::string path = RS("url", ".");
    char* cut = strrchr(path.c_str(), '/');
    if (!cut)
	path = "./";
#if QT_VERSION <= 220
    else
	*cut = 0;
#endif
    //QFileDialog *fld = new QFileDialog();
    //open_qs = fld->getOpenFileName(tr(path.c_str()), tr("AVI files (*.avi *.AVI);;ASF files (*.asf *.ASF);;All files(*)"));
    //mb->setTextFormat(Qt::RichText);
    QStringList o = QFileDialog::getOpenFileNames(
#if QT_VERSION > 220
						  tr( "AVI files (*.avi *.AVI);;"
						     "ASF files (*.asf *.ASF *.wma *.WMA *.wmv *.WMV);;"
						     "MPEG files (*.mpg *.MPG *.mpeg *.MPEG *.mp3 *.MP3 *.m2v *.M2V *.mp2 *.MP2 *.mpv2 *.MPV2);;"
						     "QuickTime files (*.mov *.MOV *.qt *.QT);;"
						     "All files (*)" )
#else
						  ""
#endif
						  , path.c_str(),
						  0, 0
#if QT_VERSION > 220
						  , tr( "Open video file(s)" )
#endif
						 );
    m_Dialog.Unlock();
    if (o.isEmpty())
	return;

    apip.urls.clear();
    apip.idx = 0;
    for (unsigned i = 0; i < o.count(); i++)
	apip.urls.push_back((const char*)o[i].local8Bit());
    initPlayer(apip);
}

void PlayerControl::opensubSlot()
{
    if (!player || m_Dialog.TryLock())
        return;

    avm::string path = RS("subtitle-url", ".");
    QString o = QFileDialog::getOpenFileName( path.c_str(),
#if QT_VERSION > 220
					     tr(
						"SUB files (*.sub *.SUB *.txt *.TXT);;"
						"SRT files (*.srt *.SRT);;"
						"SAMI files (*.smi *.SMI);;"
						"All files (*)"
					       ),
#else
					     "",
#endif
					     0, 0
#if QT_VERSION > 220
					     , tr( "Open subtitle file" )
#endif
					  );
    if (!o.isNull()) {
	player->InitSubtitles(o.local8Bit());
	WS("subtitle-url", (const char *)o.local8Bit());
    }

    m_Dialog.Unlock();
}

void PlayerControl::openaudSlot()
{
    if (!player || m_Dialog.TryLock())
        return;

    char* astr = 0;
    player->Get(player->AUDIO_URL, &astr, 0);
    QString o = QFileDialog::getOpenFileName( astr,
#if QT_VERSION > 220
					     tr( "All files (*)" ),
#else
					     "",
#endif
					     0, 0
#if QT_VERSION > 220
					     , tr( "Open audio file" )
#endif
					    );
    if (astr)
	free(astr);

    if (!o.isNull())
	player->Set(player->AUDIO_URL, (const char*)o.local8Bit(), 0);

    m_Dialog.Unlock();
}

void PlayerControl::keyPressEvent(QKeyEvent * kevent)
{
    avm::Locker locker(m_Mutex);
    // for SDL events are also mirrored in EventFilter ( see renderer.cpp )
    //printf("KEYPRESS %d   ascii: %c\n", kevent->key(), kevent->ascii());
    m_bKeyProcessed = false;
    switch(toupper(kevent->ascii()))
    {
    case 'Q': sendEvent(Q_MyCloseEvent); return;
    case 'X': stopSlot(); return;
    case 'C':
    case 'P':
    case ' ':
	// try to support most natural keystrokes
	pauseSlot();
	return;
    case '[':
	if (player)
	{
	    int t;
            player->Get(avm::IAviPlayer::ASYNC_TIME_MS, &t, 0);
	    player->Set(avm::IAviPlayer::ASYNC_TIME_MS, ((t - 100) * 100) / 100, 0);
	    printf("Set async %dms\n", t);
	}
	return;
    case ']':
	if (player)
	{
	    int t;
	    player->Get(avm::IAviPlayer::ASYNC_TIME_MS, &t, 0);
	    player->Set(avm::IAviPlayer::ASYNC_TIME_MS, ((t + 100) * 100) / 100, 0);
	    printf("Set async %dms\n", t);
	}
	return;
    case 'V': playSlot(); return;
    case 'F': fullscreen(); return;
    case 'M': maximize(); return;
    case '1': zoom(0.5); return;
    case '2': zoom(1.0); return;
    case '3': zoom(2.0); return;
    case '4': crop_4_3(); return;
    case '6': crop_16_9(); return;
    case 'A': zoom_mode(ZOOM_PLUS); return;
    case 'Z': zoom_mode(ZOOM_MINUS); return;
    case 'L': sendEvent(Q_MyOpenEvent); return;
    case 'I': sendEvent(Q_MyAboutEvent); return;
    case 'N':
	{
	    int maxa;
	    int cura;
	    player->Get(avm::IAviPlayer::QUERY_AUDIO_STREAMS, &maxa,
			avm::IAviPlayer::AUDIO_STREAM, &cura, 0);
	    cura++;
	    //printf("ASTREAM %d  %d\n", cura, maxa);
	    if (cura >= maxa)
		cura = 0;
	    player->Set(avm::IAviPlayer::AUDIO_STREAM, cura, 0);
	}
        return;
    }

    switch(kevent->key())
    {
    case Qt::Key_Return: player->NextFrame(); return;
    case Qt::Key_Backspace: player->PrevFrame(); return;
    case Qt::Key_Right:	player->NextKeyFrame(); return;
    case Qt::Key_Left: player->PrevKeyFrame(); return;
    case Qt::Key_Home: player->ReseekExact(0); return;
    case Qt::Key_End:  player->ReseekExact(player->GetLengthTime() - 1.); return;
    case Qt::Key_Plus:
	if (m_pMW) m_pMW->vbar->addStep();
	return;
    case Qt::Key_Minus:
        if (m_pMW) m_pMW->vbar->subtractStep();
	return;
    case Qt::Key_Insert:
    case Qt::Key_Delete:
	if (apip.urls.size() > 1)
	{
	    if (kevent->key() == Qt::Key_Delete)
	    {
		if (++apip.idx >= apip.urls.size())
		    apip.idx = 0;
	    }
	    else
	    {
		if (--apip.idx >= apip.urls.size())
		    apip.idx = apip.urls.size() - 1;
	    }
            initPlayer(apip);
	}
	return;
    case Qt::Key_Down:
    case Qt::Key_PageDown:
	{
            double sktime = (kevent->key() == Qt::Key_Down) ? 60. : 10.;
	    double o = player->GetTime();
	    double p = o + sktime;
	    double len = player->GetLengthTime();
	    if (p > len)
		// we are at the end - try 5 seconds before end
		p = len;
	    player->ReseekExact(p);
	    if (player->GetTime() <= (o + 1.0))
		// some movies have very long intervals between KeyFrames
		// so always skip at least to the next keyframe
		player->NextKeyFrame();
	}
	return;
    case Qt::Key_Up:
    case Qt::Key_PageUp:
	{
            double sktime = (kevent->key() == Qt::Key_Up) ? 60. : 10.;
	    double p = player->GetTime() - sktime;
	    if (p < 0)
		p = 0.;
	    player->ReseekExact(p);
	}
	return;
    }

    static const struct runtimecnf {
	int qtkey;
	const char* attr;
	int add;
    } rttable[] = {
	{ Qt::Key_F1, "saturation", -2 },
	{ Qt::Key_F2, "saturation", 2 },
	{ Qt::Key_F5, "brightness", -2 },
	{ Qt::Key_F6, "brightness", 2 },
	{ Qt::Key_F7, "hue", -2 },
	{ Qt::Key_F8, "hue", 2 },
	{ Qt::Key_F9, "contrast", 2 },
	{ Qt::Key_F10, "contrast", -2 },
	{ 0, 0, 0 }
    };

    int i = 0;
    avm::IRtConfig* rt = 0;
    bool cset = true;
    while (rttable[i].qtkey)
    {
	if (kevent->key() == rttable[i].qtkey)
	{
	    if (rt == 0)
	    {
		rt = player->GetRtConfig();
		if (rt == 0)
		{
		    const avm::vector<avm::IVideoRenderer*>& v = player->GetVideoRenderers();
		    if (v.size())
		    {
			rt = v.front()->GetRtConfig();
			if (!rt)
                            break;
		    }
		}
	    }
	    const avm::vector<avm::AttributeInfo>& attrs = rt ?
		rt->GetAttrs() : player->GetCodecInfo().decoder_info ;
            int val;

	    for (unsigned j = 0; j < attrs.size(); j++)
	    {
                const char* attr = attrs[j].GetName();
		if (strcasecmp(rttable[i].attr, attr) == 0
		    || strcasecmp(rttable[i].attr, attrs[j].GetAbout()) == 0)
		{
		    if (rt->GetValue(attr, &val) == 0)
		    {
			val += rttable[i].add;
			if (val < attrs[j].i_min)
			    val = attrs[j].i_min;
			else if (val > attrs[j].i_max)
			    val = attrs[j].i_max;

			if (rt)
			    rt->SetValue(attr, val);
			else
			    avm::CodecSetAttr(player->GetCodecInfo(), attr, val);
			return;
		    }
		}
	    }
	}
        i++;
    }
}

void PlayerControl::mouseDoubleClickEvent(QMouseEvent *)
{
    showconfSlot();
}

void PlayerControl::aboutSlot()
{
    if (!m_pAboutPopup)
    {
	m_pAboutPopup = new QPopupMenu();
	m_pAboutPopup->insertItem( tr( "&Usage" ), this, SLOT( help() ) );
	m_pAboutPopup->insertItem( tr( "&About" ), this, SLOT( about() ) );
	m_pAboutPopup->insertItem( tr( "&Config" ), this, SLOT( showconfSlot() ) );
	m_pAboutPopup->insertSeparator();
	m_pAboutPopup->insertItem( tr( "&Properties" ), this, SLOT( aboutMovie() ) );
    }

    m_pAboutPopup->popup( QCursor::pos() );
}

void PlayerControl::openpopupSlot()
{
    if (!m_pOpenPopup)
    {
	m_pOpenPopup = new QPopupMenu();
	m_pOpenPopup->insertItem( tr( "Open &File" ), this, SLOT( openSlot() ) );
	m_pOpenPopup->insertItem( tr( "Open &URL" ), this, SLOT( openSlot() ) );
	m_pOpenPopup->insertItem( tr( "Open &Subtitle File" ), this, SLOT( opensubSlot() ) );
	m_pOpenPopup->insertItem( tr( "Open &Audio File" ), this, SLOT( openaudSlot() ) );
    }

    m_pOpenPopup->popup( QCursor::pos() );
}

void PlayerControl::resizePlayer(int w, int h)
{
    if (player)
	player->Resize(w, h);
}

void PlayerControl::refresh()
{
    if (player)
	player->Refresh();
}

void PlayerControl::remote_command()
{
    char command[32];
    int command_pos = 0;
    static long oldpos;
    long newpos = (long) getTime();
    fd_set fds; // stdin changed?
    struct timeval tv; // how long to w8
    int n; // indicating whether something happened on stdin

    if (isStopped())
	sendEvent(Q_MyCloseEvent);
    if (newpos != oldpos) {
	oldpos = newpos;
	printf ("Pos: %li s / %li s\n", (long) getTime(),
		(long) player->GetLengthTime());
    }
    tv.tv_sec = 0;
    tv.tv_usec = 50000;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    if ((n = select(32, &fds, NULL, NULL, &tv)) != 0)
    {
	double cpos;
	int i;
	int complete = 0;

 	while (command_pos < 32 &&
	    (i = read (STDIN_FILENO, command + command_pos, 1)) > 0 &&
	    command[command_pos] != '\r' &&
	    command[command_pos] != '\n')
	    command_pos++;

	if (command_pos == 31 || command[command_pos] == '\r' ||
	    command[command_pos] == '\n') {
	    command[command_pos] = 0;
	    complete = 1;
	}
	if (complete == 1) {
	    if (command[0] == 'p' || command[0] == 'P')
		pauseSlot();
	    else if (command[0] == 'f' || command[0] == 'F')
		fullscreen();
	    else if (command[0] == 'm' || command[0] == 'M')
		maximize();
	    else if (command[0] == 's' || command[0] == 'S')
		stopSlot();
	    else if (command[0] == 'q' || command[0] == 'Q')
		sendEvent(Q_MyCloseEvent);
	    else if (command[0] == 'r' || command[0] == 'R') {
		for (i = 1; i < 32 && command[i] >= '0' && command[i] <= '9'; ++i)
		    ;
		command[i] = 0;
		cpos = atof (command + 1);
		player->ReseekExact(cpos);
	    } else
		printf ("Unknown command: %s\n", command);
	    command_pos = 0;
	    complete = 0;
	}
	return;
    }
}

void PlayerControl::aboutMovie()
{
    if (!player)
	return;

    avm::string s = tr( "Stream: " ).ascii();
    s += player->GetFileName();
    s += "\n\n";

    avm::StreamInfo* info = player->GetVideoStreamInfo();
    if (info)
    {
	s += info->GetString();
	s += tr( "Video decoder: ").ascii();
	s += player->GetVideoFormat();
	s += '\n';
        delete info;
    }

    s += '\n';

    info = player->GetAudioStreamInfo();
    if (info)
    {
	s += info->GetString();
	s += tr( "Audio decoder: " ).ascii();
	s += player->GetAudioFormat();
	s += '\n';
        delete info;
    }

    printf("%s", s.c_str());
    // latin-1 ? utf-8 ?
    QMessageBox::information(0, tr( "About stream" ),
			     QString::fromLocal8Bit( s.c_str() ));
}

void PlayerControl::middle_button()
{
    int qtkey = 0, qtascii = 0;

    int w, h;
    player->Get(player->QUERY_VIDEO_WIDTH, &w,
		player->QUERY_VIDEO_HEIGHT, &h,
		0);
    int iw = player->GetWidth();
    int ih = player->GetHeight();
    switch((m_iResizeCount + 1) % 3)
    {
    case 0:
        // zoom 50%
	qtkey = Qt::Key_1;
	qtascii = '1';
	break;
    case 1:
        // zoom 100%
	qtkey = Qt::Key_2;
	qtascii = '2';
	break;
    case 2:
        // zoom 200%
	qtkey = Qt::Key_3;
	qtascii = '3';
	break;
    }

    sendKeyEvent(qtkey, qtascii);

    m_bKeyProcessed = false;
    // intentionaly setting false here !
    // this is like sync point - this will
    // reset keyboard processed status
    // just in case something wrong happened before
}

void PlayerControl::readConfig()
{
    if (player)
    {
	// FIXME
	bool ht;
	char* vcodecs = 0;
	char* acodecs = 0;

        player->Get(player->DISPLAY_FRAME_POS, &m_bDisplayFramePos,
		    player->USE_HTTP_PROXY, &ht,
		    player->VIDEO_CODECS, &vcodecs,
		    player->AUDIO_CODECS, &acodecs,
		    0);
	if (ht)
	{
	    char* proxy = 0;
	    player->Get(player->HTTP_PROXY, &proxy, 0);
	    if (proxy)
	    {
		if (strlen(proxy))
		    avm_setenv("HTTP_PROXY", proxy, 1);
                free(proxy);
	    }
	}
	else avm_unsetenv("HTTP_PROXY");

	if (vcodecs)
	{
	    avm::SortVideoCodecs(vcodecs);
            free(vcodecs);
	}
	if (acodecs)
	{
	    avm::SortAudioCodecs(acodecs);
            free(acodecs);
	}
    }
}

void PlayerControl::reseekSlot()
{
    if (!player || !m_pMW)
        return;

    double p = (m_pMW->m_pScroller->value() / (double) MAX_MBAR) * m_dLengthTime;
    p += m_dStartTime;
    m_bInReseek = true;
    player->ReseekExact(p);
    updatePos();
    m_bIsTracking = false;
    m_bInReseek = false;
}

void killer(int z)
{
    printf("quit_flag=1\n");
}

int PlayerControl::initPlayer(AviPlayerInitParams& uapip)
{
    if (&apip != &uapip)
	apip = uapip;

    last_slider = 0;
    endPlayer();

    if (m_pMW)
    {
	m_pMW->m_pScroller->setValue(0);
	m_pMW->m_pLStatus->setText( tr( "Opening..." ) );
    }

    if (apip.urls.size() == 0)
        apip.urls.push_back("");

    while (apip.urls.size())
    {
	//printf("INIT  %s %d\n", apip.urls[apip.idx].c_str(), apip.idx);
        if (apip.idx >= apip.urls.size())
	    apip.idx = 0;

	player = CreateAviPlayer2(this, m_pMW->x11Display(),
				  apip.urls[apip.idx], apip.subname, 0,
				  apip.vcodec, apip.acodec);
	if (player)
	{
	    starterSlot();
	    return 0;
	}
	//printf("URLSIZE  %d  %d\n", apip.idx, apip.urls.size());
	apip.urls.erase(&apip.urls[apip.idx]);
    }
    return -1;
}

void PlayerControl::starterSlot()
{
    if (!player->IsOpened())
    {
	if (!m_TimerStarter->isActive())
	    m_TimerStarter->start(100);
        // periodicaly check and loop until the file is opened
	return;
    }

    m_TimerStarter->stop();
    if (!player->IsValid())
    {
	// may be show a message box here?
	if (m_pMW)
	    m_pMW->m_pLStatus->setText( tr( "Failed to open" ) );
        apip.urls.erase(&apip.urls[apip.idx]);
	if (apip.urls.size())
	    initPlayer(apip); // recursive call - FIXME
	return;
    }

    //printf("OPENED2 %p  v:%d  r:%d\n", player, player->IsValid(), player->IsRedirector());
    m_Filename = apip.urls[apip.idx];
    if (player->IsRedirector())
    {
	player->GetURLs(apip.urls);
	if (!apip.urls.size())
	{
	    printf("Redirector with no entries?\n");
            if (m_pMW)
		m_pMW->m_pLStatus->setText( tr( "No redirector entries" ) );
	    return;
	}
	initPlayer(apip); // recursive call - FIXME
        return;
    }

    WS("url", m_Filename.c_str());

    if (apip.subname)
    {
	WS("subtitle-url", apip.subname);
	m_Subname = apip.subname;
    }
    readConfig();
    //if (quality == 11)
    //    player->SetDecodingMode(IVideoDecoder::REALTIME_QUALITY_AUTO);
    if (m_pMW)
	volumeChanged(m_pMW->vbar->value());
    //printf("X %d  Y %d\n", apip.x, apip.y);

    if (m_pMW)
	m_pMW->setCaption( QFile::decodeName( m_Filename.c_str() ) );

    playSlot();

    if (-8192 <= apip.x && apip.x <= 8192
	&& -8192 <= apip.y && apip.y <= 8192)
    {
	const avm::vector<avm::IVideoRenderer*>& v = player->GetVideoRenderers();
	if (v.size())
	    v.front()->SetPosition(apip.x, apip.y);
    }

    //printf("STARTTIME %f\n", m_dStartTime);
    if (apip.seek > 0.0)
	player->ReseekExact(apip.seek);

    if (m_pTimer)
    {
	updatePos();
	m_pTimer->start(1000); // updatepos timer
    }
    if (apip.width && apip.height)
	resizePlayer(apip.width, apip.height);

    // wait here - so all graphics is initialized & opened
    if (apip.maximize)
    {
        apip.maximize = !apip.maximize;
	sendMaximize();
    }
    else if (apip.fullscreen)
    {
        apip.fullscreen = !apip.fullscreen;
	sendFullscreen();
    }
}

void PlayerControl::endPlayer()
{
    if (m_pTimer)
	m_pTimer->stop();
    delete popup;
    popup = 0;
    delete m_pAboutPopup;
    m_pAboutPopup = 0;
    delete m_pOpenPopup;
    m_pOpenPopup = 0;

    if (player)
    {
	const avm::vector<avm::IVideoRenderer*>& v = player->GetVideoRenderers();
	//printf("SETPOSITION %d %d\n", apip.x, apip.y);
	if (v.size())
	    v.front()->GetPosition(apip.x, apip.y);
    }
    delete player;
    player = 0;
}

void PlayerControl::updatePosDisplay(double timepos)
{
    if (!player)
        return;

    double percent = 0;
    avm::IAviPlayer::State state = player->GetState(&percent);

    if (state == avm::IAviPlayer::Playing
	|| state == avm::IAviPlayer::Paused
	|| state == avm::IAviPlayer::Buffering)
    {
	char timing[50];
	double len = player->GetLengthTime();
	double stream_time = player->GetTime();
	//printf("POS %f  %f\n", timepos, stream_time);
	if (timepos > stream_time + 2 || timepos < stream_time - 2)
	    //user is seeking - show the time for seek-bar
	    stream_time = timepos;

	char* spos = timing;

	if (stream_time >= 3600)
	    spos += sprintf(spos, "%u:", uint_t(stream_time/3600));

	spos += sprintf(spos, "%02u:%02u", uint_t(stream_time/60)%60,
			uint_t(stream_time)%60);

	if (m_bDisplayFramePos)
            // more precise timeing with frame position
	    spos += sprintf(spos, ".%03u", uint_t(stream_time*1000)%1000);

	if (len >= 0x7fffffff)
	    spos += sprintf(spos, "/Live");
	else
	{
	    if (len >= 3600)
		spos += sprintf(spos, "/%u:", uint_t(len/3600));
	    else
		spos += sprintf(spos, "/");

	    spos += sprintf(spos, "%02u:%02u", uint_t(len/60)%60, uint_t(len)%60);
	}

	if (player->GetVideoLengthTime() > 0)
	{
            if (m_bDisplayFramePos)
		spos += sprintf(spos, " (%u)", player->GetFramePos());

	    if (m_pMW && m_pMW->pbar)
	    {
		int prog;
		player->Get(player->QUERY_AVG_QUALITY, &prog, 0);
#if QT_VERSION <= 220
		m_pMW->pbar->reset();
#endif
		m_pMW->pbar->setProgress(prog);
	    }
	}
	else
	{
	    if (m_pMW && m_pMW->pbar)
	    {
#if QT_VERSION <= 220
		m_pMW->pbar->reset();
#endif
		m_pMW->pbar->setProgress((int)(percent * 100));
	    }

	}

	if (percent < 0.01)
	{
	    player->Pause(true);
	    m_bUnpause = true;
	}
	else if (m_bUnpause && percent > 0.21)
	{
	    player->Pause(false);
	    m_bUnpause = false;
	}
        if (m_pMW)
	    m_pMW->m_pLTime->setText(timing);
    }
}

void PlayerControl::movedSlot(int value)
{
    double t = value * m_dLengthTime / MAX_MBAR + m_dStartTime;
    updatePosDisplay((int)(t));
}

void PlayerControl::updatePos()
{
    if (!player || !m_pTimer)
	return;

    double dpos = player->GetTime();

    if (m_pMW)
    {
	int v;
	player->Get(player->AUDIO_VOLUME, &v, 0);
	m_pMW->vbar->setValue(v * MAX_VBAR / 1000);
    }

    if (player->IsPlaying())
    {
        bool b;
	player->Get(player->QUERY_EOF, &b, 0);
        if (b)
	{
	    player->Get(player->AUTOREPEAT, &b, 0);
	    //printf("\n\n\nAUTOREP %d  %d   %d\n", b, apip.urls.size(), apip.idx);
	    if (b)
	    {
		if (apip.urls.size() > 1)
		{
		    apip.idx++;
		    initPlayer(apip);
                    return;
		}
                else
		    player->ReseekExact(0.0);
	    }
	    else
	    {
		if (apip.urls.size() > 1)
		{
		    apip.idx++;
		    if (apip.idx < apip.urls.size())
		    {
			initPlayer(apip);
                        return;
		    }
		}
		sendEvent(Q_MyCloseEvent);
	    }
	}
    }

    if (!m_bIsTracking && m_pMW)
    {
        m_bSettingScroller = true;
	m_dLengthTime = player->GetLengthTime();
	if (m_dLengthTime >= 0x7fffffff)
	    m_dLengthTime = 1;
	else
	    m_dLengthTime -= m_dStartTime;

	// Scroll bar
	m_pMW->m_pScroller->setValue((int) (MAX_MBAR * (dpos - m_dStartTime) / m_dLengthTime));
	m_bSettingScroller = false;
    }

    int w = 400;
    if (m_pMW) {
	double percent;
	char statustxt[100];
	avm::IAviPlayer::State state = player->GetState(&percent);

#ifndef HAVE_LIBSDL
	w = m_pMW->width();
#endif
	switch(state)
	{
	case avm::IAviPlayer::Invalid:
	    strcpy(statustxt, tr( "Invalid stream" ));
	    break;
	case avm::IAviPlayer::Opening:
	    strcpy(statustxt, tr( "Opening..." ));
	    break;
	case avm::IAviPlayer::Buffering:
	    sprintf(statustxt, tr( "Buffering: %d%%" ), int(100*percent));
	    break;
	case avm::IAviPlayer::Paused:
	    strcpy(statustxt, tr( "Paused" ));
	    break;
	case avm::IAviPlayer::Stopped:
	    strcpy(statustxt, tr( "Stopped" ));
	    break;
	case avm::IAviPlayer::Playing:
	    {
		int drop;
		player->Get(player->QUERY_AVG_DROP, &drop, 0);
		if (drop > 0)
		    sprintf(statustxt, tr( "Drop: %d%%" ), drop);
		else
		    strcpy(statustxt, tr( "Playing..." ));
	    }
	    break;
	}

	m_pMW->m_pLStatus->setText(statustxt);
    }

    if(w > 90 && m_bIsTracking != 1)
    	updatePosDisplay((int)dpos);
}

void PlayerControl::help()
{
    QMessageBox::information( 0, tr( "AVI play usage" ),
                              tr(
                                  "<h3>Usage:</h3>"
                                  "aviplay [options] [your-file(s)]<br>"
                                  "<h3>Key bindings:</h3>"
                                  "Cursor left/right - previous/next keyframe.<br>"
                                  "Cursor up/down - 60 seconds backward/forward.<br>"
                                  "Enter/Backspace - display next/previous(slow!) frame.<br>"
                                  "Home/End - movie begin/end.<br>"
                                  "Insert/Delete - previous/next file. (if more files given)<br>"
                                  "Keypad '+'/'-' - volume up/down.<br>"
                                  "PageUp/PageDown - 10 seconds backward/forward.<br>"
                                  "F, Escape or Alt + Enter - toggles fullscreen/windowed mode.<br>"
                                  "M - toggles fullscreen/windowed mode with maximization.<br>"
                                  "N - swith to next audio stream (if available).<br>"
                                  "Q - quit<br>"
                                  "X - stop<br>"
                                  "V - play<br>"
                                  "[ / ] - adjust a-v sync by 0.1 second.<br>"
                                  "C, P or Space - pause<br>"
                                  "1,2,3 - switch zoom 0.5x, 1x, 2x.<br>"
                                  "Left-click in player window - pause/play.<br>"
                                  "Right-click in movie window brings up zoom/fullscreen menu.<br>"
                                  "F1/F2, F5/F6, F7/F8, F9/F10 - set Saturation, Brightness, Hue, Contrast.<br>"
				  //"Double-click in player window when paused/stopped "
                                  //"to bring up configuration menu.<br>"
                                  ),
                              QMessageBox::Ok );
}

void PlayerControl::about()
{
    QMessageBox::information( 0, tr( "About AVIPlay" ),
                              tr(
				  "AVI player version: %1<br>"
                                  "(C) 2000 - 2003 Zdenek Kabelac, Eugene Kuznetsov <br>"
                                  "Based on avifile ( <a href=\"http://avifile.sf.net\">http://avifile.sourceforge.net/</a>).<br>"
                                  "Distributed under the GNU Public Licence (GPL) version 2.<br>"
                                  "Special thanks to: "
                                  "<ul>"
                                  "<li>Authors of Wine project for Win32 DLL loading code."
                                  "<li>Avery Lee for AVI and ASF parser and his VirtualDub."
                                  "<li>Hiroshi Yamashita for porting sources to FreeBSD."
                                  "<li>Jürgen Keil for porting sources to Solaris 8 x86."
                                  "<li>Árpád Gereöffy (A'rpi/ESP-team) and the rest of Mplayer team."
                                  "<li>All people from avifile mailing lists for their patience and support."
                                  "</ul>" ).arg( AVIFILE_BUILD ),
                              QMessageBox::Ok );
}

void PlayerControl::dropEvent(QDropEvent* e)
{
    QStrList i;
    QUriDrag::decode(e, i);
    const char* uri=i.first();

    if (!uri || strlen(uri) < 6)
	return;

    QString file;

    if (0 == qstrnicmp(uri,"file:/",6) )
    {
	uri += 6;
	if ( uri[0] != '/' || uri[1] == '/' )
	{
	    // It is local.
	    file = QUriDrag::uriToUnicodeUri(uri);
	    if ( (uri[1] == '/') && (uri[0]=='/') )
		file.remove(0,1);
	    else
		file.insert(0,'/');

	    apip.urls.push_back((const char*)file.ascii());
	    initPlayer(apip);
	}
    }
}

void PlayerControl::dragEnterEvent(QDragEnterEvent* e)
{
    e->accept(QUriDrag::canDecode(e));
}
