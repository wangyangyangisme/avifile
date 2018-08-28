#include "audc_p.h"
#include "audc_p.moc"

#include <qcombobox.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qvgroupbox.h>
#include <qwhatsthis.h>

/* 
 *  Constructs a AudioCompress which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
AudioCompress::AudioCompress( QWidget* parent,  const char* name, bool modal, WFlags fl )
    : QavmOkDialog( parent, name, modal, fl )
{
    if ( !name )
	setName( "AudioCompress" );
    resize( 256, 102 ); 
    setCaption( tr( "Audio compression format"  ) );

    QGridLayout* gl = gridLayout();
    gl->setMargin( 5 );
    gl->setSpacing( 4 );

    QGroupBox* vbl = new QVGroupBox( this );
    vbl->setTitle( tr( "Audio compression:" ) );

    m_pComboBox = new QComboBox( FALSE, vbl );
    m_pComboBox->insertItem( tr( "PCM Uncompressed" ) );
    m_pComboBox->insertItem( tr( "MP3" ) );
    gl->addMultiCellWidget( vbl, 0, 0, 0, 0);
}
