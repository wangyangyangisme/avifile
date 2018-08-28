#ifndef V4LWINDOW_H
#define V4LWINDOW_H

#include <avm_stl.h>
#include <avm_locker.h>

#include <qwidget.h>
#include <qtimer.h>

AVM_BEGIN_NAMESPACE;
class AvicapRenderer;
AVM_END_NAMESPACE;

class v4lxif;
class AviCapDialog;
class ClosedCaption;
class PicturePropDialog;
class ShmRenderer;
class TimerTable;
class EpgWindow;

struct CaptureConfig;

class QPopupMenu;
class QLabel;


#define Q_MyCloseEvent QEvent::Type(QEvent::User+2)

class V4LWindow: public QWidget
{
    Q_OBJECT;

friend class AviCapDialog;
public:
    enum Modes { Overlay, Preview, DeinterlacedPreview,AvicapRend,MultiChannel };

    V4LWindow(v4lxif * pDev, QWidget * pParent=0);
    ~V4LWindow();
    void setupDevice();
    void setAutoRecord() { m_bAutoRecord = true; }
    void unregisterCapDialog() { m_pCapDialog = 0; }
    void scanXawtv();
    void get_clips();
    ClosedCaption* getcc() { return m_pCC; }
    Modes getMode() const { return m_eMode; }
    void setMode(Modes m);
    AviCapDialog *getCaptureDialog() { return m_pCapDialog; }
    void captureAVI(CaptureConfig *capconf);
    void startTimer();
    void lock(bool lock);
    void avicap_renderer_popup();
    void avicap_renderer_close();
    void avicap_renderer_stop();
    void avicap_renderer_keypress(int sym,int mod);
    void makePopup();
    void keyPress(int k,int key);
    virtual bool event(QEvent* e);

    void multichannel_increase_tiles(bool inc);
    void multichannel_increase_speed(bool inc);

public slots:
    void captureAVI();
    void captureBMP();
    void captureJPG();
    void config();
    void deinterlaced_preview();
    void force4x3();
    void hidePopup();
    void overlay();
    void picprop();
    void timertable();
    void epgwindow();
    void fullscreen();
    void maximize();
    void avicap_renderer();
    void avicap_renderer_display();
    void multichannel();
    void multichannel_display();
    void multichannel_stop();
    void preview();
    void preview_display();
    void setXawtv(int);
    void changeChannel(int);
    void showsub();
    void timerStop();
    void updatesub();
    void ratioTimerStop();

protected: // x11 stuff for handling overlay clipping
    void postResizeEvent(int w, int h);
    void sendQt(QEvent *e);
    void setupSubtitles();
    void setupPicDimensions(int* =0, int* =0);
    void getOverlaySizeAndOffset(int *pw, int *ph, int* pdx=0, int* pdy=0);

    void avicap_renderer_resize_input(int w,int h);

    virtual bool x11Event( XEvent * );
    virtual void focusInEvent( QFocusEvent * );
    virtual void focusOutEvent( QFocusEvent * );
    virtual void hideEvent( QHideEvent * );
    virtual void keyPressEvent( QKeyEvent * );
    virtual void mouseReleaseEvent( QMouseEvent * );
    virtual void moveEvent( QMoveEvent * );
    virtual void paintEvent( QPaintEvent * );
    virtual void resizeEvent( QResizeEvent * );
    virtual void showEvent( QShowEvent * );
    void add_clip(int x1, int y1, int x2, int y2);
    void refresh_timer();

    v4lxif* m_pDev;
    ShmRenderer* m_pRenderer;

    int wmap;
    int visibility;

    int conf;
    int oc_count;
    struct OVERLAY_CLIP {
	int x1, y1, x2, y2;
    } oc[64];

    Modes m_eMode;
private:
    int m_iPainter;
    avm::PthreadMutex m_Mutex;
    PicturePropDialog* m_pPicConfig;
    TimerTable *m_pTimerTable;
    EpgWindow *m_pEpgWindow;
    AviCapDialog* m_pCapDialog;
    ClosedCaption* m_pCC;
    QPopupMenu* m_pPopup;
    QLabel* m_pLabel;
    QTimer m_timer;
    QTimer m_cctimer;
    QTimer m_CaptureTimer;
    QTimer m_avicapTimer;
    QTimer m_multichannelTimer;
    QTimer m_RatioTimer;
    struct xawtvstation {
	avm::string sname;
	avm::string input;
        avm::string channel;
        int finetune;
	int key;
    };
    avm::string freqtable;
    avm::vector<xawtvstation> xawtvstations;
    bool m_bForce4x3;
    bool m_bSubtitles;
    bool known_pos;
    bool m_bAutoRecord;
    bool m_bMoved;
    int  last_station;

    //avicap/sdl/yuv/fs-renderer
    avm::AvicapRenderer *fs_renderer;
    int fsr_width,fsr_height;

    //multichannel
    int mc_oldw,mc_oldh,mc_curw,mc_curh,mc_grabw,mc_grabh;
    int mc_startchannel,mc_curchannel,mc_count;
    int mc_switch,mc_tiles;

public:
    avm::string getXawtvStation(int i) { return xawtvstations[i].sname; }
    int getStations() { return xawtvstations.size(); }
};

#endif // V4LWINDOW_H
