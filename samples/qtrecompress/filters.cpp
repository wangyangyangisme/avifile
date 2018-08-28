#include "filters.h"
#include "filters.moc"
#include "rgn.h"
#include "recompressor.h"

#include "okdialog.h"

#include <qapplication.h>
#include <qslider.h>
#include <qdialog.h>
#include <qhbox.h>
#include <qmessagebox.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qlineedit.h>
#include <qvalidator.h>
#include <qcheckbox.h>

#include <math.h>
#include <stdio.h>

#if QT_VERSION <= 220
#define LAYOUTCOL 1
#else
#define LAYOUTCOL 0
#endif

// simple fullresizable dialog which handles sliders
class MySliderDialog : public QavmOkDialog
{
public:
    MySliderDialog(const char* title) :QavmOkDialog( 0, title, true ), m_iRow(0) {}
    QSlider* addSlider( const char* param, const char* left = 0, const char* right = 0 );
protected:
    int m_iRow;
};

QSlider* MySliderDialog::addSlider(const char* param, const char* left, const char* right)
{
    QGroupBox* gb = new QGroupBox( this );
    gb->setTitle( param );
    gb->setColumnLayout( LAYOUTCOL, Qt::Vertical );
    QGridLayout* qvl = new QGridLayout( gb->layout(), 1, 1 );

    QSlider* s = new QSlider( gb );
    qvl->addMultiCellWidget(s, 0, 0, 0, 4);
    s->setTracking(false);
    s->setOrientation(QSlider::Horizontal);
    s->setTickmarks(QSlider::Below);

    if (left)
    {
	QLabel *l1 = new QLabel( gb );
	l1->setText( left );
	qvl->addWidget( l1, 1, 1 );
    }

    if (right)
    {
	QLabel *l2 = new QLabel( gb );
	l2->setText( right );
	qvl->addWidget( l2, 1, 3 );
    }

    qvl->setColStretch( 0, 1 );
    qvl->setColStretch( 2, 10 );
    qvl->setColStretch( 4, 1 );

    gridLayout()->addMultiCellWidget( gb, m_iRow, m_iRow, 0, 2 );
    m_iRow++;

    return s;
}


void Filter::redraw()
{
    if (kernel)
	kernel->redraw();
}



////////////
//  GAMMA
////////////

avm::string GammaFilter::fullname()
{
    char s[128];
    sprintf(s, tr("Adjust gamma by: %.2f"), _gamma);
    return s;
}

avm::CImage* GammaFilter::process(avm::CImage* sim, int pos)
{
    avm::CImage* im = 0;
    if (sim->Format() == 0 && sim->Depth() == 24)
    {
	im = sim;
	im->AddRef();
    }
    else
    {
        sim->GetFmt()->Print();
	avm::BitmapInfo bi(sim->GetFmt());
        bi.SetBits(24);
	//bi.SetDirection(true);
	im = new avm::CImage(sim, &bi);
    }

    for(int i=0; i<im->Height(); i++)
	for(int j=0; j<im->Width(); j++)
	{
	    unsigned char* c=im->At(j,i);
	    int z=c[0];
	    z=int(z*_gamma);
	    if(z>255)z=255;
	    c[0]=z;
	    z=c[1];
	    z=int(z*_gamma);
	    if(z>255)z=255;
	    c[1]=z;
	    z=c[2];
	    z=int(z*_gamma);
	    if(z>255)z=255;
	    c[2]=z;
	}
    return im;
}

void GammaFilter::about()
{
    QMessageBox::about(0, tr("About Gamma adjustment Filter"),
		       tr("This is a sample filter"));
}

void GammaFilter::config()
{
    MySliderDialog d( tr( "Gamma adjustment" ) );
    QSlider* s = d.addSlider( tr( "Gamma:" ), "0", "5" );

    double old_gamma = _gamma;
    s->setRange(0, 50);
    s->setValue(int(_gamma * 10));
    connect( s, SIGNAL( valueChanged( int ) ), this, SLOT( valueChanged( int ) ) );

    if (d.exec() == QDialog::Rejected)
	_gamma = old_gamma;

    redraw();
}
void GammaFilter::valueChanged(int new_val)
{
    _gamma = new_val / 10.0;
    redraw();
}
avm::string GammaFilter::save()
{
    char b[50];
    sprintf(b, "%f", _gamma);
    return b;
}

