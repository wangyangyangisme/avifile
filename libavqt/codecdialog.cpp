#include "codecdialog.h"
#include "codecdialog.moc"

#include "pixmapbutton.h"

#include "avm_cpuinfo.h"
#include "avm_creators.h"
#include "configfile.h"
#include "videoencoder.h"

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlcdnumber.h>
#include <qlineedit.h>
#include <qlistbox.h>
#include <qlistview.h>
#include <qmessagebox.h>
#include <qpushbutton.h>
#include <qtextbrowser.h>
#include <qtextview.h>
#include <qvbox.h>
#include <qvgroupbox.h>

#if QT_VERSION > 220
#include <qdial.h>
#include <qvariant.h>
#include <qsplitter.h>
#endif

#include <stdlib.h>
#include <stdio.h>

QavmCodecDialog::Input::Input(QWidget* parent, const QString& title, const QString& defval)
    :QavmOkDialog( parent, title, true ), val(defval)
{
    setCaption( QString( tr("Enter new ") ) + title );

    m_pEdit = new QLineEdit( val, this );
    gridLayout()->addWidget( m_pEdit, 0, 0 );
}

void QavmCodecDialog::Input::accept()
{
    val = m_pEdit->text();
    return QDialog::accept();
}
/*
QavmCodecDialog::QInputBoolean::QInputBoolean(QWidget* parent,
					      QString title, int defval)
    :QavmOkDialog( parent, title, true ), _defval(defval)
{
    char txt[256];
    sprintf(txt, "%d", defval);
    setCaption( QString( "Enter new " ) + title );

    m_pEdit = new QLineEdit( txt, this );
    gridLayout()->addWidget( m_pEdit, 0, 0 );
}
*/


QavmCodecDialog::InputSelect::InputSelect(QWidget* parent, const QString& title, const avm::vector<avm::string>& options, int defval)
    :QavmOkDialog( parent, title, true ), _options(options), _defval(defval)
{
    setCaption( QString( tr("Enter new ") ) + title );

    m_pBox = new QComboBox( QString().sprintf("%d", defval), this );
    gridLayout()->addWidget( m_pBox, 0, 0);

    int i = 0;
    avm::vector<avm::string>::const_iterator it;
    for(it = _options.begin(); it != _options.end(); it++) {
	m_pBox->insertItem(_options[i].c_str(), i);
        i++;
    }

    m_pBox->setCurrentItem(_defval);
}

void QavmCodecDialog::InputSelect::accept()
{
    _defval = m_pBox->currentItem();
    return QDialog::accept();
}

QavmCodecDialog::QavmCodecDialog( QWidget* parent, const avm::vector<avm::CodecInfo>& codecs,
				  avm::CodecInfo::Direction dir )
    :QavmOkDialog( parent, "Select codec", true ), m_Codecs(codecs),
    m_Direction(dir), m_pKeyframe(0), m_pQuality(0)
{
    m_Order.resize(codecs.size());
    for (unsigned i = 0; i < codecs.size(); i++)
	m_Order[i] = i;

    createGui();

    //printf("CodecDialog: %x  q:%d, %s\n", info.compressor, info.quality, info.cname.c_str());

#if QT_VERSION > 220
    connect(m_pListCodecs, SIGNAL( selectionChanged() ), this, SLOT( selectCodec() ) );
#else
    // Qt2.0 compatible - instead
    connect(m_pListCodecs, SIGNAL( highlighted( int ) ), this, SLOT( selectCodec() ) );
#endif
    connect(m_pListCodecs, SIGNAL( selected( QListBoxItem* ) ), this, SLOT( selectCodec() ) );

    codecUpdateList();
    setCurrent( 0 );
}

/*
 *  Destroys the object and frees any allocated resources
 */
QavmCodecDialog::~QavmCodecDialog()
{
    // no need to delete child widgets, Qt does it all for us
}

