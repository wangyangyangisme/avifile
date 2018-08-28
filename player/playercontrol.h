#ifndef PLAYERCONTROL_H
#define PLAYERCONTROL_H

#include <qwidget.h>
#include <qpixmap.h>

#include "aviplay.h"
#include "playerwidget.h"
#include "renderer.h"
#include "avm_locker.h"

#ifdef USE_MPATROL
#include <mpatrol.h>
#endif

class ConfigDialog;
class QLabel;
class QMessageBox;
class QPixmap;
class QPopupMenu;
class QProgressBar;
class QPushButton;
class QSlider;
class QTimer;

#define Q_MenuEvent QEvent::User
#define Q_MyCloseEvent QEvent::Type(QEvent::User+2)
#define Q_MyResizeEvent QEvent::Type(QEvent::User+3)
#define Q_MyRefreshEvent QEvent::Type(QEvent::User+4)
#define Q_MyPosEvent QEvent::Type(QEvent::User+5)
#define Q_MyOpenEvent QEvent::Type(QEvent::User+6)
#define Q_MyAboutEvent QEvent::Type(QEvent::User+7)
#define Q_MyConfigEvent QEvent::Type(QEvent::User+8)

struct AviPlayerInitParams
{
    avm::string acodec;
    avm::string vcodec;
    avm::string subname;
    avm::vector<avm::string> urls;
    double seek;
    unsigned idx;
    int x;
    int y;
    int width;
    int height;
    int quality;
    bool fullscreen;
    bool maximize;
    bool nogui;
    bool quiet;
    bool autoq;

    AviPlayerInitParams()
	: seek(0.), idx(0), x(0), y(0), width(0), height(0), quality(0), fullscreen(false),
	maximize(false), nogui(false), quiet(false), autoq(false) {}
};

class QMyResizeEvent : public QEvent
{
public:
    int m_x, m_y;
    QMyResizeEvent(int x, int y)
	:QEvent(Q_MyResizeEvent), m_x(x), m_y(y) {}
};

class PlayerControl;
class QButton;

class QPlayerWidget : public QWidget
{
    PlayerControl* m_pControl;
public:
    QPlayerWidget(PlayerControl* c);
    virtual bool event(QEvent* e);

    QSlider* m_pScroller;
    QSlider* vbar;
    QProgressBar* pbar;
    QLabel* m_pLStatus;
    QLabel* m_pLTime;
    QButton* b_about;
    QButton* b_mute;
    QButton* b_open;
    QButton* b_pause;
    QButton* b_play;
    QButton* b_stop;
};

class PlayerControl : public QObject, public PlayerWidget
{
    friend class QPlayerWidget;

    Q_OBJECT;

    enum {
	RATIO_ORIG,
	RATIO_16_9,
	RATIO_4_3,
	ZOOM_PLUS,
	ZOOM_MINUS,
	CROP_TOP,
	CROP_BOTTOM,
	CROP_LEFT,
	CROP_RIGHT,
	CROP_16_9,
	CROP_4_3,
    };

public:
    PlayerControl();
    ~PlayerControl();
    int initPlayer(AviPlayerInitParams& avip);
    bool isValid() { return (player) ? true : false; }
    bool isStopped() { return (player) ? player->IsStopped() : true; }
    bool isPlaying() { return (player) ? player->IsPlaying() : false; }
    bool isPaused() { return (player) ? player->IsPaused() : false; }
    void decoderConfig(int);
    void endPlayer();
    void ratio(int);
    void setDisplayFramePos(bool b) { m_bDisplayFramePos = b; }
    void sendKeyEvent(int qtkey, int qtascii = 0, int qtstate = 0);
    void sendRefresh();
    void updatePosDisplay(double timepos);
    void zoom(double scale);
    void zoom_mode(int);
    void setRatio(int& x, int& y, int& w, int& h, int rw, int rh)
    {
	if (rh * w >= rw * h)
	{
	    int n = rw * h / rh; x = (w - n) / 2;
	    if (x < 8) { x = 0; n = w; }
	    w = n;
	}
	else
	{
	    int n = rh * w / rw; y = (h - n) / 2;
	    if (y < 8) { y = 0; n = h; }
	    h = n;
	}
    }
    void zoomWH(int width, int height, int original_w, int original_h);
    double getTime() { return (player) ? player->GetTime() : 0.; }
    avm::IAviPlayer2* getPlayer() { return player; }
    QWidget* getWidget() { return m_pMW; }

