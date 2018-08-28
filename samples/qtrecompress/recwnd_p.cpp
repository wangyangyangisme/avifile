#include "recwnd_p.h"
#include "recwnd_p.moc"

#include <qgroupbox.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qprogressbar.h>
#include <qpushbutton.h>
#include <qtabwidget.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

class MainTabWidget: public QWidget
{
public:
    MainTabWidget(RecWnd_p& rec, QWidget* parent) : QWidget(parent)
    {
	static const char* labels[] =
	{
	    "Current video frame:",
	    "Current audio sample:",
	    "Video data:",
	    "Audio data:",
	    "Current file size:",
	    "Projected file size:",
	    "Video rendering rate:",
	    "Time elapsed:",
	    "Total time (estimated):"
	};

	QGridLayout* qgbl = new QGridLayout( this, 1, 1 );
	qgbl->setMargin(5);
	qgbl->setSpacing(5);

	int i = -1;
        QLabel* l;
	while (++i < RecWnd_p::LAST_LABEL)
	{
	    l = new QLabel( this );
	    l->setText( tr( labels[i] ));
	    qgbl->addWidget( l, i, 0 );

	    l = rec.m_pText[i] = new QLabel( this );
	    l->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
	    l->setLineWidth( 1 );
	    l->setIndent( 5 );
	    l->setAlignment( int( QLabel::AlignVCenter | QLabel::AlignRight ) );
	    l->setMinimumWidth( 180 );
	    qgbl->addWidget( l, i, 1 );
	}
    }
};

/*
 *  Constructs a RecWnd_p which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
RecWnd_p::RecWnd_p( QWidget* parent,  const char* name, bool modal, WFlags fl )
    : QDialog( parent, name, modal, fl )
{
    setCaption( tr( "Recompression progress"  ) );
#if QT_VERSION > 220
    setSizeGripEnabled( TRUE );
#endif

    QVBoxLayout* vbl = new QVBoxLayout( this );
    vbl->setMargin(5);
    vbl->setSpacing(5);

    m_pTabWidget = new QTabWidget( this );

    m_pTabWidget->insertTab(new MainTabWidget(*this, m_pTabWidget),
			    tr( "&Main" ) );

    m_pTabWidget->insertTab(new QWidget(m_pTabWidget), tr( "&Video" ) );

    vbl->addWidget( m_pTabWidget );

    QHBoxLayout* hbl = new QHBoxLayout( vbl );

    QLabel* l = new QLabel( this );
    l->setText( tr( "Progress:" ) );
    hbl->addWidget( l );

    m_pProgress = new QProgressBar( this );
    m_pProgress->setProgress( -1 );
    m_pProgress->setTotalSteps( 1000 );
    m_pProgress->setFrameShadow( QProgressBar::Sunken );
    m_pProgress->setFrameShape( QProgressBar::StyledPanel );
    m_pProgress->setIndicatorFollowsStyle( TRUE );
    hbl->addWidget( m_pProgress );

    hbl = new QHBoxLayout( vbl );
    hbl->addStretch( 1 );
    m_pPause = new QPushButton( this );
    m_pPause->setText( tr( "&Pause" ) );
    hbl->addWidget( m_pPause );

    m_pStop = new QPushButton( this );
    m_pStop->setText( tr( "&Stop" ) );
    hbl->addWidget( m_pStop );

    // signals and slots connections
    connect( m_pPause, SIGNAL( clicked() ), this, SLOT( pauseProcess() ) );
    connect( m_pStop, SIGNAL( clicked() ), this, SLOT( cancelProcess() ) );
    connect( m_pTabWidget, SIGNAL( currentChanged( QWidget* ) ), this,
	     SLOT( currentChanged( QWidget* ) ) );
}

/*
 *  Destroys the object and frees any allocated resources
 */
RecWnd_p::~RecWnd_p()
{
    // no need to delete child widgets, Qt does it all for us
}

void RecWnd_p::cancelProcess()
{
    qWarning( "RecWnd_p::cancelProcess(): Not implemented yet!" );
}

void RecWnd_p::pauseProcess()
{
    qWarning( "RecWnd_p::pauseProcess(): Not implemented yet!" );
}

void RecWnd_p::currentChanged(QWidget* w)
{
    qWarning( "RecWnd_p::currentWidget(): Not implemented yet!" );
}