void GammaFilter::load(avm::string str)
{
    sscanf(str.c_str(), "%f", &_gamma);
}

///////////
//  BLUR
///////////

avm::string BlurFilter::fullname()
{
    char s[128];
    sprintf(s, tr("Blur - radius: %d"), _range);
    return s;
}

avm::CImage* BlurFilter::process(avm::CImage* sim, int pos)
{
    avm::CImage* im;
    if (sim->Format() == IMG_FMT_YUV)
    {
	im = sim;
	im->AddRef();
    }
    else
	im = new avm::CImage(sim, IMG_FMT_YUV);

    im->Blur(_range);
    return im;
}

void BlurFilter::about()
{
    QMessageBox::about(0, tr("About Blur Filter"),
		       tr("This is a sample MMX filter"));
}

void BlurFilter::config()
{
    MySliderDialog d(tr("Blur adjustment"));
    QSlider* s = d.addSlider(tr("Radius:"), "1", "8");
    int old_range=_range;
    s->setRange(1, 8);
    s->setValue(_range);
    //s->setLineStep(1);
#if QT_VERSION > 220
    s->setPageStep(1);
#endif
    connect(s, SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)));

    if(d.exec()==QDialog::Rejected)
	_range=old_range;
    redraw();
}
void BlurFilter::valueChanged(int new_val)
{
    _range=new_val;
    redraw();
}
avm::string BlurFilter::save()
{
    char b[50];
    sprintf(b, "%d", _range);
    return b;
}

void BlurFilter::load(avm::string s)
{
    sscanf(s.c_str(), "%d", &_range);
}


/////////////
//  REFRACT
/////////////

avm::string RefractFilter::fullname()
{
    char s[128];
    sprintf(s, tr("Compensate refraction - distance: %d, power: %.3f"),
	    _dist, _power);
    return s;
}

void RefractFilter::about()
{
    QMessageBox::about(0, tr("About Refraction Filter"),
		       tr("This is a sample filter"));
}

void RefractFilter::config()
{
    MySliderDialog d(tr("Refraction compensation"));
    QSlider* s = d.addSlider(tr("Distance:"));
    int old_dist = _dist;
    double old_power = _power;

    s->setRange(1, 50);
    s->setValue(_dist);
    connect(s, SIGNAL(valueChanged(int)), this, SLOT(valueChanged1(int)));

    s = d.addSlider(tr("Power:"));
    s->setRange(1, 50);
    s->setValue(int(_power * 100));
    connect(s, SIGNAL(valueChanged(int)), this, SLOT(valueChanged2(int)));

    if (d.exec() == QDialog::Rejected)
    {
	_dist = old_dist;
	_power = old_power;
    }
    redraw();
}

void RefractFilter::valueChanged1(int new_val)
{
    _dist = new_val;
    redraw();
}
void RefractFilter::valueChanged2(int new_val)
{
    _power = new_val / 100.0;
    redraw();
}

avm::CImage* RefractFilter::process(avm::CImage* sim, int pos)
{
    avm::CImage* im;
    if (sim->Format() == IMG_FMT_YUV)
    {
	im = sim;
	im->AddRef();
    }
    else
	im = new avm::CImage(sim, IMG_FMT_YUV);

    avm::CImage* refractor = new avm::CImage(im);

    //refractor->Blur(2);
    int xd=im->Width();
    int yd=im->Height();

    uint8_t* data = im->Data();
    uint8_t* refdata = refractor->Data();
    for (unsigned int i=xd*yd*3-3; i>=3*_dist+9+9*xd; i-=3)
    {
	unsigned char* p = data+i;
	unsigned char* refp = refdata+i-3-3*xd;
	int brightness;
	if (((i-3*_dist)%(3*xd)<(unsigned int)(2.6*xd)) || ((i%(3*xd)>(unsigned int)xd)))
	    //brightness=(refp[-3*dist+0]+refp[-3*dist+1]+refp[-3*dist+2])/3;
	    //brightness=(.114*refp[-3*dist+0]+.586*refp[-3*dist+1]+.300*refp[-3*dist+2]);
	    //brightness=((col*)(refp-3*dist))->Y();
	    brightness=refp[-3*_dist];
	else
	    brightness=0;
	brightness=int(brightness*_power);
	//int y=(*p+power*p[-3*dist])/(1+power);
	/*if(y<0x10)y=0x10;
	 if(y>0xef)y=0xef;*/
	/*p[0]=max(0, unsigned char((p[0]+brightness)/(1+power)));
	 p[1]=max(0, unsigned char((p[1]+brightness)/(1+power)));
	 p[2]=max(0, unsigned char((p[2]+brightness)/(1+power)));*/
	p[0]=(unsigned char)((p[0]+brightness)/(1+_power));
	//p[1]=unsigned char((p[1]+brightness)/(1+power));
	//p[2]=unsigned char((p[2]+brightness)/(1+power));
	//*p=y;
    }
    refractor->Release();
    return im;
}
avm::string RefractFilter::save()
{
    char b[50];
    sprintf(b, "%d %f", _dist, _power);
    return b;
}