    // PlayerWidget
    virtual void PW_showconf_func() {return showconfSlot();}
    virtual void PW_stop_func() { return stopSlot(); }
    virtual void PW_middle_button() {return middle_button();}
    virtual void PW_pause_func() { return pauseSlot(); }
    virtual void PW_play_func() { return playSlot(); }
    virtual void PW_quit_func() { return sendEvent(Q_MyCloseEvent); }
    virtual void PW_menu_slot() { return sendEvent(Q_MenuEvent); }
    virtual void PW_fullscreen() { return sendFullscreen(); }
    virtual void PW_key_func(int sym, int mod) { return keySlot(sym, mod); }
    virtual void PW_resize(int w, int h) { return sendQt(new QMyResizeEvent(w, h)); }
    virtual void PW_refresh() { return sendEvent(Q_MyRefreshEvent); }
    virtual void PW_maximize_func() { return sendMaximize(); }

public slots:
    void aboutSlot();
    void changedSlot(int);
    void fullscreen();
    void keySlot(int sym, int mod);
    void maximize();
    void maximizeMode(int);
    void menuSlot();
    void middle_button();
    void movedSlot(int);
    void muteChanged();
    void nextPage();
    void openSlot();
    void openaudSlot();
    void openpopupSlot();
    void opensubSlot();
    void pauseSlot();
    void playSlot();
    void prevPage();
    void ratio_16_9() { ratio(RATIO_16_9); }
    void ratio_4_3() { ratio(RATIO_4_3); }
    void ratio_orig() { ratio(RATIO_ORIG); }
    void refresh();
    void reseekSlot();
    void resizePlayer(int, int);
    void sendConfig() { sendEvent(Q_MyConfigEvent); }
    void sendEvent(QEvent ev) { sendQt(new QEvent(ev)); }
    void sendFullscreen() { sendKeyEvent(Qt::Key_F, 'F'); }
    void sendMaximize() { sendKeyEvent(Qt::Key_M, 'M'); }
    void sendQt(QEvent* ev);
    void showconfSlot();
    void sliderSlot() { m_bIsTracking = 1; }
    void stopSlot();
    void updatePos();
    void volumeChanged(int);
    void zoom_0_5x() { zoom(0.5); }
    void zoom_1x() { zoom(1.0); }
    void zoom_2x() { zoom(2.0); }
    void zoom_plus() { zoom_mode(ZOOM_PLUS); }
    void zoom_minus() { zoom_mode(ZOOM_MINUS); }
    void crop_top() { zoom_mode(CROP_TOP); }
    void crop_bottom() { zoom_mode(CROP_BOTTOM); }
    void crop_left() { zoom_mode(CROP_LEFT); }
    void crop_right() { zoom_mode(CROP_RIGHT); }
    void crop_16_9() { zoom_mode(CROP_16_9); }
    void crop_4_3() { zoom_mode(CROP_4_3); }

    void remote_command();

    void help();
    void about();
    void aboutMovie();
    void audioDecoderConfig() { decoderConfig(player->AUDIO_CODECS); }
    void videoDecoderConfig() { decoderConfig(player->VIDEO_CODECS); }
    void rendererConfig();
    void dropEvent(QDropEvent*);
    void dragEnterEvent(QDragEnterEvent* event);
    void starterSlot();

    void keyPressEvent(QKeyEvent *);
    void mouseDoubleClickEvent(QMouseEvent*);
    void readConfig();

protected:

    // GUI
    QPlayerWidget* m_pMW;
    QPopupMenu* popup;
    QPopupMenu* modes;
    QPopupMenu* m_pAboutPopup;
    QPopupMenu* m_pOpenPopup;
    avm::string m_Theme;

    // player
    avm::IAviPlayer2* player;
    avm::PthreadMutex m_Mutex;
    avm::PthreadMutex m_Dialog;
    ConfigDialog* dlg;
    QTimer* m_pTimer;
    QTimer* m_TimerStarter;
    avm::string m_Filename;
    avm::string m_Subname;
    double m_dStartTime;
    double m_dLengthTime;
    int last_slider;
    int m_iLastVolume;
    int m_iResizeCount;
    // pick better name here
    AviPlayerInitParams apip;

    bool m_bSettingScroller;
    bool m_bKeyProcessed;
    bool m_bDisplayFramePos;
    bool m_bIsTracking;
    bool m_bInReseek;
    //bool m_bOpenFileDlg;
    bool m_bUnpause;
};

#endif // PLAYERCONTROL_H
