#include "imagecontrol.h"

#include <avm_locker.h>

#include <qwidget.h>

class QFrame;
class QImage;

class QImageControl: public QWidget, public IImageControl
{
public:
    QImageControl(QWidget* parent, const char* name="Image control");
    virtual ~QImageControl();
    virtual void setSize(int width, int height);
    virtual void setSourcePicture(const avm::CImage*);
    virtual void setDestPicture(const avm::CImage*); 
    bool close(bool) { return false; }
    virtual void paintEvent(QPaintEvent*);
protected:
    void setPicture(const QImage*, QFrame*);
    bool event(QEvent* ev);
    QImage* getQImage(const avm::CImage*) const;

    avm::PthreadMutex m_Mutex;
    QFrame* _left;
    QFrame* _right;
    QImage* _il;
    QImage* _ir;
};
