#include "decoder_config.h"
#include "decoder_config.moc"

#include "playercontrol.h"

#include "avm_creators.h"
#include "videodecoder.h"
#include "renderer.h"
#include "avm_except.h"

#include <qgroupbox.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qslider.h>

#include <stdio.h>

QConfDialog::QConfDialog(const avm::vector<avm::AttributeInfo>& _attrs)
    :QDialog(0, "", true), attrs(_attrs)
{
#if QT_VERSION > 220
    setSizeGripEnabled( TRUE );
#endif

    QVBoxLayout *top_vbl = new QVBoxLayout(this, 5);
    top_vbl->setSpacing(2);
    //m_pTabAttr = new QListView( gb );

    sliders.resize(attrs.size());
    original.resize(attrs.size());

    for (unsigned i = 0; i < attrs.size(); i++)
    {
	sliders[i] = 0;
	if (attrs[i].kind == avm::AttributeInfo::Integer)
	{
	    if (attrs[i].i_min + 0 >= attrs[i].i_max)
		continue;//integers with unlimited values
			 //and boolean attributes are not supported
	    QGroupBox* box = new QGroupBox( this );
	    box->setTitle( attrs[i].GetAbout() );
	    box->setColumnLayout(
#if QT_VERSION > 220
				 0,
#else
				 1, // there is bug and 0 doesn't work
#endif
				 Qt::Vertical );

	    top_vbl->addWidget(box);
	    QVBoxLayout *box_vbl = new QVBoxLayout( box->layout() );

            int pgs = 1;
	    if (attrs[i].i_max - attrs[i].i_min > 20)
		pgs = (attrs[i].i_max - attrs[i].i_min) / 10;

	    QSlider* slider = new QSlider( attrs[i].i_min, attrs[i].i_max, pgs, 0, QSlider::Horizontal, box );
#if QT_VERSION > 220
	    slider->setLineStep(1);
#endif
	    sliders[i] = slider;
            box_vbl->addWidget(slider);
	    slider->setTickmarks(QSlider::Both);

	    QGridLayout *box_hbl = new QGridLayout( box_vbl, 1, 1 );
	    char txt[100];
	    sprintf(txt, "%d", attrs[i].i_min);
	    QLabel* label = new QLabel( txt, box );
            box_hbl->addWidget(label, 0, 0);
	    sprintf(txt, "%d", attrs[i].i_max);
	    label = new QLabel( txt, box );
	    box_hbl->setColStretch(1, 1);
	    box_hbl->addWidget(label, 0, 2);
	}
	//other types of atributes are not supported
    }

    top_vbl->addStretch(1);

    QHBox* hbox = new QHBox(this);
    top_vbl->addWidget(hbox);

    QPushButton* b_ok = new QPushButton( tr( "&Ok" ), hbox );
    b_ok->setDefault( TRUE );
    QPushButton* b_reset = new QPushButton( tr ( "&Reset" ), hbox );
    QPushButton* b_cancel = new QPushButton( tr( "&Cancel" ), hbox );

    connect( b_ok, SIGNAL( clicked() ), this, SLOT( accept() ) );
    connect( b_reset, SIGNAL( clicked() ), this, SLOT( reset() ) );
    connect( b_cancel, SIGNAL( clicked() ), this, SLOT( reject() ) );
}

void QConfDialog::reject()
{
    // restore original values & reject
    for (unsigned i = 0; i < attrs.size(); i++)
	if (sliders[i] &&  (original[i] != sliders[i]->value()))
	    sliders[i]->setValue(original[i]);

    return QDialog::reject();
}

void QConfDialog::reset()
{
    for (unsigned i = 0; i < attrs.size(); i++)
	if (sliders[i])
	    sliders[i]->setValue(attrs[i].GetDefault());
}


QCodecConf::QCodecConf( const avm::CodecInfo& _info, avm::IRtConfig* _rt )
    : QConfDialog( _rt ? _rt->GetAttrs() : _info.decoder_info ),
    info(_info), rt(_rt)
{
    setCaption( tr( "Decoder configuration" ) );

    for (unsigned i = 0; i < attrs.size(); i++)
    {
	int val = 0;
	if (rt)
	    rt->GetValue(attrs[i].GetName(), &val);
	else
	    avm::CodecGetAttr(info, attrs[i].GetName(), &val);
	if (sliders[i])
	{
	    sliders[i]->setValue(val);
	    // so we have arrays of the same length
	    original[i] = val;

	    if (rt)
		connect(sliders[i], SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)));
	}
    }
}

void QCodecConf::valueChanged(int)
{
    for (unsigned i = 0; i < attrs.size(); i++)
	if (sliders[i])
	{
	    int v = sliders[i]->value();
	    if (rt)
		rt->SetValue(attrs[i].GetName(), v);
	    // store to registers
	    avm::CodecSetAttr(info, attrs[i].GetName(), v);
	}
}

#define __MODULE__ "RendererConfig"

QRendererConf::QRendererConf( const avm::vector<avm::AttributeInfo>& _attrs, avm::IRtConfig* _rt )
    : QConfDialog( _attrs ), rt(_rt)
{
    if (!rt)
        throw FATAL("Missing IRtConfig");

    setCaption( tr( "Renderer configuration" ) );

    for (unsigned i = 0; i < attrs.size(); i++)
    {
	if (sliders[i])
	{
	    int val = 0;
	    rt->GetValue(attrs[i].GetName(), &val);
	    original[i] = val;

	    sliders[i]->setValue(val);
	    connect(sliders[i], SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)));
	}
    }
}

void QRendererConf::valueChanged(int)
{
    for (unsigned i = 0; i < attrs.size(); i++)
    {
	QSlider* slider = sliders[i];

	if (slider)
	{
	    int v = sliders[i]->value();

	    if (rt)
		rt->SetValue(attrs[i].GetName(), v);
	}
    }
}
