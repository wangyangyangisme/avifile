#include "configdialog.h"
#include "configdialog.moc"

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qlistbox.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qtabwidget.h>
#include <qtooltip.h>
#include <qvalidator.h>
#include <qwhatsthis.h>

#if QT_VERSION > 300
#include <qstylefactory.h>
#endif


#include <stdio.h>

const unsigned int ConfigDialog::frequencyList[] =
{
    48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 0
};

#if QT_VERSION <= 220
#define LAYOUTCOL 1
#else
#define LAYOUTCOL 0
#endif


/*
 *  Constructs a ConfigDialog which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
ConfigDialog::ConfigDialog( QWidget* parent,  const char* name, bool modal, WFlags fl )
    : QavmOkDialog( parent, name, modal, fl ),
    m_pNoDefaultTheme(0), m_pThemeList(0)
{
    setCaption( tr( "Configuration" ) );
#if QT_VERSION > 220
    setSizeGripEnabled( TRUE );
#endif
#if 0
    QFont f( font() );
    f.setFamily( "adobe-helvetica" );
    f.setPointSize( 15 );
    setFont( f );
#endif
    QTabWidget* tabWidget = new QTabWidget( this );
    // ok we don't need this now tabWidget->setTabShape(QTabWidget::Rounded);
    tabWidget->insertTab( createVideo(tabWidget), tr( "&Video" ) );
    tabWidget->insertTab( createAudio(tabWidget), tr( "&Audio" ) );
    tabWidget->insertTab( createMisc(tabWidget),  tr( "&Misc" ) );
    tabWidget->insertTab( createSync(tabWidget),  tr( "&Sync" ) );
    tabWidget->insertTab( createSubtitles(tabWidget), tr( "S&ubtitles" ) );
    tabWidget->insertTab( createDecoder(tabWidget), tr( "&Decoder" ) );

    setApplyEnabled( TRUE );
    setOkDefault( FALSE );
    gridLayout()->addWidget( tabWidget, 0, 0 );
}

QWidget* ConfigDialog::createSubtitles(QWidget* parent)
{
    QWidget* w = new QWidget(parent);
    QVBoxLayout* vl = new QVBoxLayout( w );
    vl->setMargin( 5 );
    vl->setSpacing( 5 );

    QGridLayout* gl = new QGridLayout( vl, 1, 1 );
    gl->setSpacing( 5 );

    m_pSubFGColor = new QPushButton( tr( " " ), w );
    gl->addWidget( m_pSubFGColor, 0, 0 );
    QLabel* l = new QLabel( tr( "Foreground" ), w );
    gl->addWidget( l, 0, 1 );
    m_pSubBGColor = new QPushButton( tr( " " ), w );
    gl->addWidget( m_pSubBGColor, 1, 0 );
    l = new QLabel( tr( "Background" ), w );
    gl->addWidget( l, 1, 1 );

    QGroupBox* gb = new QGroupBox( LAYOUTCOL, Qt::Horizontal,
				  tr( "Subtitle font" ), w );
    vl->addWidget( gb );

    gl = new QGridLayout( gb->layout(), 1, 1 );
    gl->setSpacing( 5 );

    m_pFontButton = new QPushButton( tr( "Select font..." ), gb );
    gl->addWidget(m_pFontButton, 0, 0);

    m_pFontType = new QLabel( gb );
    m_pFontType->setAlignment( QLabel::AlignCenter );
    gl->addWidget(m_pFontType, 0, 1);

    m_pFontName = new QLineEdit( gb );
    gl->addMultiCellWidget( m_pFontName, 1, 1, 0, 2 );

    gl->setColStretch( 2, 1 );

    m_pPreview = new QLabel( tr( "Quick brown fox jumps over the lazy dog." ), gb );
    m_pPreview->setAlignment( QLabel::AlignCenter );
#if QT_VERSION > 220
    m_pPreview->setFrameShadow( QLabel::Sunken );
    m_pPreview->setFrameShape( QLabel::Panel );
#endif
    gl->addMultiCellWidget( m_pPreview, 2, 2, 0, 2 );

    return w;
}


class DefAudioGroupBox: public QGroupBox
{
public:
    DefAudioGroupBox( ConfigDialog& m, QWidget* parent ) : QGroupBox( parent )
    {
	setTitle( tr( "Default audio stream" ) );
	setColumnLayout( LAYOUTCOL, Qt::Horizontal );

	QHBoxLayout* hl = new QHBoxLayout( layout() );
	QLabel* l1 = new QLabel( tr( "Default played audio stream is # " ), this );
	hl->addWidget( l1 );

	m.m_pDefAudio = new QSpinBox( 0, 127, 1, this );
	hl->addWidget( m.m_pDefAudio );
    }
};

class HttpGroupBox: public QGroupBox
{
public:
    HttpGroupBox( ConfigDialog& m, QWidget* parent ) : QGroupBox( parent )
    {
	setTitle( tr( "HTTP proxy" ) );
	setColumnLayout( LAYOUTCOL, Qt::Horizontal );

	QHBoxLayout* httpb = new QHBoxLayout( layout() );

	httpb->setMargin(15);

	m.m_pUseProxy = new QCheckBox( tr( "Use proxy" ), this );
	QFont UseProxy_font(  m.m_pUseProxy->font() );
	m.m_pUseProxy->setFont( UseProxy_font );
	httpb->addWidget( m.m_pUseProxy );

	m.m_pProxyName = new QLineEdit( this );
	//m_pProxyName->setMinimumSize( m_pProxyName->minimumSizeHint() );
	//QFontInfo finfo( f );
	//ProxyName->setFixedHeight ( 2 * finfo.pointSize() );

	m.m_pProxyName->setEnabled( FALSE );
	//ProxyName->setEnabled( TRUE );
	httpb->addWidget( m.m_pProxyName );

    }
};

class RenderingModeGroupBox: public QGroupBox
{
public:
    RenderingModeGroupBox( ConfigDialog& m, QWidget* parent ) : QGroupBox( parent )
    {
	//setTitle( tr( "Rendering mode" ) );
	setColumnLayout( LAYOUTCOL, Qt::Horizontal );

	QVBoxLayout* v = new QVBoxLayout( layout() );
	m.m_pHwaccel = new QCheckBox( tr( "Use YUV overlay if available (hw accelerated or sw emulation)" ), this );
	m.m_pQualityAuto = new QCheckBox( tr( "Set CPU quality automagicaly (needs buffering)" ), this );
	m.m_pVideoDirect = new QCheckBox( tr( "Use direct rendering if possible (scaling disabled without hw accel)" ), this );
	m.m_pVideoBuffered = new QCheckBox( tr( "Buffer frames ahead - smoother video, but more CPU intensive" ), this );
	m.m_pVideoDropping = new QCheckBox( tr( "Dropping frames" ), this );
	m.m_pPreserveAspect = new QCheckBox( tr( "Preserve video aspect ratio" ), this );

        v->addWidget( m.m_pHwaccel );
        v->addWidget( m.m_pQualityAuto );
        v->addWidget( m.m_pVideoDirect );
        v->addWidget( m.m_pVideoBuffered );
        v->addWidget( m.m_pVideoDropping );
        v->addWidget( m.m_pPreserveAspect );
    }
};

class ResamplingGroupBox: public QGroupBox
{
public:
    ResamplingGroupBox( ConfigDialog& m, QWidget* parent ) : QGroupBox( parent )
    {
	setTitle( tr( "Audio resampling" ) );
	setColumnLayout( LAYOUTCOL, Qt::Horizontal );

	QGridLayout* gl = new QGridLayout( layout(), 1, 1 );
	gl->setSpacing(5);

	m.m_pAudioResampling = new QCheckBox( tr( "&Enable sound resampling" ), this );
	gl->addWidget(m.m_pAudioResampling, 1, 0);

	//m.m_pAudioResamplingRate = new QSpinBox( this, "AudioResamplingRate" );
	m.m_pAudioResamplingRate = new QComboBox( this );
#if QT_VERSION > 220
	m.m_pAudioResamplingRate->setEditable( true );
#endif
        // !!!!FIXME - leak - destroy later
	QValidator* qv = new QIntValidator( 1000, 128000, m.m_pAudioResamplingRate );
	m.m_pAudioResamplingRate->setValidator( qv );

	gl->addWidget(m.m_pAudioResamplingRate, 2, 0);

	QLabel* l = new QLabel( tr( "Resampling frequency " ), this );
	gl->addWidget( l, 2, 1 );

	m.m_pAudioPlayingRate = new QComboBox( this );
	for (unsigned int i = 0; m.frequencyList[i]; i++)
	{
	    m.m_pAudioResamplingRate->insertItem(QString::number(m.frequencyList[i]), i);
	    m.m_pAudioPlayingRate->insertItem(QString::number(m.frequencyList[i]), i);
	}

	gl->addWidget(m.m_pAudioPlayingRate, 3, 0);

	l = new QLabel( tr( "Playing frequency " ), this );
	gl->addWidget( l, 3, 1 );
	gl->setColStretch( 10, 1 );

/*
	m.m_pUseProxy = new QCheckBox( this, "UseProxy" );
	m.m_pUseProxy->setText( tr( "Use proxy" ) );
	QFont UseProxy_font(  m.m_pUseProxy->font() );
	m.m_pUseProxy->setFont( UseProxy_font );
	httpb->addWidget( m.m_pUseProxy );

	m.m_pProxyName = new QLineEdit( this, "ProxyName" );
	m.m_pProxyName->setEnabled( FALSE );
	httpb->addWidget( m.m_pProxyName );
*/
    }
};