void RefractFilter::load(avm::string s)
{
    sscanf(s.c_str(), "%d %f", &_dist, &_power);
}

////////////
//  NOISE
////////////

avm::string NoiseFilter::fullname()
{
    char s[128];
    sprintf(s, tr( "Reduce noise - quality: %d, edge radius: %d" ),
	    _qual, _edge);
    return s;
}

void NoiseFilter::about()
{
    QMessageBox::about(0, tr("About Noise reduction Filter"),
		       tr("This is a sample filter"));
}

void NoiseFilter::config()
{
    MySliderDialog d( tr( "Noise reduction" ) );
    QSlider* s = d.addSlider( tr( "Quality:" ), "850", "1000" );
    int old_qual=_qual;
    s->setRange(850,1000);
    s->setValue(_qual);
    connect(s, SIGNAL(valueChanged(int)), this, SLOT(valueChanged1(int)));

    s = d.addSlider( tr( "Radius:" ), "1", "6" );
    int old_edge=_edge;
    s->setRange(1,6);
    s->setValue(_edge);
#if QT_VERSION > 220
    s->setPageStep(1);
#endif
    connect(s, SIGNAL(valueChanged(int)), this, SLOT(valueChanged2(int)));

    s = d.addSlider( tr(""), "1", "6" ); // FIXME:  name of this parameter ????
    int old_zz=_zz;
    s->setRange(1,6);
    s->setValue(_zz);
#if QT_VERSION > 220
    s->setPageStep(1);
#endif
    connect(s, SIGNAL(valueChanged(int)), this, SLOT(valueChanged3(int)));

    if (d.exec() == QDialog::Rejected)
    {
	_qual = old_qual;
	_edge = old_edge;
	_zz = old_zz;
    }
    redraw();
}


void NoiseFilter::valueChanged1(int new_val)
{
    _qual=new_val;
    redraw();
}

void NoiseFilter::valueChanged2(int new_val)
{
    _edge=new_val;
    redraw();
}

void NoiseFilter::valueChanged3(int new_val)
{
    _zz=new_val;
    redraw();
}

