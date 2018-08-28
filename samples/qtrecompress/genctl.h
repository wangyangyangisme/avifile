#ifndef QTRECOMPRESSORCTL_H
#define QTRECOMPRESSORCTL_H

#include "genctl_p.h"
#include "recompressor.h"
#include "qimagecontrol.h"
#include "srcctl.h"

class QtRecompressorCtl : public QtRecompressor
{
    Q_OBJECT;

public:
    QtRecompressorCtl( QWidget* parent = 0 );
    ~QtRecompressorCtl();

public slots:
    virtual void fileOpenVideo();
    virtual void fileAppendVideo();
    virtual void fileSaveAvi();
    virtual void fileLoadConfig();
    virtual void fileSaveConfig();
    virtual void fileQuit();

    virtual void videoFilters();

    virtual void optionsPreferences();

    virtual void helpContents();
    virtual void helpChangelog();

    virtual void startRecompress();
    virtual void aboutFilterClicked();
    virtual void addFilterClicked();
    virtual void configFilterClicked();
    virtual void detailsClicked();
    virtual void downFilterClicked();
    virtual void removeFilterClicked();
    virtual void streamFormatClicked();
    virtual void streamLimitModeChanged(int);
    virtual void streamModeChanged(int);
    virtual void streamSelected(int);
    virtual void upFilterSelected();

    void lo_fr_changed(const QString&);
    void hi_fr_changed(const QString&);
    void lo_sec_changed(const QString&);
    void hi_sec_changed(const QString&);
protected:
    void updateFilters();
    RecKernel* kernel;
    int no_recurs;
    SourceControl* wnd;
    QImageControl* ictl;
};

#endif // QTRECOMPRESSORCTL_H
