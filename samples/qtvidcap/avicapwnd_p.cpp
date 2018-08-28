#include "avicapwnd_p.h"
#include "avicapwnd_p.moc"

#include <qframe.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qtabwidget.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qcolor.h>
#include <qwhatsthis.h>

/* 
 *  Constructs a AviCapDlg which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 */
AviCapDlg::AviCapDlg( QWidget* parent,  const char* name, WFlags fl )
    : QWidget( parent, name, fl )
{
    resize( 299, 422 ); 
    setCaption( tr( "AVI capture" ) );

    QVBoxLayout *vbmain=new QVBoxLayout(this);
    bigwidget=new QWidget(this);
    vbmain->addWidget(bigwidget);

    //bigwidget=this;

    QGridLayout* gl = new QGridLayout( bigwidget, 1, 1 );
    gl->setSpacing( 5 );
    gl->setMargin( 5 );

    CaptureTab = new QTabWidget( bigwidget, "CaptureTab" );
#if QT_VERSION > 220
    CaptureTab->setTabShape( QTabWidget::Rounded );
#endif
    //CaptureTab->setTabPosition( QTabWidget::Top );

    gl->addWidget( CaptureTab, 0, 0 );

    // first tab
    QWidget* w = new QWidget( CaptureTab );

    CaptureTab->insertTab( w, tr( "Progress" ) );
    QGridLayout* glt = new QGridLayout( w, 1, 1 );
    glt->setSpacing( 5 );
    glt->setMargin( 5 );
/*    Frame34 = new QFrame( w );
    glt->addWidget( Frame34, 0, 0 );
    Frame34->setFrameShadow( QFrame::Sunken );
    Frame34->setFrameShape( QFrame::StyledPanel );
    QVBoxLayout* vl = new QVBoxLayout( Frame34 );*/
    ProgressText = new QLabel( w );
    glt->addWidget( ProgressText, 0, 0 );
    //vl->addWidget( ProgressText );
    //ProgressText->setBackgroundColor( QColor(255,255,0) );
    ProgressText->setAlignment( Qt::AlignCenter );

    // second tab
    w = new QWidget( CaptureTab );
    CaptureTab->insertTab( w, tr( "Performance" ) );
    glt = new QGridLayout( w, 1, 1 );

    CaptureBufferText = new QLabel( tr( "Video capture buffer usage" ), w );
    CaptureBufferText->setAlignment( int( QLabel::AlignCenter ) );
    glt->addWidget( CaptureBufferText, 0, 0 );
    glt->setSpacing( 5 );
    glt->setMargin( 5 );

#if QT_VERSION > 220

    Frame3 = new QFrame( w );
    Frame3->setMinimumSize( QSize( 184, 94 ) );
    Frame3->setFrameShadow( QFrame::Sunken );
    Frame3->setFrameShape( QFrame::StyledPanel );

    QPalette pal;
    QColorGroup cg1;
    QColorGroup cg2;

    cg1.setColor( QColorGroup::Foreground, black );
    cg1.setColor( QColorGroup::Button, QColor( 192, 192, 192) );
    cg1.setColor( QColorGroup::Light, white );
    cg1.setColor( QColorGroup::Midlight, QColor( 223, 223, 223) );
    cg1.setColor( QColorGroup::Dark, QColor( 96, 96, 96) );
    cg1.setColor( QColorGroup::Mid, QColor( 128, 128, 128) );
    cg1.setColor( QColorGroup::Text, black );
    cg1.setColor( QColorGroup::BrightText, white );
    cg1.setColor( QColorGroup::ButtonText, black );
    cg1.setColor( QColorGroup::Base, white );
    cg1.setColor( QColorGroup::Background, black );
    cg1.setColor( QColorGroup::Shadow, black );
    cg1.setColor( QColorGroup::Highlight, black );
    cg1.setColor( QColorGroup::HighlightedText, white );

    pal.setActive( cg1 );
    pal.setInactive( cg1 );

    cg2 = cg1;
    cg2.setColor( QColorGroup::Foreground, QColor( 128, 128, 128) );
    cg2.setColor( QColorGroup::ButtonText, QColor( 128, 128, 128) );
    pal.setDisabled( cg2 );

    Frame3->setPalette( pal );
    //Frame3->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)3, (QSizePolicy::SizeType)3, Frame3->sizePolicy().hasHeightForWidth() ) );

    TextLabel3 = new QLabel( tr( "0 %" ), w );
    TextLabel3_2 = new QLabel( tr( " 100 % " ), w );

    glt->addMultiCellWidget( Frame3, 1, 3, 0, 0 );
    glt->addWidget( TextLabel3_2, 1, 1 );
    glt->addWidget( TextLabel3, 3, 1 );


    Frame3_2 = new QFrame( w );
    Frame3_2->setMinimumSize( QSize( 184, 94 ) );
    Frame3_2->setFrameShadow( QFrame::Sunken );
    Frame3_2->setFrameShape( QFrame::StyledPanel );
    Frame3_2->setPalette( pal );

    MaxFrameSizeText = new QLabel( tr( "100 kb" ), w );
    TextLabel3_3 = new QLabel( tr( "0 kb" ), w );

    glt->addMultiCellWidget( Frame3_2, 4, 6, 0, 0 );
    glt->addWidget( MaxFrameSizeText, 4, 1 );
    glt->addWidget( TextLabel3_3, 6, 1 );

    //QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
    //glt->addItem( spacer, 1, 1 );

#endif
    QHBoxLayout* hbl = new QHBoxLayout( gl );
    StartButton = new QPushButton( tr( "Start" ), bigwidget );
    hbl->addWidget( StartButton );
    SegmentButton = new QPushButton( tr( "Segment" ), bigwidget );
    hbl->addWidget( SegmentButton );
    MiniButton = new QPushButton( tr( "Mini" ), bigwidget );
    hbl->addWidget( MiniButton );
    CloseButton = new QPushButton( tr( "Close" ), bigwidget );
    hbl->addWidget( CloseButton );

    miniwidget = new QWidget();
    //vbmain->addWidget(miniwidget);
    QVBoxLayout *vbmini = new QVBoxLayout(miniwidget);

    //minilabel=new QLabel("text",miniwidget);
    m_pMiniButton=new QPushButton("pressme",miniwidget);
    vbmini->addWidget(m_pMiniButton);

    miniwidget->hide();
}

/*  
 *  Destroys the object and frees any allocated resources
 */
AviCapDlg::~AviCapDlg()
{
    // no need to delete child widgets, Qt does it all for us
}