avm::CImage* NoiseFilter::process(avm::CImage* sim, int pos)
{
    avm::CImage* im;
    if (sim->Format() == IMG_FMT_YUV)
    {
	im = sim;
	im->AddRef();
    }
    else
	im = new avm::CImage(sim, IMG_FMT_YUV);

    //	Move(move);
    double m_dQual=_qual/1000.;

    avm::CImage* result=new avm::CImage(im);
    //	result->Enhance(256/m_dEnhQual);//128 ms
    //	result->Move(move);

    CEdgeRgn* edges=new CEdgeRgn(im, int(256/m_dQual), _edge, false, _zz);

    avm::CImage* blurred=new avm::CImage(result);
    blurred->Blur(1);//53 ms

    avm::CImage* blurred3=new avm::CImage(blurred);

    blurred3->Blur(2,1);

    CEdgeRgn* v2=new CEdgeRgn(edges);

    v2->Normalize();//17 ms

    v2->Blur(1);
    CEdgeRgn* v3=new CEdgeRgn(v2);
    v3->Blur(2,1);

    /*
     struct yuv* yptr=(struct yuv*)result->Data();
     for(int i=0; i<im->Width(); i++)
     for(int j=0; j<im->Height(); j++)
     {
     yptr[i+j*im->Width()].Cb=yptr[i+j*im->Width()].Cr=128;
     //	    if((*v2.at(i,j))<0)
     //		yptr[i+j*im->Width()].Y=128;
     //	    else
     yptr[i+j*im->Width()].Y=*(v2.at(i,j));
     }
     */
    const int _xd=im->Width();
    const int _yd=im->Height();
    //const int OFF1=-1-1*_xd;
    const int OFF2=-2-2*_xd;
    const int OFF4=-4-4*_xd;

    for(int i=4; i<_xd-4; i++)
    {
	for(int j=i+_xd*4; j<i+_xd*(_yd-4); j+=_xd)
	{
	    avm::yuv* src=(avm::yuv*)result->Data()+j;
	    avm::yuv* v;
	    if((*v3->Offset(j+OFF4))==0)
	    {
		v=(avm::yuv*)blurred3->Data()+j+OFF4;
		src->Y=v->Y;
	    }
	    else
		if((*v2->Offset(j+OFF2))==0)
		{
		    v=(avm::yuv*)blurred3->Data()+j+OFF4;
		    src->Y=v->Y;
		}
	    //	    else
	    //	    if((*edges->Offset(j))>128)
	    //	    {
	    //	        v=(yuv*)result->Data()+j;
	    //		src->Y=128;
	    //		src->Cr=64;
	    //		src->Cb=96;
	    //		continue;
	    //	    }
	    //    	    else
	    //	    {
	    //		src->Y=128;
	    //		src->Cr=96;
	    //		src->Cb=64;
	    //		continue;
	    //	    }
	    //	    src->Cr=v->Cr;
	    //    	    src->Cb=v->Cb;
		else
		    if((*v2->Offset(j+OFF2))==0)
		    {
			v=(avm::yuv*)blurred->Data()+j+OFF2;
			src->Y=v->Y;
		    }
	    //	    else
	    //	    if((*edges->Offset(j))>128)
	    //	    {
	    //	        v=(yuv*)result->Data()+j;
	    //		src->Y=128;
	    //		src->Cr=64;
	    //		src->Cb=96;
	    //		continue;
	    //	    }
	    //    	    else
	    //	    {
	    //		src->Y=128;
	    //		src->Cr=96;
	    //		src->Cb=64;
	    //		continue;
	    //	    }
	    //	    src->Cr=v->Cr;
	    //    	    src->Cb=v->Cb;
	}
    }
    delete edges;
    delete blurred;
    delete blurred3;
    delete v2;
    delete v3;

    return result;
}
avm::string NoiseFilter::save()
{
    char b[50];
    sprintf(b, "%d %d %d", _qual, _edge, _zz);
    return b;
}

void NoiseFilter::load(avm::string s)
{
    sscanf(s.c_str(), "%d %d %d", &_qual, &_edge, &_zz);
}


///////////
// MOVE
//////////

avm::string MoveFilter::fullname()
{
    char s[128];
    sprintf(s, tr("Move colors by %d"), _delta);
    return avm::string(s);
}

avm::CImage* MoveFilter::process(avm::CImage* im, int pos)
{
    im->AddRef();
    if (im->Depth() != 24)
	return im;
    if(_delta==0)
	return im;
    if(!im->GetFmt()->IsRGB())im->ToYUV();
    avm::yuv* ptr=(avm::yuv*)im->Data();
    int csize=im->Width()*im->Height();
    if(_delta<0)
    {
	for(int i=0; i<csize+_delta; i++)
	{
	    ptr[i].Cb=ptr[i-_delta].Cb;
	    ptr[i].Cr=ptr[i-_delta].Cr;
	}
    }
    else
    {
	for(int i=csize-1; i>=_delta; i--)
	{
	    ptr[i].Cb=ptr[i-_delta].Cb;
	    ptr[i].Cr=ptr[i-_delta].Cr;
	}
    }
    return im;
}

void MoveFilter::about()
{
    QMessageBox::about(0, tr( "About Color move Filter" ),
		       tr( "This is a sample filter" ));
}

void MoveFilter::config()
{
    MySliderDialog d( tr( "Color move adjustment" ) );
    QSlider* s = d.addSlider( tr( "Color move:" ), "-20", "20");
    int old_delta=_delta;
    s->setRange(-20, 20);
    s->setValue(_delta);
    connect(s, SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)));
    if (d.exec() == QDialog::Rejected)
	_delta = old_delta;
    redraw();
}