avm::VideoEncoderInfo QavmCodecDialog::getInfo()
{
    avm::VideoEncoderInfo info;
    int codec = getCurrent();
    info.compressor = m_Codecs[codec].fourcc;
    info.cname = m_Codecs[codec].GetName();

    printf("CodecDialog returns: %x, %s\n", info.compressor, info.cname.c_str());
    return info;
}

int QavmCodecDialog::getCurrent() const
{
    return m_Order[m_pListCodecs->currentItem()];
}

void QavmCodecDialog::setCurrent( int c )
{
    m_pListCodecs->setCurrentItem( c );
}

void QavmCodecDialog::about()
{
    int codec = getCurrent();

    QMessageBox::information( this, m_Codecs[codec].GetName(),
                              QString("<p align=center>") + m_Codecs[codec].GetAbout(),
			      QMessageBox::Ok );
}

void QavmCodecDialog::shortNames(int)
{
    //selectCodec();
    codecUpdateList();
}

void QavmCodecDialog::accept()
{
    return QDialog::accept();
}

void QavmCodecDialog::clickedAttr( QListViewItem* item )
{
    //const AttributeInfo& ai = m_Codecs[codec].encoder_info;
    if (!item)
        return;
    QString str = item->text(1);
    int codec = getCurrent();
    const avm::vector<avm::AttributeInfo> ci = m_Codecs[codec].encoder_info;
    int row = 0;
    avm::vector<avm::AttributeInfo>::const_iterator it;
    for (it = ci.begin(); it != ci.end(); row++, it++)
	if (!strcmp(it->GetAbout(), str.ascii()))
	{
	    if (it->GetMin() == 0 && it->GetMax() == 1)
	    {
		avm::CodecSetAttr(m_Codecs[codec], it->GetName(),
				  ((QCheckListItem*)item)->isOn() ? 1 : 0);
		//printf("STR %s  %d   %d\n", str.ascii(), row, ((QCheckListItem*)item)->isOn());
	    }
	    break;
	}
}

// backward compatible method
void QavmCodecDialog::rightClickedAttr( QListViewItem* item, const QPoint&, int )
{
    clickedAttr(item);
}

void QavmCodecDialog::changeAttr( QListViewItem* item )
{
    if (!item)
	return;

    QString str = item->text(1);
    QString qs;
    int defval = 0;
    float deffval = 0.;
    const char* def_str;
    int codec = getCurrent();
    bool setc = false;
    //printf("Getting %s  for %s\n", str.ascii(), m_Codecs[codec].GetName());
    Input* inp;
    InputSelect* qit;

    const avm::AttributeInfo* ai = m_Codecs[codec].FindAttribute( str.ascii(),
								  m_Direction );
    if (!ai)
	return;

    switch(ai->kind)
    {
    case avm::AttributeInfo::Integer:
	avm::CodecGetAttr(m_Codecs[codec], ai->GetName(), &defval);
	{
	    Input i(this, str + QString( tr(" value") ), qs.setNum(defval));
	    if (i.exec() == QDialog::Accepted)
	    {
		//cerr<<"Setting "<<it->GetName()<<" to "<<inp->value()<<" for "<<m_Codecs[codec].GetName()<<endl;
		avm::CodecSetAttr(m_Codecs[codec], ai->GetName(), i.getInt());
                setc = true;
	    }
	}
	break;
    case avm::AttributeInfo::Select:
	avm::CodecGetAttr(m_Codecs[codec], ai->GetName(), &defval);
	{
	    InputSelect i(this, str + QString( tr(" value") ), ai->options, defval);
	    if (i.exec() == QDialog::Accepted)
	    {
		//cerr<<"Setting "<<it->GetName()<<" to "<<qit->value()<<" for "<<m_Codecs[codec].GetName()<<endl;
		avm::CodecSetAttr(m_Codecs[codec], ai->GetName(), i.value());
                setc = true;
	    }
	}
	break;
    case avm::AttributeInfo::String:
	avm::CodecGetAttr(m_Codecs[codec], ai->GetName(), &def_str);
	{
	    Input i(this, str + QString( tr(" value") ), def_str);
	    if (i.exec() == QDialog::Accepted)
	    {
		//cerr<<"Setting "<<it->GetName()<<" to "<<qis->value()<<" for "<<m_Codecs[codec].GetName()<<endl;
		avm::CodecSetAttr(m_Codecs[codec], ai->GetName(), i.getString());
                setc = true;
	    }
	}
	break;
    case avm::AttributeInfo::Float:
	avm::CodecGetAttr(m_Codecs[codec], ai->GetName(), &deffval);
	{
	    Input i(this, str + QString( tr(" value") ), qs.setNum(deffval));
	    if (i.exec() == QDialog::Accepted)
	    {
		//cerr<<"Setting "<<it->GetName()<<" to "<<qis->value()<<" for "<<m_Codecs[codec].GetName()<<endl;
		avm::CodecSetAttr(m_Codecs[codec], ai->GetName(), i.getFloat());
                setc = true;
	    }
	}
	break;
    }
    if (setc)
	selectCodec();
}

