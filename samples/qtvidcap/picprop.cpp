#include "picprop.h"
#include "picprop.moc"

#include "v4lxif.h"

#define DECLARE_REGISTRY_SHORTCUT
#include "configfile.h"
#undef DECLARE_REGISTRY_SHORTCUT

#include <qlabel.h>
#include <qslider.h>

#include <stdio.h>
/*
 *  Constructs a PicturePropDialog which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
PicturePropDialog::PicturePropDialog(v4lxif* pDev)
    : PicPropDialog( 0, 0, FALSE, 0), m_pDev(pDev)
{
    connect(m_pSlider[PROP_BRIGHTNESS], SIGNAL(valueChanged(int)), this, SLOT(changeBrightness(int)));
    connect(m_pSlider[PROP_SATURATION], SIGNAL(valueChanged(int)), this, SLOT(changeSaturation(int)));
    connect(m_pSlider[PROP_CONTRAST], SIGNAL(valueChanged(int)), this, SLOT(changeContrast(int)));
    connect(m_pSlider[PROP_HUE], SIGNAL(valueChanged(int)), this, SLOT(changeHue(int)));

    m_pSlider[PROP_BRIGHTNESS]->setValue( RI("Image\\Brightness", 0) );
    m_pSlider[PROP_SATURATION]->setValue( (RI("Image\\Color", 255)-255)/2 );
    m_pSlider[PROP_CONTRAST]->setValue( (RI("Image\\Contrast", 255)-255)/2 );
    m_pSlider[PROP_HUE]->setValue( RI("Image\\Hue", 0) );
}

/*
 *  Destroys the object and frees any allocated resources
 */
PicturePropDialog::~PicturePropDialog()
{
    // no need to delete child widgets, Qt does it all for us
}


void PicturePropDialog::changeBrightness(int val)
{
    QString s = QString().sprintf( tr("Brightness")+": %d", val );
    m_pLabel[PROP_BRIGHTNESS]->setText( s );
    m_pDev->setPicBrightness( val );
}
void PicturePropDialog::changeContrast(int val)
{
    QString s = QString().sprintf( tr("Contrast:")+": %d", val );
    m_pLabel[PROP_CONTRAST]->setText( s );
    m_pDev->setPicConstrast( 2 * (val + 128) );
}
void PicturePropDialog::changeHue(int val)
{
    QString s = QString().sprintf( tr("Hue")+": %d", val );
    m_pLabel[PROP_HUE]->setText( s );
    m_pDev->setPicHue( val );
}
void PicturePropDialog::changeSaturation(int val)
{
    QString s = QString().sprintf( tr("Saturation")+": %d", val );
    m_pLabel[PROP_SATURATION]->setText( s );
    m_pDev->setPicColor( 2 * (val + 128) );
}

void PicturePropDialog::accept()
{
    WI("Image\\Brightness", m_pSlider[PROP_BRIGHTNESS]->value());
    WI("Image\\Color", 256+2*m_pSlider[PROP_SATURATION]->value());
    WI("Image\\Contrast", 256+2*m_pSlider[PROP_CONTRAST]->value());
    WI("Image\\Hue", m_pSlider[PROP_HUE]->value());
//    printf("Writing: %d %d %d %d\n",
//	m_pSBrightness->value(),
//	256+2*m_pSConstrast->value(),
//	256+2*m_pSSaturation->value(),
//	m_pSHue->value()
//	);	
    return QDialog::accept();
}