void MoveFilter::valueChanged(int new_val)
{
    _delta=new_val;
    redraw();
}

avm::string MoveFilter::save()
{
    char b[50];
    sprintf(b, "%d", _delta);
    return b;
}

void MoveFilter::load(avm::string s)
{
    sscanf(s.c_str(), "%d", &_delta);
}
/*
 avm::CImage* SwapFilter::process(avm::CImage* im, int pos)
 {
 //    if(im->yuv())im->ToRGB();
 if(!im->yuv())im->ToYUV();
 int csize=im->Width()*im->Height();
 yuv* ptr=(yuv*)im->Data();
 for(int i=0; i<csize; i++)
 {
 int t=ptr[i].r;
 ptr[i].r=ptr[i].g;
 ptr[i].g=t;
 //	col z=col(ptr[i]);
 //	*(col*)(ptr+i)=z;
 //	yuv z(0,0,0);
 //	z.Cr=_c1*ptr[i].Cr+_c2*ptr[i].Cb;
 //	z.Cb=_c3*ptr[i].Cr+_c4*ptr[i].Cb;
 int z=ptr[i].Y;
 z+=ptr[i].Cr;
 z+=ptr[i].Cb;
 z/=3;
 ptr[i].Y=z;
 ptr[i].Cr=ptr[i].Cb=128;
 //	ptr[i]=z;

 }
 im->addref();
 return im;
 }

 void SwapFilter::config()
 {
 QDialog d(0, "Noise reduction", true);
 d.setFixedSize(300,200);
 QSlider* slider1=new QSlider(&d, "");
 QSlider* slider2=new QSlider(&d, "");
 QSlider* slider3=new QSlider(&d, "");
 QSlider* slider4=new QSlider(&d, "");
 //    slider1->setTracking(false);
 //    slider2->setTracking(false);
 //    slider3->setTracking(false);
 //    slider4->setTracking(false);
 QLabel* label1=new QLabel(&d, "");
 label1->setText("Adjust qual:");
 label1->resize(70,15);
 label1->move(10,15);
 QLabel* label2=new QLabel(&d, "");
 label2->setText("Adjust radius:");
 label2->resize(70,15);
 label2->move(10,50);
 slider1->setOrientation(QSlider::Horizontal);
 slider1->setTickmarks(QSlider::Below);
 slider2->setOrientation(QSlider::Horizontal);
 slider2->setTickmarks(QSlider::Below);
 slider3->setOrientation(QSlider::Horizontal);
 slider3->setTickmarks(QSlider::Below);
 slider4->setOrientation(QSlider::Horizontal);
 slider4->setTickmarks(QSlider::Below);

 slider1->move(90,15);
 slider1->resize(200,25);
 slider1->setRange(-200,200);
 slider1->setValue(int(_c1*100));
 slider2->move(90,50);
 slider2->resize(200,25);
 slider2->setRange(-200,200);
 slider2->setValue(int(_c2*100));
 slider3->move(90,85);
 slider3->resize(200,25);
 slider3->setRange(-200,200);
 slider3->setValue(int(_c3*100));
 slider4->move(90,135);
 slider4->resize(200,25);
 slider4->setRange(-200,200);
 slider4->setValue(int(_c4*100));
 connect(slider1, SIGNAL(valueChanged(int)), this, SLOT(valueChanged1(int)));
 connect(slider2, SIGNAL(valueChanged(int)), this, SLOT(valueChanged2(int)));
 connect(slider3, SIGNAL(valueChanged(int)), this, SLOT(valueChanged3(int)));
 connect(slider4, SIGNAL(valueChanged(int)), this, SLOT(valueChanged4(int)));
 QPushButton* b_ok=new QPushButton("Ok", &d);
 b_ok->resize(40,20);
 b_ok->move(200,160);
 QPushButton* b_cancel=new QPushButton("Cancel", &d);
 b_cancel->resize(40,20);
 b_cancel->move(250,160);
 connect(b_ok, SIGNAL(clicked()), &d, SLOT(accept()));
 connect(b_cancel, SIGNAL(clicked()), &d, SLOT(reject()));
 d.exec();
 kernel->redraw();
 }
 void SwapFilter::valueChanged1(int new_val)
 {
 _c1=double(new_val)/100.;
 kernel->redraw();
 }
 void SwapFilter::valueChanged2(int new_val)
 {
 _c2=double(new_val)/100.;
 kernel->redraw();
 }
 void SwapFilter::valueChanged3(int new_val)
 {
 _c3=double(new_val)/100.;
 kernel->redraw();
 }
 void SwapFilter::valueChanged4(int new_val)
 {
 _c4=double(new_val)/100.;
 kernel->redraw();
 }
 */