#if QT_VERSION >= 300
class ThemeGroupBox: public QGroupBox
{
public:
    ThemeGroupBox( ConfigDialog& m, QWidget* parent ) : QGroupBox( parent )
    {
	setTitle( tr( "Theme selector" ) );
	setColumnLayout( LAYOUTCOL, Qt::Horizontal );

	QGridLayout* gl = new QGridLayout( layout(), 1, 1 );
	gl->setSpacing(10);

        m.m_pNoDefaultTheme = new QCheckBox( tr( "&Use theme" ), this );
	gl->addWidget( m.m_pNoDefaultTheme, 0, 0 );

        m.m_pThemeList = new QComboBox( this );

        QStringList list = QStyleFactory::keys();
        list.sort();
        m.m_pThemeList->insertStringList( list );
	m.m_pThemeList->setEnabled( FALSE );

	gl->addWidget( m.m_pThemeList, 0, 1 );
    }
};
#endif

class DecoderGroupBox: public QGroupBox
{
public:
    DecoderGroupBox( ConfigDialog& m, QWidget* parent ) : QGroupBox( parent )
    {
	//setTitle( tr( "Rendering mode" ) );
	setColumnLayout( LAYOUTCOL, Qt::Horizontal );

	QHBoxLayout* vl = new QHBoxLayout( layout() );

	m.m_pVideoList = new QListBox( this );
	m.m_pVideoList->insertItem( tr( "New Item" ) );

	vl->addWidget(m.m_pVideoList);

	m.m_pAudioList = new QListBox( this );
	m.m_pAudioList->insertItem( tr( "New Item" ) );

        vl->addWidget(m.m_pAudioList);
    }
};

