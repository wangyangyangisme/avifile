#ifndef QTRENDER_H
#define QTRENDER_H

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

#include <avm_locker.h>

class QPaintDevice;
class QPainter;
class QWidget;

class ShmRenderer
{
    XImage* xshmimg;
    Pixmap xshmpm;
    XShmSegmentInfo xshminfo;
    GC gc;
    avm::PthreadMutex mutex;
    int xpos, ypos;
    int m_w, m_h, pic_w, pic_h;
    QPaintDevice* dev;
    bool xshminit;

    void alloc();
    int free();
public:
    ShmRenderer(QWidget* w, int x, int y, int xpos = 0, int ypos = 0);
    ~ShmRenderer();

    int resize(int& new_w, int& new_h);
    int draw(QPainter* pm, const void* data, bool deinterlace = false);
    int getWidth() const {return m_w;}
    int getHeight() const {return m_h;}
    void move(int x, int y);
    int sync();
};

#endif // QTRENDER_H
