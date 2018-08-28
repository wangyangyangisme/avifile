#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include "okdialog.h"

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QListBox;
class QPushButton;
class QSlider;
class QSpinBox;

class ConfigDialog : public QavmOkDialog
{
    Q_OBJECT;

public:
    static const unsigned int frequencyList[];
    ConfigDialog( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );

    //QSpinBox* m_pAudioResamplingRate;
    QComboBox* m_pAudioResamplingRate;
    QComboBox* m_pAudioPlayingRate;
    QCheckBox* m_pAudioResampling;
    QCheckBox* m_pAutorepeat;
    QCheckBox* m_pHwaccel;
    QCheckBox* m_pPreserveAspect;
    QCheckBox* m_pQualityAuto;
    QCheckBox* m_pDisplayFramePos;
    QCheckBox* m_pVideoBuffered;
    QCheckBox* m_pVideoDirect;
    QCheckBox* m_pVideoDropping;
    QCheckBox* m_pUseProxy;
    QCheckBox* m_pNoDefaultTheme;
    QLabel* m_pAsync;
    QLabel* m_pFontType;
    QLabel* m_pPreview;
    QLineEdit* m_pFontName;
    QLineEdit* m_pProxyName;
    QPushButton* m_pFontButton;

    QPushButton* m_pSubFGColor;
    QPushButton* m_pSubBGColor;
    QCheckBox* m_pSubWrap;
    QCheckBox* m_pSubEnabled;

    QSlider* m_pAsyncSlider;
    QSpinBox* m_pDefAudio;
    QSpinBox* m_pSubAsyncMin;
    QSpinBox* m_pSubAsyncSec;
    QCheckBox* m_pSubNegative;
    QListBox* m_pVideoList;
    QListBox* m_pAudioList;
    QComboBox* m_pThemeList;

protected:
    bool event( QEvent* );
    QWidget* createAudio(QWidget* parent);
    QWidget* createMisc(QWidget* parent);
    QWidget* createSubtitles(QWidget* parent);
    QWidget* createSync(QWidget* parent);
    QWidget* createVideo(QWidget* parent);
    QWidget* createDecoder(QWidget* parent);
};

#endif // CONFIGDIALOG_H