QWidget* ConfigDialog::createMisc(QWidget* parent)
{
    QWidget* w = new QWidget( parent );

    QGridLayout* gl = new QGridLayout( w, 1, 1 );
    gl->setMargin(5);
    gl->setSpacing(5);

    QGroupBox* gb2 = new HttpGroupBox( *this, w  );
    gl->addWidget(gb2, 0, 0);

    // Miscellaneous
    QGroupBox* gb3 = new QGroupBox( LAYOUTCOL, Qt::Horizontal,
				    tr( "Miscellaneous" ), w );

    QVBoxLayout* vl = new QVBoxLayout( gb3->layout() );
    vl->setMargin(5);

    m_pAutorepeat = new QCheckBox( tr( "Auto&replay - restart video when finished" ), gb3 );
    vl->addWidget(m_pAutorepeat);

    m_pDisplayFramePos = new QCheckBox( tr( "D&isplay frame position" ), gb3 );
    vl->addWidget(m_pDisplayFramePos);

    gl->addWidget(gb3, 1, 0);

#if QT_VERSION >= 300
    QGroupBox* t = new ThemeGroupBox( *this, w );
    gl->addWidget(t, 2, 0);
#endif

    gl->setColStretch(10, 1);
    gl->setRowStretch(10, 1);

    return w;
}

