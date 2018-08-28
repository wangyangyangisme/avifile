#include "picprop_p.h"
#include "picprop_p.moc"

#include <qframe.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qslider.h>

/* 
 *  Constructs a PicPropDialog which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
PicPropDialog::PicPropDialog( QWidget* parent,  const char* name, bool modal, WFlags fl )
    : QDialog( parent, name, modal, fl )
{
    if ( !name )
	setName( "PicPropDialog" );

    setCaption( tr( "Picture Properties" ) );
#if QT_VERSION > 220
    setSizeGripEnabled( TRUE );
#endif
    //setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)0, (QSizePolicy::SizeType)0, sizePolicy().hasHeightForWidth() ) );
    //setMouseTracking( TRUE );

    QVBoxLayout* vb = new QVBoxLayout( this, 5 );

    QFrame* pFrame = new QFrame( this );
    pFrame->setFrameShadow( QFrame::Raised );
    //pFrame->setFrameShadow( QFrame::Sunken );
    //pFrame->setFrameShape( QFrame::Box );
    pFrame->setFrameShape( QFrame::WinPanel );

    vb->addWidget( pFrame );

    QGridLayout* gl = new QGridLayout( pFrame, 1, 1 );
    gl->setMargin( 10 );
    gl->setSpacing( 5 );

    for (unsigned i = 0; i < PROP_LAST; i++) {
	m_pSlider[i] = new QSlider( -128, 127, 16, 0, QSlider::Horizontal, pFrame );
	m_pSlider[i]->setTickmarks( QSlider::Right );
	m_pSlider[i]->setMinimumWidth( 150 );
	m_pSlider[i]->setValue( -128 );
	m_pLabel[i] = new QLabel( "", pFrame );
	m_pLabel[i]->setMinimumWidth( 110 );
	gl->addWidget( m_pSlider[i], i, 0 );
	gl->addWidget( m_pLabel[i], i, 1 );
    }

    QHBoxLayout* hb = new QHBoxLayout( vb );
    hb->addStretch( 1 );
    QPushButton* pButton = new QPushButton( tr( "&Ok" ), this );
    pButton->setDefault( TRUE );
    hb->addWidget( pButton );
    hb->addStretch( 1 );

    // signals and slots connections
    connect( pButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
}
