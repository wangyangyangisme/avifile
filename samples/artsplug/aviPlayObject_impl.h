#ifndef __AVIPLAYOBJECT_IMPL_H
#define __AVIPLAYOBJECT_IMPL_H

#include "playobjects.h"
#include <aviplay.h>

using namespace Arts;
using namespace std;
class AVIPlayObject_impl: virtual public AVIPlayObject_skel 
{
    IAviPlayer2* _player;
    Display* _dpy;
public:
    AVIPlayObject_impl();
    virtual ~AVIPlayObject_impl();

    virtual string description() {return string("Arts AVI plugin");}
    virtual poTime currentTime();
    virtual poTime overallTime();
    virtual poCapabilities capabilities();
    virtual string mediaName();
    virtual poState state();
    
    virtual void play();
    virtual void seek(const Arts::poTime& newTime);
    virtual void pause();
    virtual void halt();

    virtual bool loadMedia(const std::string& filename) ;
};


#endif
