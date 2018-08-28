#ifndef RECWINDOW_H
#define RECWINDOW_H

#include "recwnd_p.h"
#include "rec_cb.h"

class RecKernel;

class Progress
{
    unsigned int* m_pValues;
    int* m_pValuesFlags;
    unsigned int m_uiPos;
    unsigned int m_uiSize;
    unsigned int m_uiMax;
    bool m_bValidMax;
public:
    Progress(unsigned int size);
    ~Progress();
    void insert(unsigned int v, int flags);
    unsigned int getMax();
    unsigned int get(int idx) { return m_pValues[idx]; }
    int getFlags(int idx)  { return m_pValuesFlags[idx]; }
};

class RecWindow : public RecWnd_p, public IRecompressCallback
{ 
    Q_OBJECT
    RecKernel* kernel;
    double progress, elapsed, estimated;
    int64_t fsize;

    static const int PROGRESS_SIZE = 200;
    Progress m_Progress;

    framepos_t totalVideoFrames;
    unsigned int totalAudioSamples;
    framepos_t curVideoFrame;
    framepos_t curAudioSample;
    double startVideoTime;
    double startAudioTime;
    double curVideoTime;
    double curAudioTime;
    int64_t videoSize;
    int64_t audioSize;
    int64_t lasttime;

public:
    RecWindow( QWidget* parent, RecKernel* kern);
    ~RecWindow();

    virtual void setNewState(double progress, double elapsed, int64_t fsize);
    virtual void setTotal(framepos_t vtotal, framepos_t atotal,
			  double videoTime, double audioTime);
    virtual void addVideo(framepos_t vframe, unsigned int vsize, double videoTime, bool keyframe);
    virtual void addAudio(framepos_t asample, unsigned int asize, double atime);
    virtual void finished();

    bool update();

public slots:
    virtual void cancelProcess();
    virtual void pauseProcess();
    virtual void currentChanged(QWidget*);
protected:
    bool event(QEvent*);
    bool updateGraphs();
//    bool close(bool){return false;}
};

#endif // RECWINDOW_H
