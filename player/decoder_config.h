#ifndef DECODER_CONFIG_H
#define DECODER_CONFIG_H

#include <qdialog.h>

#include <infotypes.h>
#include <videodecoder.h>

class QSlider;

class QConfDialog: public QDialog
{
    Q_OBJECT;

public:
    QConfDialog(const avm::vector<avm::AttributeInfo>&);
public slots:
    virtual void reset();
    virtual void reject();
    virtual void valueChanged(int)	= 0;
protected:
    const avm::vector<avm::AttributeInfo>& attrs;
    avm::vector<QSlider*> sliders;
    avm::vector<int> original;
};

class QCodecConf: public QConfDialog
{
    Q_OBJECT;

public:
    QCodecConf(const avm::CodecInfo&, avm::IRtConfig* _rt=0);
public slots:
    virtual void valueChanged(int);
protected:
    const avm::CodecInfo& info;
    avm::IRtConfig* rt;
};

class QRendererConf: public QConfDialog
{
    Q_OBJECT;

public:
    QRendererConf(const avm::vector<avm::AttributeInfo>&, avm::IRtConfig* _rt=0);
public slots:
    virtual void valueChanged(int);
protected:
    avm::IRtConfig* rt;
};

#endif //DECODER_CONFIG_H
