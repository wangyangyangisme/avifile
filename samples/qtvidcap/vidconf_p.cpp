#include "vidconf_p.h"
#include "vidconf_p.moc"

#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qvgroupbox.h>
#include <qhgroupbox.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qtabwidget.h>
#include <qwidget.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qvalidator.h>
#include <qwhatsthis.h>
#include <qradiobutton.h>
#include <qvbuttongroup.h>
#include <qlistview.h>

/*
 *  Constructs a VidcapConfigDialog which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
VidcapConfigDialog::VidcapConfigDialog( QWidget* parent,  const char* name, bool modal, WFlags fl )
    : QavmOkDialog( parent, name, modal, fl )
{
    setCaption( tr( "Avicap configuration" ) );
#if QT_VERSION > 220
    setSizeGripEnabled( TRUE );
#endif

    tabWidget = new QTabWidget( this );

    QWidget* w = new QWidget( tabWidget );
    QGridLayout* gl = new QGridLayout( w, 1, 1 );
    gl->setSpacing( 5 );
    gl->setMargin( 5 );

    // capture device
    int row = 0;
    QLabel* l = new QLabel( tr( "BTTV capture device:" ), w );
    l->setAlignment( QLabel::AlignRight | QLabel::AlignVCenter );
    gl->addWidget( l, row, 0 );

    _CapDevice = new QLineEdit( w );
    QWhatsThis::add(  _CapDevice, tr( "Do not change default value ( /dev/video ) unless you exactly know what you're doing." ) );
    gl->addWidget( _CapDevice, row, 1 );

    // audio device
    row++;
    l = new QLabel( tr( "Audio device:" ), w );
    l->setAlignment( QLabel::AlignRight | QLabel::AlignVCenter );
    gl->addWidget( l, row, 0 );

    _AudDevice = new QLineEdit( w );
    QWhatsThis::add(  _AudDevice, tr( "Do not change default value ( /dev/dsp ) unless you exactly know what you're doing." ) );
    gl->addWidget( _AudDevice, row, 1 );

    // channel
    row++;
    l = new QLabel( tr( "Video channel:" ), w );
    l->setAlignment( QLabel::AlignRight | QLabel::AlignVCenter );
    gl->addWidget( l, row, 0 );

    _VideoChannel = new QComboBox( FALSE, w );
    gl->addWidget( _VideoChannel, row, 1 );
    tabWidget->insertTab( w, tr( "&Device" ) );

    // color mode
    row++;
    l = new QLabel( tr( "Color mode:" ), w );
    l->setAlignment( QLabel::AlignRight | QLabel::AlignVCenter );
    gl->addWidget( l, row, 0 );

    _VideoColorMode = new QComboBox( FALSE, w );
    _VideoColorMode->insertItem( tr( "PAL" ) );
    _VideoColorMode->insertItem( tr( "NTSC" ) );
    _VideoColorMode->insertItem( tr( "SECAM" ) );
    _VideoColorMode->insertItem( tr( "Auto" ) );
    gl->addWidget( _VideoColorMode, row, 1 );

    // resolution
    row++;
    l = new QLabel( tr( "Resolution:" ), w );
    l->setAlignment( QLabel::AlignRight | QLabel::AlignVCenter );
    gl->addWidget( l, row, 0 );

    _Resolution = new QComboBox( FALSE, w );
    QWhatsThis::add(  _Resolution, tr( "Resolutions higher than 384x288 aren't supported." ) );
    gl->addWidget( _Resolution, row, 1 );

    // color space
    row++;
    l = new QLabel( tr( "Capture color space:" ), w );
    l->setAlignment( QLabel::AlignRight | QLabel::AlignVCenter );
    gl->addWidget( l, row, 0 );

    _Colorspace = new QComboBox( FALSE, w );
    _Colorspace->insertItem( tr( "YV12" ) );
    _Colorspace->insertItem( tr( "YUY2" ) );
    _Colorspace->insertItem( tr( "RGB15LE" ) );
    _Colorspace->insertItem( tr( "RGB24" ) );
    _Colorspace->insertItem( tr( "RGB32" ) );
    //QWhatsThis::add(  _Colorspace, tr( "Resolutions higher than 384x288 aren't supported." ) );
    gl->addWidget( _Colorspace, row, 1 );

    w = new QWidget( tabWidget );
    QVBoxLayout* vbl36 = new QVBoxLayout( w );
    vbl36->setMargin( 5 );

    QGroupBox* gb = new QGroupBox( tr( "Destination file" ), w );
    vbl36->addWidget( gb );
    gb->setColumnLayout( 0, Qt::Vertical );
    QHBoxLayout* hbl = new QHBoxLayout( gb->layout() );
    hbl->setSpacing( 5 );
    _ChangeFile = new QPushButton( tr( "C&hange..." ), gb );
    hbl->addWidget( _ChangeFile );
    _FileName = new QLineEdit( gb );
    hbl->addWidget( _FileName );

    cb_never_overwrite=new QCheckBox(tr("never overwrite existing files"),gb);
    QVBoxLayout* vblnvo = new QVBoxLayout( gb->layout() );
    vblnvo->setSpacing( 5 );
    vblnvo->addWidget(cb_never_overwrite);

    gb_dirpool=new QGroupBox(tr("Directory Pool"),w);
    gb_dirpool->setColumnLayout( 0, Qt::Vertical );
    vbl36->addWidget(gb_dirpool);

    QVBoxLayout * lay_pool1=new QVBoxLayout( gb_dirpool->layout() );
    lv_dirpool=new QListView(gb_dirpool);
    lv_dirpool->setBaseSize(50,20);
    lay_pool1->addWidget(lv_dirpool);

    lv_dirpool->setSelectionMode(QListView::Single);
    lv_dirpool->addColumn(tr("Path"));
    lv_dirpool->addColumn(tr("keep free"));
    lv_dirpool->addColumn(tr("now free"));
    lv_dirpool->setAllColumnsShowFocus(true);
    lv_dirpool->setShowSortIndicator(true);

    QHBoxLayout * lay_pool2=new QHBoxLayout( gb_dirpool->layout() );

    bt_add_dirpool=new QPushButton(tr("add"),gb_dirpool);
    lay_pool2->addWidget(bt_add_dirpool);
    lay_pool2->setSpacing(3);
    lay_pool2->setMargin(3);

    bt_rem_dirpool=new QPushButton(tr("remove"),gb_dirpool);
    lay_pool2->addWidget(bt_rem_dirpool);

    QHBoxLayout * lay_pool3=new QHBoxLayout( gb_dirpool->layout() );

    connect(bt_add_dirpool,SIGNAL(pressed()),this,SLOT(dirpool_add()));
    connect(bt_rem_dirpool,SIGNAL(pressed()),this,SLOT(dirpool_rem()));

    le_dirpool_name=new QLineEdit(gb_dirpool);
    lay_pool3->addWidget(le_dirpool_name);
    lay_pool3->setSpacing(3);
    lay_pool3->setMargin(3);

    qs_dirpool_minfree=new QSpinBox(100,320*1000,100,gb_dirpool);
    qs_dirpool_minfree->setPrefix(tr("keep free "));
    qs_dirpool_minfree->setSuffix(" MB");
    lay_pool3->addWidget(qs_dirpool_minfree);
;

    gb = new QGroupBox( tr( "Segmentation" ), w );
    vbl36->addWidget( gb );
    gb->setColumnLayout( 0, Qt::Vertical );
    QToolTip::add( gb, tr( "" ) );
    QWhatsThis::add( gb, tr( "Segmentation allows to write more than 2 Gb"
			    " of data ( limit for an AVI file ) during one"
			    " session. When it's turned on, program will"
			    " create a sequence of AVI files, automatically"
			    " switching them when their sizes become larger"
			    " than user-specified value." ) );

    gl = new QGridLayout( gb->layout(), 1, 1 );
    gl->setSpacing( 5 );

    l = new QLabel( tr( "Segment size ( KBytes ):" ), gb );
    gl->addWidget( l, 1, 0 );

    _HaveSegmented = new QCheckBox( tr( "&Write segmented file" ), gb );
    gl->addWidget( _HaveSegmented, 0, 0 );

    _SegmentSize = new QLineEdit( gb );
    _SegmentSize->setEnabled( FALSE );
    gl->addWidget( _SegmentSize, 1, 1 );

    tabWidget->insertTab( w, tr( "&File" ) );



    w = new QWidget( tabWidget );
    QVBoxLayout* vbl37 = new QVBoxLayout( w );
    vbl37->setMargin( 5 );

    gb = new QGroupBox( tr( "Video compression format" ), w );
    vbl37->addWidget( gb );
    gb->setColumnLayout( 0, Qt::Vertical );

    hbl = new QHBoxLayout( gb->layout() );
    hbl->setSpacing( 5 );

    l = new QLabel( tr( "Current format:" ), gb );
    hbl->addWidget( l );

    _codecName = new QLabel( gb );
    _codecName->setText( tr( "" ) );
    _codecName->setFrameShadow( QLabel::Sunken );
    _codecName->setFrameShape( QLabel::Panel );
    _codecName->setAlignment( QLabel::AlignCenter );
    hbl->addWidget( _codecName );

    _ChangeCodec = new QPushButton( gb );
    _ChangeCodec->setText( tr( "C&hange..." ) );
    hbl->addWidget( _ChangeCodec );

    QHBoxLayout *vblnc = new QHBoxLayout( gb->layout() );
    vblnc->setSpacing( 5 );

    named_codecs_cb=new QComboBox(true,gb);
    vblnc->addWidget( named_codecs_cb);
    named_codecs_cb->insertItem(tr("-Default-"));


    gb = new QGroupBox( tr( "Video frame rate" ), w );
    vbl37->addWidget( gb );
    gb->setColumnLayout(0, Qt::Vertical);

    gl = new QGridLayout( gb->layout(), 1, 1 );
    gl->setSpacing( 5 );

    l = new QLabel( tr( "Frames per second:" ), gb );
    gl->addWidget( l, 0, 0 );

    _fps = new QComboBox( gb );
    _fps->insertItem( tr( "12" ) );
    _fps->insertItem( tr( "15" ) );
    _fps->insertItem( tr( "18" ) );
    _fps->insertItem( tr( "20" ) );
    _fps->insertItem( tr( "23.975" ) );
    _fps->insertItem( tr( "24" ) );
    _fps->insertItem( tr( "25" ) );
    _fps->insertItem( tr( "29.970" ) );
    _fps->insertItem( tr( "30" ) );
#if QT_VERSION > 220
    _fps->setEditable( true );
#endif
    QValidator* qv = new QDoubleValidator( 1, 30, 3, gb );
    _fps->setValidator( qv );
    _fps->setEditText( "25" );
    gl->addWidget( _fps, 0, 1 );

    gb = new QGroupBox( tr( "Video cropping" ), w );
    vbl37->addWidget( gb );
    gb->setColumnLayout(0, Qt::Vertical);

    tabWidget->insertTab( w, tr( "&Video" ) );


    w = new QWidget( tabWidget );
    vbl37 = new QVBoxLayout( w );
    vbl37->setMargin( 5 );

    gb = new QGroupBox( tr( "Audio compression format" ), w );
    vbl37->addWidget( gb );
    gb->setColumnLayout( 0, Qt::Vertical );

    gb = new QGroupBox( tr( "Audio format" ), w );
    vbl37->addWidget( gb );
    gb->setColumnLayout( 0, Qt::Vertical );

    gl = new QGridLayout( gb->layout(), 1, 1 );
    gl->setSpacing( 5 );

    l = new QLabel( tr( "Frequency:" ), gb );
    l->setAlignment( QLabel::AlignRight | QLabel::AlignVCenter );
    gl->addWidget( l, 1, 0 );

    l = new QLabel( tr( "Sample size:" ), gb );
    l->setAlignment( QLabel::AlignRight | QLabel::AlignVCenter );
    gl->addWidget( l, 1, 2 );

    l = new QLabel( tr( "Channels:" ), gb );
    l->setAlignment( QLabel::AlignRight | QLabel::AlignVCenter );
    gl->addWidget( l, 0, 2 );

    _listSamp = new QComboBox( FALSE, gb );
    _listSamp->insertItem( tr( "16 bit" ) );
    _listSamp->insertItem( tr( "8 bit" ) );
    _listSamp->setEnabled( FALSE );
    gl->addWidget( _listSamp, 1, 3 );

    _HaveAudio = new QCheckBox( tr( "Capture audi&o" ), gb );
    gl->addMultiCellWidget( _HaveAudio, 0, 0, 0, 1 );

    _listFreq = new QComboBox( FALSE, gb );
    _listFreq->insertItem( tr( "48000 Hz" ) );
    _listFreq->insertItem( tr( "44100 Hz" ) );
    _listFreq->insertItem( tr( "32000 Hz" ) );
    _listFreq->insertItem( tr( "22050 Hz" ) );
    _listFreq->insertItem( tr( "16000 Hz" ) );
    _listFreq->insertItem( tr( "12000 Hz" ) );
    _listFreq->insertItem( tr( "11025 Hz" ) );
    _listFreq->insertItem( tr( " 8000 Hz" ) );
    _listFreq->setEnabled( FALSE );
    gl->addWidget( _listFreq, 1, 1 );

    _listChan = new QComboBox( FALSE, gb );
    _listChan->insertItem( tr( "Mono" ) );
    _listChan->insertItem( tr( "Stereo" ) );
    _listChan->insertItem( tr( "Lang1" ) );
    _listChan->insertItem( tr( "Lang2" ) );
    _listChan->setEnabled( FALSE );
    gl->addWidget( _listChan, 0, 3 );

    tabWidget->insertTab( w, tr( "&Audio" ) );



    w = new QWidget( tabWidget );
    QVBoxLayout* vbl38 = new QVBoxLayout( w );
    vbl38->setMargin( 5 );

    gb = new QGroupBox( tr( "Limit capture process by:" ), w );
    vbl38->addWidget( gb );
    gb->setColumnLayout( 0, Qt::Vertical );

    gl = new QGridLayout( gb->layout(), 1, 1 );
    gl->setSpacing( 5 );

    _chkTime = new QCheckBox( tr( "&Time" ), gb );
    gl->addWidget( _chkTime, 0, 0 );

    _timeLimit = new QLineEdit( gb );
    _timeLimit->setEnabled( FALSE );
    gl->addWidget( _timeLimit, 0, 1 );

    l = new QLabel( tr( "seconds" ), gb );
    gl->addWidget( l, 0, 2 );

    _chkFileSize = new QCheckBox( tr( "File &size" ), gb );
    gl->addWidget( _chkFileSize, 1, 0 );

    _sizeLimit = new QLineEdit( gb );
    _sizeLimit->setEnabled( FALSE );
    gl->addWidget( _sizeLimit, 1, 1 );

    l = new QLabel( tr( "KBytes" ), gb );
    gl->addWidget( l, 1, 2 );

    tabWidget->insertTab( w, tr( "&Limits" ) );

    // shutdown tab/widgets

    w = new QWidget( tabWidget );
    QVBoxLayout* vblshut = new QVBoxLayout( w );
    vblshut->setMargin( 1 );
    
    rb_buttons=new QVButtonGroup(tr("when to shutdown"),w);
    rb_shutdown_never=new QRadioButton(tr("never shut down"),rb_buttons);
    rb_shutdown_last=new QRadioButton(tr("shutdown after last recording"),rb_buttons);
    rb_shutdown_inbetween=new QRadioButton(tr("shutdown in-between recordings"),rb_buttons);
    vblshut->addWidget(rb_buttons);

    gb = new QGroupBox( tr( "Shutdown/Resume Options" ), w );
    vblshut->addWidget( gb );
    gb->setColumnLayout( 0, Qt::Vertical );

    gl = new QGridLayout( gb->layout(), 1, 1 );
    gl->setSpacing( 1 );

    l=new QLabel(tr("minimum downtime in min"),gb);
    gl->addWidget(l,1,0);
    sb_min_timespan=new QSpinBox(5,60,1,gb);
    gl->addWidget(sb_min_timespan,1,1);

    l=new QLabel(tr("boot n mins before recording"),gb);
    gl->addWidget(l,2,0);
    sb_reboot_timespan=new QSpinBox(1,20,1,gb);
    gl->addWidget(sb_reboot_timespan,2,1);

    l=new QLabel(tr("gracetime for shutdown in min"),gb);
    gl->addWidget(l,3,0);
    sb_shutdown_grace=new QSpinBox(1,10,1,gb);
    gl->addWidget(sb_shutdown_grace,3,1);


    tabWidget->insertTab(w,tr("Shutdown/Resume"));

    //end of shutdown widgets



    // other tab/widgets

    QVGroupBox *gb_other = new QVGroupBox(tr("other options"), tabWidget);
    
    cb_log=new QCheckBox(tr("Log actions to file"),gb_other);

    QHGroupBox *gb_password=new QHGroupBox(tr("Password Lock"),gb_other);
    
    bt_password_set=new QPushButton(tr("Set Password"),gb_password);
    bt_password_lock=new QPushButton(tr("press to Lock"),gb_password);

    tabWidget->insertTab(gb_other,tr("Other"));

    //end of other tab widgets


    gridLayout()->addWidget( tabWidget, 0, 0 );
    setApplyEnabled( TRUE );

    // signals and slots connections
    connect( _HaveAudio, SIGNAL( toggled(bool) ), this, SLOT( toggle_audio(bool) ) );
    connect( _HaveSegmented, SIGNAL( toggled(bool) ), this, SLOT( toggle_segmented(bool) ) );
    connect( _ChangeFile, SIGNAL( clicked() ), this, SLOT( change_filename() ) );
    connect( _ChangeCodec, SIGNAL( clicked() ), this, SLOT( change_codec() ) );
    connect( _chkTime, SIGNAL( toggled(bool) ), this, SLOT( toggle_limittime(bool) ) );
    connect( _chkFileSize, SIGNAL( toggled(bool) ), this, SLOT( toggle_limitsize(bool) ) );
}

/*
 *  Destroys the object and frees any allocated resources
 */
VidcapConfigDialog::~VidcapConfigDialog()
{
    // no need to delete child widgets, Qt does it all for us
}
