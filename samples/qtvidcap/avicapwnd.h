#ifndef AVICAPDIALOG_H
#define AVICAPDIALOG_H

#include "avicapwnd_p.h"

class V4LWindow;
class QTimer;
class CaptureProcess;
class CaptureProgress;
class ClosedCaption;

struct CaptureConfig;

class AviCapDialog : public AviCapDlg
{ 
    Q_OBJECT;

    V4LWindow* m_pWnd;
    QTimer* m_pTimer;
    CaptureProcess* m_pCapproc;
    CaptureProgress* m_pProgress;
    CaptureConfig *my_config;
    ClosedCaption* m_pCC;
    int m_eOldMode;
    bool m_bStarted;
    bool i_am_mini;
    bool use_dirpool;
    int last_dropped_enc,last_dropped_cap,last_synchfix;
    long last_synchfix_time;
    
public:
    AviCapDialog(V4LWindow* pWnd,CaptureConfig *conf);
    // QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );
    ~AviCapDialog();
    CaptureProcess *getCaptureProcess() { return m_pCapproc; };
public slots:
    void start();
    void mini(); 
    void segment();
    void stop();    
    void updateGraphs();
    bool close();
};

#endif // AVICAPDIALOG_H
