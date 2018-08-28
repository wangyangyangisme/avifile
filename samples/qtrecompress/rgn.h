#ifndef IMAGE_H
#define IMAGE_H

#include <image.h>

class CEdgeRgn: public avm::CImage
{
public:
    CEdgeRgn(avm::CImage* im, int _qual, int _edge, bool flag, int mxm);
    CEdgeRgn(CEdgeRgn* im) :avm::CImage(im) {}
    static void Maximize(unsigned char* data, int xd, int yd, int depth=3);
    void Normalize();
    void Blur(int depth, int src=0);
};

#endif
