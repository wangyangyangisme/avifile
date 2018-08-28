#include "genctl_p.h"
#include "genctl_p.moc"

#include <qbuttongroup.h>
#include <qvbuttongroup.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qhgroupbox.h>
#include <qvgroupbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qlistbox.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qmenubar.h>
#include <qmainwindow.h>
#include <qvbox.h>
#include <qhbox.h>

#if QT_VERSION <= 220
#define LAYOUTCOL 1
#else
#define LAYOUTCOL 0
#endif

/* 
 *  Constructs a QtRecompressor which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 */
QtRecompressor::QtRecompressor( QWidget* parent,  const char* name, WFlags fl )
    : QMainWindow( parent, name, fl )
{
    QLabel* lb;
    QGridLayout* gl;
    QHBox* hb;
    QVBox* vb;

    if ( !name )
	setName( "AviRecompressor" );

    setCaption( tr( "AviRecompressor" ) );
    //setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)0, (QSizePolicy::SizeType)0, sizePolicy().hasHeightForWidth() ) );

    QPopupMenu *file = new QPopupMenu( this );
    CHECK_PTR( file );

    file->insertItem( "&Open video file",  this, SLOT( fileOpenVideo() ), CTRL+Key_O );
    file->insertItem( "&Append video file",  this, SLOT( fileAppendVideo() ), CTRL+Key_A );
    file->insertItem( "&Save AVI As",  this, SLOT( fileSaveAvi() ), CTRL+Key_S );
    //file->insertItem( "Save WAV ",  this, SLOT( fileSaveAvi() ) );
    //file->insertItem( "Save audio ",  this, SLOT( fileSaveAvi() ) );
    file->insertSeparator();
    file->insertItem( "&Read Config",  this, SLOT( fileLoadConfig() ), CTRL+Key_R );
    file->insertItem( "&Write Config",  this, SLOT( fileSaveConfig() ), CTRL+Key_W );
    file->insertSeparator();
    file->insertItem( "&Quit",  this, SLOT( fileQuit() ), CTRL+Key_Q );

    QPopupMenu *edit = new QPopupMenu( this );
    CHECK_PTR( edit );

    QPopupMenu *video = new QPopupMenu( this );
    CHECK_PTR( video );
    video->insertItem( "&Filters",  this, SLOT( videoFilters() ), CTRL+Key_F );

    QPopupMenu *audio = new QPopupMenu( this );
    CHECK_PTR( audio );

    QPopupMenu *options = new QPopupMenu( this );
    CHECK_PTR( options );
    options->insertItem( "&Preferences",  this, SLOT( optionsPreferences() ) );

    QPopupMenu *help = new QPopupMenu( this );
    CHECK_PTR( help );
    help->insertItem( "&Contents",  this, SLOT( helpContents() ), CTRL+Key_H );
    help->insertSeparator();
    help->insertItem( "Changelog",  this, SLOT( helpChangelog() ) );

    QMenuBar* menu = menuBar(); // new QMenuBar( this );
    statusBar();

    CHECK_PTR( menu );
    menu->insertItem( "&File", file );
    menu->insertItem( "&Edit", edit );
    menu->insertItem( "&Video", video );
    menu->insertItem( "&Audio", audio );
    menu->insertItem( "&Options", options );
    menu->insertSeparator();
    menu->insertItem( "&Help", help );
    menu->setSeparator( QMenuBar::InWindowsStyle );

    //QToolBar* qtb = new QToolBar( this );
    //centralWidget()
    QVBox* bg = new QVBox( this );
    bg->setMargin(10);
    setCentralWidget(bg);

    QGroupBox* g = new QGroupBox( tr( "Source and destination" ), bg );
    g->setColumnLayout( LAYOUTCOL, Qt::Vertical );

    gl = new QGridLayout( g->layout(), 1, 1 );
    gl->setSpacing( 5 );

    m_pButtonSrc = new QPushButton( tr( "&Source:" ), g );
    //m_pButtonSrc->setAlignment( AlignRight | AlignVCenter );
    gl->addWidget( m_pButtonSrc, 0, 0 );

    m_pButtonDst = new QPushButton( tr( "&Destination:" ), g );
    //m_pButtonDst->setAlignment( AlignRight | AlignVCenter );
    gl->addWidget( m_pButtonDst, 1, 0 );

    m_pButtonCfg = new QPushButton( tr( "Configuration:" ), g );
    //m_pButtonCfg->setAlignment( AlignRight | AlignVCenter );
    gl->addWidget( m_pButtonCfg, 2, 0 );

    m_pLabelSrc = new QLabel( g );
    m_pLabelSrc->setFrameShadow( QLabel::Sunken );
    m_pLabelSrc->setFrameShape( QLabel::Box );
    gl->addWidget( m_pLabelSrc, 0, 1 );

    m_pLabelDst = new QLabel( g );
    m_pLabelDst->setFrameShadow( QLabel::Sunken );
    m_pLabelDst->setFrameShape( QLabel::Box );
    gl->addWidget( m_pLabelDst, 1, 1 );

    m_pLabelConfig = new QLabel( g );
    m_pLabelConfig->setFrameShadow( QLabel::Sunken );
    m_pLabelConfig->setFrameShape( QLabel::Box );
    gl->addWidget( m_pLabelConfig, 2, 1 );

    m_pButtonStart = new QPushButton( tr( "&Go!" ), g );

    gl->addMultiCellWidget( m_pButtonStart, 0, 2, 2, 2 );
    gl->setColStretch( 1, 1 );


    g = new QGroupBox( tr( "Recompress options" ), bg );
    g->setMargin( 5 );
    g->setFrameShadow( QGroupBox::Sunken );
    g->setFrameShape( QGroupBox::Box );
    g->setColumnLayout( LAYOUTCOL , Qt::Vertical );
    gl = new QGridLayout( g->layout(), 1, 1 );

    hb = new QHBox( g );
    hb->setSpacing( 5 );
    gl->addMultiCellWidget( hb, 0, 0, 0, 1 );

    lb = new QLabel( tr( "Stream:" ), hb );

    m_pComboStreams = new QComboBox( FALSE, hb );

    m_pButtonStreamDetails = new QPushButton( tr( "Details..." ), hb );

    m_pButtonFormat = new QPushButton( tr( "Format..." ), hb );
    m_pButtonFormat->setEnabled( FALSE );

    ///////////

    m_pButtonGroupLimits = new QVButtonGroup( g );
    m_pButtonGroupLimits->setLineWidth( 0 );

    m_pRadioSeconds = new QRadioButton( tr( "seconds" ), m_pButtonGroupLimits );
    m_pRadioFrames = new QRadioButton( tr( "frames" ), m_pButtonGroupLimits );

    gl->addWidget( m_pButtonGroupLimits, 1, 0 );


    QWidget* Layout27 = new QWidget( g );
    gl->addWidget( Layout27, 1, 1, 1 );

    QGridLayout* sgl = new QGridLayout( Layout27, 1, 1 );
    sgl->setSpacing( 5 );
    sgl->setMargin( 0 );

    // first row
    m_pLineLowSec = new QLineEdit( Layout27 );
    sgl->addWidget( m_pLineLowSec, 0, 0 );

    lb = new QLabel( tr( "to" ), Layout27 );
    sgl->addWidget( lb, 0, 1 );

    m_pLineHighSec = new QLineEdit( Layout27 );
    sgl->addWidget( m_pLineHighSec, 0, 2 );

    // second row
    m_pLineLowFrames = new QLineEdit( Layout27 );
    sgl->addWidget( m_pLineLowFrames, 1, 0 );

    lb = new QLabel( tr( "to" ), Layout27 );
    sgl->addWidget( lb, 1, 1 );

    m_pLineHighFrames = new QLineEdit( Layout27 );
    sgl->addWidget( m_pLineHighFrames, 1, 2 );

    /////////////

    m_pButtonGroupStreamMode = new QVButtonGroup( g );
    m_pButtonGroupStreamMode->setTitle( tr( ""  ) );
    m_pButtonGroupStreamMode->setLineWidth( 0 );

    gl->addMultiCellWidget( m_pButtonGroupStreamMode, 0, 1, 2, 2 );

    m_pRadioRemove = new QRadioButton( tr( "Remove" ), m_pButtonGroupStreamMode );
    m_pRadioCopy = new QRadioButton( tr( "Copy" ), m_pButtonGroupStreamMode );
    m_pRadioRecompress = new QRadioButton( tr( "Recompress" ), m_pButtonGroupStreamMode );

    /////////////

    g = new QGroupBox( bg );
    g->setTitle( tr( "Filters" ) );
    g->setColumnLayout( LAYOUTCOL, Qt::Vertical );
    gl = new QGridLayout( g->layout(), 1, 1 );

    gl->setSpacing( 5 );

    gl->setColStretch( 0, 4 );
    gl->setColStretch( 2, 2 );

    _lbFilters = new QListBox( g );
    _lbFilters->insertItem( tr( "New Item" ) );

    gl->addMultiCellWidget( _lbFilters, 0, 1, 0, 0 );

    vb = new QVBox( g );
    vb->setSpacing( 5 );

    gl->addMultiCellWidget( vb, 0, 1, 1, 1 );

    m_pButtonConfigFilter = new QPushButton( tr( "Configure" ), vb );
    m_pButtonAddFilter = new QPushButton( tr( "Add" ), vb );
    m_pButtonRemoveFilter = new QPushButton( tr( "Remove" ), vb );
    m_pButtonUpFilter = new QPushButton( tr( "Up" ), vb );
    m_pButtonUpFilter->setAutoRepeat( TRUE );
    m_pButtonDownFilter = new QPushButton( tr( "Down" ) , vb );
    m_pButtonDownFilter->setAutoRepeat( TRUE );


    hb = new QHBox( g );
    hb->setSpacing( 5 );
    m_pButtonAboutFilter = new QPushButton( tr( "About" ), hb );

    lb = new QLabel( tr( "Available filters" ), hb );
    lb->setAlignment( AlignRight | AlignVCenter );

    gl->addWidget( hb, 0, 2 );

    hb = new QHBox();

    m_pListAllFilters = new QListBox( g );
    m_pListAllFilters->insertItem( tr( "New Item" ) );

    gl->addWidget( m_pListAllFilters, 1, 2 );

    // signals and slots connections
    connect( m_pButtonStreamDetails, SIGNAL( clicked() ), this, SLOT( detailsClicked() ) );
    connect( m_pButtonAboutFilter, SIGNAL( clicked() ), this, SLOT( aboutFilterClicked() ) );
    connect( m_pButtonAddFilter, SIGNAL( clicked() ), this, SLOT( addFilterClicked() ) );
    connect( m_pButtonRemoveFilter, SIGNAL( clicked() ), this, SLOT( removeFilterClicked() ) );
    connect( m_pButtonConfigFilter, SIGNAL( clicked() ), this, SLOT( configFilterClicked() ) );
    connect( m_pButtonUpFilter, SIGNAL( clicked() ), this, SLOT( upFilterSelected() ) );
    connect( m_pButtonDownFilter, SIGNAL( clicked() ), this, SLOT( downFilterClicked() ) );
    connect( m_pButtonGroupStreamMode, SIGNAL( clicked(int) ), this, SLOT( streamModeChanged(int) ) );
    connect( m_pButtonGroupLimits, SIGNAL( clicked(int) ), this, SLOT( streamLimitModeChanged(int) ) );
    connect( m_pButtonFormat, SIGNAL( clicked() ), this, SLOT( streamFormatClicked() ) );
    connect( m_pComboStreams, SIGNAL( activated(int) ), this, SLOT( streamSelected(int) ) );
    connect( m_pButtonStart, SIGNAL( clicked() ), this, SLOT( startRecompress() ) );

    connect( m_pButtonSrc, SIGNAL( clicked() ), this, SLOT( fileOpenVideo() ) );
    connect( m_pButtonDst, SIGNAL( clicked() ), this, SLOT( fileSaveAvi() ) );
    connect( m_pButtonCfg, SIGNAL( clicked() ), this, SLOT( fileLoadConfig() ) );
}

/*  
 *  Destroys the object and frees any allocated resources
 */
QtRecompressor::~QtRecompressor()
{
    // no need to delete child widgets, Qt does it all for us
}