void QavmCodecDialog::selectCodec()
{
    int codec = getCurrent();
    m_pTabAttr->clear();
    //m_pTabAttr->setSorting(0, false);
    //m_pTabAttr->setSorting(1, false);

    if (m_Direction == avm::CodecInfo::Encode
	|| m_Direction == avm::CodecInfo::Both)
	addAttributes(m_Codecs[codec], m_Codecs[codec].encoder_info);

    if (m_Direction == avm::CodecInfo::Decode
	|| m_Direction == avm::CodecInfo::Both)
	addAttributes(m_Codecs[codec], m_Codecs[codec].decoder_info);
}

void QavmCodecDialog::addAttributes(const avm::CodecInfo& cdi, const avm::vector<avm::AttributeInfo>& ci)
{
    QListViewItem* litem = 0;
    avm::vector<avm::AttributeInfo>::const_iterator it;
    int i = 0;
    bool use_short = (m_pShortBox->state() == QButton::On);

    m_pTabAttr->setEnabled( (ci.size() != 0) );

    for (it = ci.begin(); it != ci.end(); it++)
    {
	//printf("attr %s\n", it->GetAbout());
	char tmp[256];
	avm::string val = "<none>";
	int param;
        float fparam;
	bool check = false;

	switch (it->kind)
	{
	case avm::AttributeInfo::Integer:
	    if (avm::CodecGetAttr(cdi, it->GetName(), &param) == 0)
	    {
		//val=itoa(param, tmp, 10);
		check = (it->GetMin() == 0 && it->GetMax() == 1);
		sprintf(tmp, "%d", param);
		val = tmp;
	    }
	    break;
	case avm::AttributeInfo::Select:
	    if (avm::CodecGetAttr(cdi, it->GetName(), &param) == 0)
	    {
		//val=itoa(param, tmp, 10);
		sprintf(tmp, "%d", param);
		val = tmp;
		val += " ( ";
		val += it->options[param];
		val += " )";
		break;
	    }
	case avm::AttributeInfo::String:
	    {
                const char* n = 0;
		avm::CodecGetAttr(cdi, it->GetName(), &n);
                if (n)
		    val = strncpy(tmp, n, sizeof(tmp) - 1);
		else
		    val[0] = 0;
	    }
	    break;
	case avm::AttributeInfo::Float:
	    if (avm::CodecGetAttr(cdi, it->GetName(), &fparam) == 0)
	    {
		//val=itoa(param, tmp, 10);
		sprintf(tmp, "%f", fparam);
		val = tmp;
	    }
	    break;
	}

	QListViewItem* item;
	const char* n = (use_short) ? it->GetName() : it->GetAbout();
	if (check)
	{
	    item = new QCheckListItem( m_pTabAttr, "",
				      QCheckListItem::CheckBox );
	    ((QCheckListItem*)item)->setOn( param );
#if QT_VERSION > 220
	    if (litem) item->moveItem( litem );
#endif
	}
	else
	{
	    item = new QListViewItem( m_pTabAttr, litem );
	    item->setText( 0, val.c_str() );
	}
	item->setText( 1, n );
	litem = item;
    }
}

