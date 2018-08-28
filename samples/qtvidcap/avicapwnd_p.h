/****************************************************************************
** Form interface generated from reading ui file 'capture.ui'
**
** Created: Sat May 19 20:42:24 2001
**      by:  The User Interface Compiler (uic)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/
#ifndef AVICAPDLG_H
#define AVICAPDLG_H

#include <qwidget.h>

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QFrame;
class QLabel;
class QPushButton;
class QTabWidget;

class AviCapDlg : public QWidget
{
    Q_OBJECT;

public:
    AviCapDlg( QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );
    ~AviCapDlg();

    QTabWidget* CaptureTab;
    QFrame* Frame34;
    QLabel* ProgressText;
    QLabel* FrameSizeText;
    QFrame* Frame3_2;
    QLabel* TextLabel3_3;
    QLabel* MaxFrameSizeText;
    QLabel* TextLabel3;
    QFrame* Frame3;
    QLabel* TextLabel3_2;
    QLabel* CaptureBufferText;
    QPushButton* StartButton;
    QPushButton* SegmentButton;
    QPushButton* CloseButton;
    QPushButton* MiniButton;
    QWidget* bigwidget;
    QWidget* miniwidget;
    QLabel* minilabel;
    QPushButton *m_pMiniButton;
};

#endif // AVICAPDLG_H
