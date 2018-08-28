#ifndef FILTERS_H
#define FILTERS_H

#include <image.h>

#include <qobjectdefs.h>
#include <qobject.h>

class RecKernel;

class Filter
{
    int counter;
public:
    Filter(RecKernel* k = 0) : counter(1), kernel(k) {}
    virtual ~Filter(){}
    virtual void about() =0;
    virtual void config() =0;
    virtual const char* name() const =0;	//for list of all filters
    virtual avm::string fullname() =0; //includes config options
    virtual avm::CImage* process(avm::CImage* im, int pos) =0;   //filters may affect only a part of stream
    virtual void addref() { counter++; }
    virtual void release() { counter--; if (!counter) delete this; }
    virtual avm::string save() =0;
    virtual void load(avm::string) =0;
    virtual int id() const { return -1; }
    virtual void adjust(BITMAPINFOHEADER&) {};
protected:
    virtual void redraw();

    RecKernel* kernel;
};

class GammaFilter: public QObject, public Filter
{
    Q_OBJECT;

    float _gamma;
public:
    GammaFilter(RecKernel* k) : Filter(k), _gamma(1) {}
    virtual const char* name() const { return "Gamma adjustment"; }
    virtual avm::string fullname();
    virtual avm::CImage* process(avm::CImage* im, int pos);
    virtual void about();
    virtual void config();
    virtual avm::string save();
    virtual void load(avm::string);
    virtual int id() const {return 0;}
public slots:
    void valueChanged(int new_val);
};

class BlurFilter: public QObject, public Filter
{
    Q_OBJECT;

    int _range;
public:
    BlurFilter(RecKernel* k) : Filter(k), _range(3) {}
    virtual const char* name() const { return "Blur filter"; }
    virtual avm::string fullname();
    virtual avm::CImage* process(avm::CImage* im, int pos);
    virtual void about();
    virtual void config();
    virtual avm::string save();
    virtual void load(avm::string);
    virtual int id() const { return 1; }
public slots:
    void valueChanged(int new_val);
};

class RefractFilter:  public QObject, public Filter
{
    Q_OBJECT;

    unsigned int _dist;
    float _power;
public:
    RefractFilter(RecKernel* k) : Filter(k), _dist(5), _power(.1) {}
    virtual const char* name() const { return "Refraction compensation"; }
    virtual avm::string fullname();
    virtual avm::CImage* process(avm::CImage* im, int pos);
    virtual void about();
    virtual void config();
    virtual avm::string save();
    virtual void load(avm::string);
    virtual int id() const { return 2; }
public slots:
    void valueChanged1(int new_val);
    void valueChanged2(int new_val);
};

class NoiseFilter: public QObject, public Filter
{
    Q_OBJECT;

    int _qual;
    int _edge;
    int _zz;
public:
    NoiseFilter(RecKernel* k) : Filter(k), _qual(940), _edge(2), _zz(4) {}
    virtual const char* name() const { return "Noise reduction"; }
    virtual avm::string fullname();
    virtual avm::CImage* process(avm::CImage* im, int pos);
    virtual void about();
    virtual void config();
    virtual avm::string save();
    virtual void load(avm::string);
    virtual int id() const { return 3; }
public slots:
    void valueChanged1(int new_val);
    void valueChanged2(int new_val);
    void valueChanged3(int new_val);
};

class MoveFilter: public QObject, public Filter
{
    Q_OBJECT;

    int _delta;
public:
    MoveFilter(RecKernel* k) : Filter(k), _delta(0) {}
    virtual const char* name() const { return "Color move"; }
    virtual avm::string fullname();
    virtual avm::CImage* process(avm::CImage* im, int pos);
    virtual void about();
    virtual void config();
    virtual avm::string save();
    virtual void load(avm::string);
    virtual int id() const {return 4;}
public slots:
    void valueChanged(int new_val);
};
/*
class SwapFilter: public QObject, public Filter
{
    RecKernel* kernel;
    Q_OBJECT
    double _c1, _c2, _c3, _c4;    
public:
    SwapFilter(RecKernel* k):kernel(k), _c1(1), _c2(0), _c3(0), _c4(1){}
    virtual string name(){return "Swap";}
    virtual string fullname(){return "Swap";}
    virtual CImage* process(CImage* im, int pos);
    virtual void about(){}
    virtual void config();
    virtual string save(){}
    virtual void load(string){}    
    virtual int id() const {return 5;}
public slots:
    void valueChanged1(int new_val);
    void valueChanged2(int new_val);
    void valueChanged3(int new_val);
    void valueChanged4(int new_val);
};
*/

class QLineEdit;
class ScaleFilter: public QObject, public Filter
{
    Q_OBJECT;

    float m_dWidth, m_dHeight;
    bool m_bAspect;
    bool m_bPercent;
    enum ScaleType { FAST, RESAMPLE } type;
    QLineEdit* m_pLew;
    QLineEdit* m_pLeh;

public:
    ScaleFilter(RecKernel* k) : Filter(k), m_dWidth(256), m_dHeight(256),
	m_bAspect(false), m_bPercent(false) {}
    virtual const char* name() const { return "Scale"; }
    virtual avm::string fullname();
    virtual avm::CImage* process(avm::CImage* im, int pos);
    virtual void about();
    virtual void config();
    virtual avm::string save() { return ""; }
    virtual void load(avm::string){}
    virtual int id() const { return 5; }
    virtual void adjust(BITMAPINFOHEADER&);
public slots:
    void onAspectToggle();
    void onPercentToggle();
    void valueChanged();
};

#endif // FILTERS_H