void QavmCodecDialog::createGui()
{
    setCaption( tr( "Configure codecs" ) );

#if QT_VERSION > 220
    setSizeGripEnabled( TRUE );
    QSplitter* spl = new QSplitter( this );
    spl->setOpaqueResize( TRUE );
#else
    QHBox* spl = new QHBox( this );
#endif

    gridLayout()->addMultiCellWidget( spl, 0, 3, 0, 0 );

    // left
    QVBox* vbl = new QVBox( spl );
    vbl->setMargin( 5 );

    // Codecs list
    QGroupBox* gb = new QVGroupBox( vbl );
    gb->setTitle( tr( "Codecs" ) );

    m_pListCodecs = new QListBox( gb );

    createMoveGroup( gb );

    // right
    vbl = new QVBox( spl );
    vbl->setMargin( 5 );

    // Attributes list
    gb = new QVGroupBox( vbl );
    gb->setTitle( tr( "Attributes" ) );
    //gb->setColumnLayout(0, Qt::Vertical);
    m_pTabAttr = new QListView( gb );
    m_pTabAttr->addColumn( tr( "Value" ) );
    m_pTabAttr->addColumn( tr( "Attribute" ) );
    //m_pTabAttr->setColumnAlignment( 1, Qt::AlignRight );
    m_pTabAttr->setSorting( -1, true );
    //QListViewItem * item = new QListViewItem( m_pTabAttr, 0 );

    //createLCD( vbl );

    QHBox* hb = new QHBox( vbl );
    m_pShortBox = new QCheckBox( tr( "&Shortcuts" ), hb );
    QWidget* w = new QWidget( hb );
    hb->setStretchFactor( w, 1 );
    QButton* pAbout = new QPushButton( tr( "&About..." ), hb );
    w = new QWidget( hb );
    hb->setStretchFactor( w, 1 );
    connect( m_pShortBox, SIGNAL( stateChanged(int) ), this, SLOT( shortNames(int) ) );
    connect( pAbout, SIGNAL( clicked() ), this, SLOT( about() ) );


#if QT_VERSION > 220
    connect( m_pTabAttr, SIGNAL( clicked( QListViewItem* ) ), this, SLOT( clickedAttr( QListViewItem* ) ) );
#else
    connect( m_pTabAttr, SIGNAL( rightButtonClicked( QListViewItem*, const QPoint&, int ) ), this, SLOT( rightClickedAttr( QListViewItem*, const QPoint&, int ) ) );
#endif

    connect( m_pTabAttr, SIGNAL( doubleClicked( QListViewItem* ) ), this, SLOT( changeAttr( QListViewItem* ) ) );
}

void QavmCodecDialog::createLCD( QWidget* parent )
{
#if QT_VERSION > 220
    QHBox* hbl = new QHBox( parent );
    // Quality
    QGroupBox* gb = new QGroupBox( hbl );
    gb->setTitle( tr( "Quality" ) );
    gb->setMaximumHeight( 100 );
    gb->setColumnLayout( 0, Qt::Horizontal );
    QHBoxLayout* hl = new QHBoxLayout( gb->layout() );
    QDial* d = new QDial( gb );
    m_pQuality = d;
    d->setMaxValue( 100 );
    d->setValue( 95 );
    m_pLCDNumber1 = new QLCDNumber( gb );
    m_pLCDNumber1->setFrameShadow( QLCDNumber::Raised );
    m_pLCDNumber1->setSegmentStyle( QLCDNumber::Flat );
    m_pLCDNumber1->setNumDigits( 3 );
    m_pLCDNumber1->setProperty( "intValue", 95 );

    hl->addWidget( m_pQuality );
    hl->addWidget( m_pLCDNumber1 );

    // KeyFrame frequency
    gb = new QGroupBox( hbl );
    gb->setTitle( tr( "Keyframe frequency" ) );
    gb->setMaximumHeight( 100 );
    gb->setColumnLayout( 0, Qt::Horizontal );
    hl = new QHBoxLayout( gb->layout() );
    d = new QDial( gb );
    m_pKeyframe = d;
    d->setMaxValue( 200 );
    d->setValue( 75 );
    d->setMinValue( 1 );
    m_pLCDNumber2 = new QLCDNumber( gb );
    m_pLCDNumber2->setSegmentStyle( QLCDNumber::Flat );
    m_pLCDNumber2->setNumDigits( 3 );
    m_pLCDNumber2->setProperty( "intValue", 75 );

    hl->addWidget( m_pKeyframe );
    hl->addWidget( m_pLCDNumber2 );

    // signals and slots connections
    connect( m_pKeyframe, SIGNAL( valueChanged( int ) ), m_pLCDNumber2, SLOT( display( int ) ) );
    connect( m_pQuality, SIGNAL( valueChanged( int ) ), m_pLCDNumber1, SLOT( display( int ) ) );
#endif
}

