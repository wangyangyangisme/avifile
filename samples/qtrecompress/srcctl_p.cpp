#include "srcctl_p.h"
#include "srcctl_p.moc"

#include <qlabel.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qtoolbutton.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>


/* 
 *  Constructs a SourceControl which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
SourceControlDialog::SourceControlDialog( QWidget* parent,  const char* name, bool modal, WFlags fl )
    : QDialog( parent, name, modal, fl )
{
    if ( !name )
	setName( "SourceControl" );
    setCaption( tr( "Source control" ) );

    QGridLayout* gl = new QGridLayout( this, 1, 1 );
    gl->setSpacing( 5 );
    gl->setMargin( 5 );

    m_pSlider = new QSlider( this );
    m_pSlider->setOrientation( QSlider::Horizontal );
    m_pSlider->setTickmarks( QSlider::Right);

    gl->addMultiCellWidget( m_pSlider, 0, 0, 0, 9 );

    m_pButtonPF = new QToolButton( this );
    m_pButtonPF->setAutoRepeat( TRUE );
    m_pButtonPF->setText( tr( "<" ) );
    gl->addWidget( m_pButtonPF, 1, 0 );

    m_pButtonNF = new QToolButton( this );
    m_pButtonNF->setAutoRepeat( TRUE );
    m_pButtonNF->setText( tr( ">" ) );
    gl->addWidget( m_pButtonNF, 1, 1 );


    m_pButtonPFF = new QToolButton( this );
    m_pButtonPFF->setText( tr( "<<" ) );
    gl->addWidget( m_pButtonPFF, 1, 2 );

    m_pButtonNFF = new QToolButton( this );
    m_pButtonNFF->setAutoRepeat( TRUE );
    m_pButtonNFF->setText( tr( ">>" ) );
    gl->addWidget( m_pButtonNFF, 1, 3 );


    m_pButtonPTF = new QToolButton( this );
    m_pButtonPTF->setText( tr( "<<<" ) );
    gl->addWidget( m_pButtonPTF, 1, 4 );

    m_pButtonNTF = new QToolButton( this );
    m_pButtonNTF->setAutoRepeat( TRUE );
    m_pButtonNTF->setText( tr( ">>>" ) );
    gl->addWidget( m_pButtonNTF, 1, 5 );


    m_pButtonPKF = new QToolButton( this );
    m_pButtonPKF->setAutoRepeat( TRUE );
    m_pButtonPKF->setText( tr( "KF<" ) );
    gl->addWidget( m_pButtonPKF, 1, 6 );

    m_pButtonNKF = new QToolButton( this );
    m_pButtonNKF->setAutoRepeat( TRUE );
    m_pButtonNKF->setText( tr( ">KF" ) );
    gl->addWidget( m_pButtonNKF, 1, 7 );

    gl->setColStretch(8, 1);

    m_pLabel = new QLabel( this );
    gl->addWidget( m_pLabel, 1, 9 );

    //gl->setRowStretch(2, 1);

    // signals and slots connections
    connect( m_pSlider, SIGNAL( sliderMoved( int ) ), this, SLOT( sliderMoved( int ) ) );
#if QT_VERSION > 220
//    connect( m_pSlider, SIGNAL( sliderReleased() ), this, SLOT( sliderReleased() ) );
//    connect( m_pSlider, SIGNAL( valueChanged( int ) ), this, SLOT( valueChanged( int ) ) );
#endif
    connect( m_pButtonPF, SIGNAL( clicked() ), this, SLOT( prevFrame() ) );
    connect( m_pButtonPFF, SIGNAL( clicked() ), this, SLOT( prevFiveFrames() ) );
    connect( m_pButtonPTF, SIGNAL( clicked() ), this, SLOT( prevTwentyFrames() ) );
    connect( m_pButtonPKF, SIGNAL( clicked() ), this, SLOT( prevKeyFrame() ) );

    connect( m_pButtonNF, SIGNAL( clicked() ), this, SLOT( nextFrame() ) );
    connect( m_pButtonNFF, SIGNAL( clicked() ), this, SLOT( nextFiveFrames() ) );
    connect( m_pButtonNTF, SIGNAL( clicked() ), this, SLOT( nextTwentyFrames() ) );
    connect( m_pButtonNKF, SIGNAL( clicked() ), this, SLOT( nextKeyFrame() ) );
}

/*  
 *  Destroys the object and frees any allocated resources
 */
SourceControlDialog::~SourceControlDialog()
{
    // no need to delete child widgets, Qt does it all for us
}
