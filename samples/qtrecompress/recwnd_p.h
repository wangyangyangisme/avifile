/****************************************************************************
 ** Form interface generated from reading ui file './recwnd.ui'
 **
 ** Created: Sun Sep 24 03:34:28 2000
 **      by:  The User Interface Compiler (uic)
 **
 ** WARNING! All changes made in this file will be lost!
 ****************************************************************************/
#ifndef RECWND_P_H
#define RECWND_P_H

#include <qdialog.h>
class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QLabel;
class QTabWidget;
class QProgressBar;
class QPushButton;

class RecWnd_p : public QDialog
{
    Q_OBJECT

public:
    RecWnd_p( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
    ~RecWnd_p();

    enum
    {
	CURRENT_VIDEO_FRAME, CURRENT_AUDIO_SAMPLE, VIDEO_DATA,
	AUDIO_DATA, CURRENT_FILE_SIZE, PROJECTED_FILE_SIZE,
	VIDEO_RENDERING_RATE, TIME_ELAPSED, TOTAL_TIME_ESTIMATED,
	LAST_LABEL
    };
    QTabWidget* m_pTabWidget;
    QPushButton* m_pPause;
    QPushButton* m_pStop;
    QProgressBar* m_pProgress;
    QLabel* m_pText[LAST_LABEL];

    public slots:
	virtual void cancelProcess();
	virtual void pauseProcess();
	virtual void currentChanged(QWidget*);
};

#endif // RECWND_P_H
