#ifndef IRecompressCallback_H
#define IRecompressCallback_H

#include <avm_default.h>

class IRecompressCallback
{
public:
    virtual ~IRecompressCallback() {}
    virtual void addAudio(framepos_t asample, unsigned int asize, double atime) = 0;
    virtual void addVideo(framepos_t vframe, unsigned int vsize, double vtime, bool keyframe) =0;
    virtual void setTotal(framepos_t vtotal, framepos_t atotal,
			  double vtime, double atime) =0;
    virtual void setNewState(double progress, double elapsed, int64_t filesize) =0;
    virtual void finished() =0;
};
#endif
