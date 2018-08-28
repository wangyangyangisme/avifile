#ifndef QTRECOMPRESSOR_H
#define QTRECOMPRESSOR_H

#include <qmainwindow.h>
class QButtonGroup;
class QComboBox;
class QLabel;
class QLineEdit;
class QListBox;
class QPushButton;
class QRadioButton;

class QtRecompressor : public QMainWindow
{ 
    Q_OBJECT;

public:
    QtRecompressor( QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );
    ~QtRecompressor();

    QButtonGroup* m_pButtonGroupLimits;
    QButtonGroup* m_pButtonGroupStreamMode;
    QComboBox* m_pComboStreams;
    QLabel* m_pLabelConfig;
    QLabel* m_pLabelDst;
    QLabel* m_pLabelSrc;
    QLineEdit* m_pLineHighFrames;
    QLineEdit* m_pLineHighSec;
    QLineEdit* m_pLineLowFrames;
    QLineEdit* m_pLineLowSec;
    QListBox* m_pListAllFilters;
    QListBox* _lbFilters;
    QPushButton* m_pButtonSrc;
    QPushButton* m_pButtonDst;
    QPushButton* m_pButtonCfg;
    QPushButton* m_pButtonAboutFilter;
    QPushButton* m_pButtonAddFilter;
    QPushButton* m_pButtonConfigFilter;
    QPushButton* m_pButtonDownFilter;
    QPushButton* m_pButtonFormat;
    QPushButton* m_pButtonLoadCfg;
    QPushButton* m_pButtonOpen;
    QPushButton* m_pButtonRemoveFilter;
    QPushButton* m_pButtonSaveAs;
    QPushButton* m_pButtonSaveCfg;
    QPushButton* m_pButtonStart;
    QPushButton* m_pButtonStreamDetails;
    QPushButton* m_pButtonUpFilter;
    QRadioButton* m_pRadioRemove;
    QRadioButton* m_pRadioCopy;
    QRadioButton* m_pRadioRecompress;
    QRadioButton* m_pRadioSeconds;
    QRadioButton* m_pRadioFrames;

public slots:
    virtual void fileOpenVideo() = 0;
    virtual void fileAppendVideo() = 0;
    virtual void fileSaveAvi() = 0;
    virtual void fileLoadConfig() = 0;
    virtual void fileSaveConfig() = 0;
    virtual void fileQuit() = 0;

    virtual void videoFilters() = 0;

    virtual void optionsPreferences() = 0;

    virtual void helpContents() = 0;
    virtual void helpChangelog() = 0;

    virtual void startRecompress() = 0;
    virtual void aboutFilterClicked() = 0;
    virtual void addFilterClicked() = 0;
    virtual void configFilterClicked() = 0;
    virtual void detailsClicked() = 0;
    virtual void downFilterClicked() = 0;
    virtual void removeFilterClicked() = 0;
    virtual void streamFormatClicked() = 0;
    virtual void streamLimitModeChanged(int) = 0;
    virtual void streamModeChanged(int) = 0;
    virtual void streamSelected(int) = 0;
    virtual void upFilterSelected() = 0;
};

#endif // QTRECOMPRESSOR_H
