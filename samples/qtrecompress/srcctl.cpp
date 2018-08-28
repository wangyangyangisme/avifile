#include "srcctl.h"
#include "srcctl.moc"

#include <qslider.h>
#include <qlabel.h>

#include <stdio.h>

/*
 *  Constructs a SourceControl which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
SourceControl::SourceControl( QWidget* parent, RecKernel* kern )
    : SourceControlDialog( parent, "Source control", false, 0 ),
    m_pKernel(kern), scale(1), btn_press(0)
{
    updateSize();
    //Slider1->setTracking(false);
    m_pSlider->setTracking( true );
}

/*
 *  Destroys the object and frees any allocated resources
 */
SourceControl::~SourceControl()
{
    // no need to delete child widgets, Qt does it all for us
}

void SourceControl::prevFrame()
{
    m_pKernel->seek(-1);
    updateLabel();
}

void SourceControl::prevFiveFrames()
{
    m_pKernel->seek(-5);
    updateLabel();
}

void SourceControl::prevTwentyFrames()
{
    m_pKernel->seek(-20);
    updateLabel();
}

void SourceControl::prevKeyFrame()
{
    m_pKernel->seekPrevKeyFrame();
    updateLabel();
}

void SourceControl::nextFrame()
{
    m_pKernel->seek(1);
    updateLabel();
}

void SourceControl::nextFiveFrames()
{
    m_pKernel->seek(5);
    updateLabel();
}

void SourceControl::nextTwentyFrames()
{
    m_pKernel->seek(20);
    updateLabel();
}

void SourceControl::nextKeyFrame()
{
    m_pKernel->seekNextKeyFrame();
    updateLabel();
}

void SourceControl::valueChanged()
{
    framepos_t pos = m_pSlider->value() * scale;
    framepos_t c = m_pKernel->pos();

    //cout << "poschane " << c << "    " << pos << endl;
    //if (!btn_press)

    if (pos != c)
    {
	pos = m_pKernel->seekPos(pos);
    }
    updateLabel();
}

void SourceControl::sliderMoved( int value )
{
    valueChanged();
}

void SourceControl::sliderReleased( int value )
{
}

void SourceControl::updateLabel()
{
    char s[256];
    double t = m_pKernel->getTime((uint_t)0);
    int sti = int(t);

    //m_pSlider->
    sprintf(s, "frame %6d  %.2d:%.2d:%.2d.%.3d", m_pKernel->pos(),
	    sti/3600, (sti/60) % 60, sti % 60, int((t - sti) * 1000));

    m_pLabel->setText( s );
}

void SourceControl::updateSize()
{
    int len = m_pKernel->getVideoLength();
    scale = len / 500 + 1;
    m_pSlider->setRange( 0, len / scale );
    m_pSlider->setValue( 0 );
    updateLabel();
}