namespace {
    typedef int myGLint;
    typedef unsigned int myGLuint;
    typedef unsigned short int myGLushort;
    typedef char myGLbyte;
    typedef unsigned char myGLubyte;

    void halveImage(myGLint components, myGLuint width, myGLuint height,
		    const myGLubyte *datain, myGLubyte *dataout)
    {
	int i, j, k;
	int newwidth, newheight;
	int delta;
	myGLubyte *s;
	const myGLubyte *t;

	newwidth = width / 2;
	newheight = height / 2;
	delta = width * components;
	s = dataout;
	t = datain;

	/* Piece o' cake! */
	for (i = 0; i < newheight; ++i) {
	    for (j = 0; j < newwidth; ++j) {
		for (k = 0; k < components; ++k) {
		    s[0] = (t[0] + t[components] + t[delta] +
			    t[delta+components] + 2) / 4;
		    ++s; ++t;
		}
		t += components;
	    }
	    t += delta;
	}
    }

    void scale_internal(myGLint components, myGLint widthin, myGLint heightin,
			const myGLubyte *datain,
			myGLint widthout, myGLint heightout,
			myGLubyte *dataout)
    {
	float x, lowx, highx, convx, halfconvx;
	float y, lowy, highy, convy, halfconvy;
	float xpercent,ypercent;
	float percent;
	/* Max components in a format is 4, so... */
	float totals[4];
	float area;
	int i,j,k,yint,xint,xindex,yindex;
	int temp;

	if (widthin == widthout*2 && heightin == heightout*2) {
	    halveImage(components, widthin, heightin, datain, dataout);
	    return;
	}
	convy = (float) heightin/heightout;
	convx = (float) widthin/widthout;
	halfconvx = convx/2;
	halfconvy = convy/2;
	for (i = 0; i < heightout; i++) {
	    y = convy * (i+0.5);
	    if (heightin > heightout) {
		highy = y + halfconvy;
		lowy = y - halfconvy;
	    } else {
		highy = y + 0.5;
		lowy = y - 0.5;
	    }
	    for (j = 0; j < widthout; j++) {
		x = convx * (j+0.5);
		if (widthin > widthout) {
		    highx = x + halfconvx;
		    lowx = x - halfconvx;
		} else {
		    highx = x + 0.5;
		    lowx = x - 0.5;
		}

		/*
		 ** Ok, now apply box filter to box that goes from (lowx, lowy)
		 ** to (highx, highy) on input data into this pixel on output
		 ** data.
		 */
		totals[0] = totals[1] = totals[2] = totals[3] = 0.0;
		area = 0.0;

		y = lowy;
		yint = (int)floor(y);
		while (y < highy) {
		    yindex = (yint + heightin) % heightin;
		    if (highy < yint+1) {
			ypercent = highy - y;
		    } else {
			ypercent = yint+1 - y;
		    }

		    x = lowx;
		    xint = (int)floor(x);

		    while (x < highx) {
			xindex = (xint + widthin) % widthin;
			if (highx < xint+1) {
			    xpercent = highx - x;
			} else {
			    xpercent = xint+1 - x;
			}

			percent = xpercent * ypercent;
			area += percent;
			temp = (xindex + (yindex * widthin)) * components;
			for (k = 0; k < components; k++) {
			    totals[k] += datain[temp + k] * percent;
			}

			xint++;
			x = xint;
		    }
		    yint++;
		    y = yint;
		}

		temp = (j + (i * widthout)) * components;
		for (k = 0; k < components; k++) {
		    /* totals[] should be rounded in the case of enlarging an RGB
		     * ramp when the type is 332 or 4444
		     */
		    dataout[temp + k] = (short unsigned int)((totals[k]+0.5)/area);
		}
	    }
	}
    }
};