QWidget* ConfigDialog::createAudio(QWidget* parent)
{
    QWidget* w = new QWidget(parent);

    QGridLayout* gl = new QGridLayout( w, 1, 1 );
    gl->setMargin(5);
    gl->setSpacing(5);

    QGroupBox* gb1 = new DefAudioGroupBox( *this, w );
    gl->addWidget(gb1, 0, 0);

    QGroupBox* gb2 = new ResamplingGroupBox( *this, w );
    gl->addWidget(gb2, 1, 0);

    gl->setColStretch(10, 1);
    gl->setRowStretch(10, 1);

    return w;
}

QWidget* ConfigDialog::createVideo(QWidget* parent)
{
    QWidget* w = new QWidget(parent);

    QGridLayout* gl = new QGridLayout( w, 1, 1 );
    gl->setMargin(5);
    gl->setSpacing(5);

    QGroupBox* gb = new RenderingModeGroupBox( *this, w );
    gl->addWidget(gb, 0, 0);

    return w;
}

QWidget* ConfigDialog::createDecoder(QWidget* parent)
{
    QWidget* w = new QWidget( parent );

    QGridLayout* gl = new QGridLayout( w, 1, 1 );
    gl->setMargin( 5 );
    gl->setSpacing( 5 );

    QGroupBox* gb = new DecoderGroupBox( *this, w );
    gl->addWidget( gb, 0, 0 );

    return w;
}

QWidget* ConfigDialog::createSync(QWidget* parent)
{
    QWidget* w = new QWidget( parent );

    QGridLayout* gl = new QGridLayout( w, 1, 1 );
    gl->setMargin( 5 );
    gl->setSpacing( 5 );

    QGroupBox* gb = new QGroupBox( tr( "Audio/video synch adjustment" ), w );
    gb->setColumnLayout( LAYOUTCOL, Qt::Horizontal );

    gl->addWidget( gb, 0, 0 );

    QHBoxLayout* hl1 = new QHBoxLayout( gb->layout() );

    m_pAsyncSlider = new QSlider( -1000, 1000, 10, 0, QSlider::Horizontal, gb );
    m_pAsyncSlider->setTickmarks( QSlider::Right );
    m_pAsyncSlider->setTickInterval( 60 );
    hl1->addWidget(m_pAsyncSlider);
    hl1->setStretchFactor(m_pAsyncSlider, 16);

    m_pAsync = new QLabel( gb );
    m_pAsync->setAlignment( QLabel::AlignCenter );
    hl1->addWidget(m_pAsync);
    hl1->setStretchFactor(m_pAsync, 3);

    QGroupBox* sgb = new QGroupBox( tr( "Subtitle synch adjustment" ), w );
    sgb->setColumnLayout( LAYOUTCOL, Qt::Horizontal );

    gl->addWidget( sgb, 1, 0 );

    QHBoxLayout* hl2 = new QHBoxLayout( sgb->layout() );
    hl2->setSpacing( 5 );

    m_pSubNegative = new QCheckBox( tr( "Negative" ), sgb );
    hl2->addWidget( m_pSubNegative );

    m_pSubAsyncMin = new QSpinBox( 0, 999, 1, sgb );
    hl2->addWidget( m_pSubAsyncMin );

    QLabel* l = new QLabel( tr( "minutes" ), sgb );
    l->setAlignment( QLabel::AlignCenter );
    hl2->addWidget( l );

    m_pSubAsyncSec = new QSpinBox( 0, 59, 1, sgb );
    hl2->addWidget( m_pSubAsyncSec );

    l = new QLabel( tr( "seconds" ), sgb );
    l->setAlignment( QLabel::AlignCenter );
    hl2->addWidget( l );

    gl->setRowStretch(2, 1);
    gl->setColStretch(1, 1);
    return w;
}

/*
 *  Main event handler. Reimplemented to handle application
 *  font changes
 */
bool ConfigDialog::event( QEvent* ev )
{
    bool ret = QDialog::event( ev );
#if QT_VERSION > 220
    if ( ev->type() == QEvent::ApplicationFontChange ) {
	QFont CheckBox1_font(  m_pSubNegative->font() );
	m_pSubNegative->setFont( CheckBox1_font );
	QFont UseProxy_font(  m_pUseProxy->font() );
	m_pUseProxy->setFont( UseProxy_font );
    }
#endif
    return ret;
}
