#ifndef SOURCECONTROL_H
#define SOURCECONTROL_H

#include <qdialog.h>

class QLabel;
class QSlider;
class QToolButton;

class SourceControlDialog : public QDialog
{ 
    Q_OBJECT;

public:
    QSlider* m_pSlider;
    QLabel* m_pLabel;

    QToolButton* m_pButtonPF;
    QToolButton* m_pButtonPFF;
    QToolButton* m_pButtonPTF;
    QToolButton* m_pButtonPKF;

    QToolButton* m_pButtonNF;
    QToolButton* m_pButtonNFF;
    QToolButton* m_pButtonNTF;
    QToolButton* m_pButtonNKF;

    SourceControlDialog( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
    ~SourceControlDialog();

public slots:
    virtual void prevFrame() = 0;
    virtual void prevFiveFrames() = 0;
    virtual void prevTwentyFrames() = 0;
    virtual void prevKeyFrame() = 0;

    virtual void nextFrame() = 0;
    virtual void nextFiveFrames() = 0;
    virtual void nextTwentyFrames() = 0;
    virtual void nextKeyFrame() = 0;

    virtual void valueChanged() = 0;
    virtual void sliderReleased(int) = 0;
    virtual void sliderMoved(int) = 0;

};

#endif // SOURCECONTROL_H
