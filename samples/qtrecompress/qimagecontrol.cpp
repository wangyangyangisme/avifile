#include "qimagecontrol.h"

#include <qapplication.h>
#include <qframe.h>
#include <qimage.h>
#include <qpainter.h>
#include <qpixmap.h>

#include <stdlib.h> // abort()
#include <stdio.h>

const QEvent::Type Type_LeftPix=QEvent::Type(QEvent::User);
const QEvent::Type Type_RightPix=QEvent::Type(QEvent::User+1);

QImageControl::QImageControl(QWidget* parent, const char* name):QWidget(parent,name),_il(0), _ir(0)
{
    _left = new QFrame( this );
    _right = new QFrame( this );
    _left->setFrameShadow( QFrame::Sunken );
    _left->setFrameShape( QFrame::Box );
    _right->setFrameShadow( QFrame::Sunken );
    _right->setFrameShape( QFrame::Box );
    _il = _ir = 0;
    //setSize( 192,144 );
}

QImageControl::~QImageControl()
{
    delete _il;
    delete _ir;
}

void QImageControl::setSize(int width, int height)
{
    if (height<0)
	height=-height;
    _left->resize( width, height );
    _right->resize( width, height );

    int nw = width + 10;
    int nh = 5;

    if (height < 400)
    {
	nw = 5;
	nh = height + 10;
    }
    _left->move( 5, 5 );
    _right->move( nw, nh );
    resize( width + 5 + nw, height + 5 + nh );
}

QImage* QImageControl::getQImage(const avm::CImage* im) const
{
    int w = im->Width();
    int h = im->Height();
    int bitdepth = 32;
    //printf("HEI %d\n", _h);
    QImage* qi = new QImage( w, h, bitdepth );
    avm::BitmapInfo bi(w, h, bitdepth);
    avm::CImage ci(&bi, qi->scanLine(0), false);
    ci.Convert(im);
    return qi;
}

void QImageControl::setSourcePicture(const avm::CImage* src)
{
    //printf("SRC:\n"); src->GetFmt()->Print();
    m_Mutex.Lock();
    QImage* s32 = getQImage(src);
    delete _il;
    _il = s32;
    QEvent* e = new QEvent(Type_LeftPix);
    qApp->postEvent(this, e);
    m_Mutex.Unlock();
    //qApp->sendPostedEvents();
}

void QImageControl::setDestPicture(const avm::CImage* src)
{
    //printf("DST:\n"); src->GetFmt()->Print();
    m_Mutex.Lock();
    QImage* s32 = getQImage(src);
    delete _ir;
    _ir = s32;
    QEvent* e = new QEvent(Type_RightPix);
    qApp->postEvent(this, e);
    m_Mutex.Unlock();
}

void QImageControl::setPicture(const QImage* qi, QFrame* frame)
{
    if (qi)
    {
	QPainter p(frame);
	p.drawImage( 0, 0, *qi );
    }
}

void QImageControl::paintEvent(QPaintEvent* e)
{
    avm::Locker locker(m_Mutex);
    setPicture(_il, _left);
    setPicture(_ir, _right);
}

bool QImageControl::event(QEvent* ev)
{
    static int lastWidth=0, lastHeight=0;

    if (ev->type()==Type_LeftPix)
    {
	avm::Locker locker(m_Mutex);
	setPicture(_il, _left);
	return true;
    }
    else if (ev->type()==Type_RightPix)
    {
	avm::Locker locker(m_Mutex);
	// first clear the destination frame if the picture size has changed
	if(_ir->width()!=lastWidth || _ir->height()!=lastHeight){
	    lastWidth=_ir->width();
	    lastHeight=_ir->height();
	    QPainter p(_right);
	    p.eraseRect(0,0,_il->width(),_il->height());
	}
	// then set the new picture
	setPicture(_ir, _right);
	return true;
    }

    if (ev->type() == QEvent::Paint)
	return QWidget::event(ev); // this seems to help with flickering
    return false;
}
