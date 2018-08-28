#ifndef SOURCECONTROLWND_H
#define SOURCECONTROLWND_H

#include "srcctl_p.h"
#include "recompressor.h"

class SourceControl : public SourceControlDialog
{
    Q_OBJECT;

public:
    SourceControl( QWidget* parent, RecKernel* kern );
//    SourceControlWnd( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
    ~SourceControl();
    void updateSize();

public slots:
    virtual void prevFrame();
    virtual void prevFiveFrames();
    virtual void prevTwentyFrames();
    virtual void prevKeyFrame();

    virtual void nextFrame();
    virtual void nextFiveFrames();
    virtual void nextTwentyFrames();
    virtual void nextKeyFrame();

    void valueChanged();
    void sliderMoved( int );
    void sliderReleased( int );

protected:
    bool close( bool ) { return false; }
    void updateLabel();

    RecKernel* m_pKernel;
    int scale;
    int btn_press;
};

#endif // SOURCECONTROLWND_H