void QavmCodecDialog::createMoveGroup( QWidget* parent )
{
    QHBox* hb = new QHBox( parent );
    hb->setMargin( 5 );
    //hb->setSpacing( 5 );

    QWidget* w = new QWidget( hb );
    hb->setStretchFactor( w, 1 );

    QButton* pTop = new QavmPixmapButton( "top", hb );
    QButton* pUp = new QavmPixmapButton( "up", hb );
    pUp->setAutoRepeat( TRUE );
    QButton* pDown = new QavmPixmapButton( "down", hb );
    pDown->setAutoRepeat( TRUE );
    QButton* pBottom = new QavmPixmapButton( "bottom", hb );

    w = new QWidget( hb );
    hb->setStretchFactor( w, 1 );

    connect( pTop, SIGNAL( clicked() ), this, SLOT( codecTop() ) );
    connect( pUp, SIGNAL( clicked() ), this, SLOT( codecUp() ) );
    connect( pDown, SIGNAL( clicked() ), this, SLOT( codecDown() ) );
    connect( pBottom, SIGNAL( clicked() ), this, SLOT( codecBottom() ) );
}

void QavmCodecDialog::codecMove(int dir)
{
    //printf("MOVE %d\n", dir);
    if (m_Order.size() < 2)
        return;
    unsigned c = m_pListCodecs->currentItem();
    switch (dir)
    {
    case 1:
	if (c < (m_Order.size() - 1))
	{
	    unsigned cs = m_Order[c + 1];
	    m_Order[c + 1] = m_Order[c];
	    m_Order[c] = cs;
            c++;
	}
	break;
    case -1:
	if (c > 0)
	{
	    unsigned cs = m_Order[c - 1];
	    m_Order[c - 1] = m_Order[c];
	    m_Order[c] = cs;
            c--;
	}
	break;
    case 0:
	for (unsigned i = c; i > 0; i--)
	{
	    unsigned cs = m_Order[i - 1];
	    m_Order[i - 1] = m_Order[i];
	    m_Order[i] = cs;
	}
	break;
    case -1000:
	for (unsigned i = c; i < m_Order.size() - 1; i++)
	{
	    unsigned cs = m_Order[i + 1];
	    m_Order[i + 1] = m_Order[i];
            m_Order[i] = cs;
	}
	break;
    }

    setCurrent( c );
    codecUpdateList();
    selectCodec();
}

void QavmCodecDialog::codecUpdateList()
{
    bool use_short = (m_pShortBox->state() == QButton::On);
    int codec = m_pListCodecs->currentItem();
    m_pListCodecs->clear();
    for (unsigned i = 0; i < m_Order.size(); i++)
    {
	const avm::CodecInfo& ci = m_Codecs[m_Order[i]];
	const char* n = (use_short) ? ci.GetPrivateName() : ci.GetName();
	m_pListCodecs->insertItem( n );
    }
    setCurrent( codec );
}