avm::string ScaleFilter::fullname()
{
    char s[128];
    sprintf(s, tr( "Scale to %.2f x %.2f" ), m_dWidth, m_dHeight);
    return s;
}

void ScaleFilter::about()
{
    QMessageBox::about( 0, tr( "About Scal Filter" ),
			tr( "Universal scale filter" ) );
}

void ScaleFilter::config()
{
    bool old_aspect = m_bAspect;
    bool old_percent = m_bPercent;
    double old_width = m_dWidth;
    double old_height = m_dHeight;
    QString s;

    QavmOkDialog d(0, tr( "Scale filter" ), true );

    QCheckBox* chb1 = new QCheckBox( tr( "Preserve aspect ratio" ), &d );
    chb1->setChecked(m_bAspect);
    d.gridLayout()->addWidget(chb1, 0, 0);

    connect(chb1, SIGNAL(clicked()), this, SLOT(onAspectToggle()));

    QCheckBox* chb2 = new QCheckBox( tr( "Use as percentage" ), &d );
    chb2->setChecked(m_bPercent);
    d.gridLayout()->addWidget(chb2, 1, 0);

    connect(chb2, SIGNAL(clicked()), this, SLOT(onPercentToggle()));

    QDoubleValidator qv( 0.0, 1000.0, 3, &d );
    m_pLew = new QLineEdit( &d, "scale_width" );
    m_pLew->setValidator( &qv );
    m_pLew->setText( s.sprintf("%.2f", m_dWidth) );
    d.gridLayout()->addMultiCellWidget( m_pLew, 2, 2, 0, 0 );

    connect(m_pLew, SIGNAL(returnPressed()), this, SLOT(valueChanged()));

    m_pLeh = new QLineEdit( &d, "scale_height" );
    m_pLeh->setValidator( &qv );
    m_pLeh->setText( s.sprintf("%.2f",m_dHeight) );
    d.gridLayout()->addMultiCellWidget( m_pLeh, 3, 3, 0, 0 );

    connect(m_pLeh, SIGNAL(returnPressed()), this, SLOT(valueChanged()));

    if (d.exec() == QDialog::Rejected)
    {
	fprintf(stderr, "tb:config> QDialog::Rejected\n");
	m_bAspect = old_aspect;
	m_bPercent = old_percent;
	m_dWidth = old_width;
	m_dHeight = old_height;
    }
    else
    {
	valueChanged();
	valueChanged();
    }
    redraw();
}

avm::CImage* ScaleFilter::process(avm::CImage* im, int pos)
{
    if (im->Depth()!=24)
    {
	im->AddRef();
	return im;
    }

    avm::BitmapInfo info = *(im->GetFmt());
    adjust(info);
    avm::CImage* result = new avm::CImage(&info);

    scale_internal(3,im->Width(),im->Height(),(myGLubyte *)(im->Data()),
		   result->Width(),result->Height(),(myGLubyte *)(result->Data()));
    return result;
}

void ScaleFilter::adjust(BITMAPINFOHEADER& bh)
{
    double r = bh.biWidth / (double) bh.biHeight;
    if (m_bPercent)
    {
	bh.biWidth = int(bh.biWidth * (m_dWidth / 100.0));
	bh.biHeight = int(bh.biHeight * (m_dHeight / 100.0));
    }
    else
    {
	bh.biWidth = (int)m_dWidth;
	bh.biHeight = (int)m_dHeight;
    }

    if (m_bAspect)
	bh.biHeight = int(bh.biWidth / r);

    bh.biSizeImage = bh.biWidth * bh.biHeight * (bh.biBitCount / 8);
}

void ScaleFilter::onAspectToggle()
{
    m_bAspect = !m_bAspect;
    redraw();
}

void ScaleFilter::onPercentToggle()
{
    m_bPercent = !m_bPercent;
    redraw();
}

void ScaleFilter::valueChanged()
{
    m_dWidth = m_pLeh->text().toFloat();
    m_dHeight = m_pLeh->text().toFloat();
    redraw();
}

class FilterSelectionDialog : public QDialog
{
public:
    QGridLayout* m_pGl;
    FilterSelectionDialog(const char* title);
    int exec();
};
