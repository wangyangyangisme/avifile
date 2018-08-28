#ifndef IIMAGECONTROL_H
#define IIMAGECONTROL_H

#include <image.h>

class IImageControl
{
public:
    virtual ~IImageControl(){}
    virtual void setSize(int width, int height) 	=0;
    virtual void setSourcePicture(const avm::CImage*)	=0;
    virtual void setDestPicture(const avm::CImage*)	=0;
};

#endif
