/****************************************************************************
****************************************************************************/
#include "audc_p.h"

#include <qcombobox.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
//#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

/* 
 */
QtavOrderedDialogG::QtavOrderedDialogG( QWidget* parent,  const char* name, bool modal, WFlags fl )
    : QDialog( parent, name, modal, fl )
{
    if ( !name )
	setName( "AudioCompress" );
    resize( 256, 102 ); 
    setCaption( tr( "Audio compression format"  ) );

    PushButton1_2 = new QPushButton( this, "PushButton1_2" );
    PushButton1_2->setGeometry( QRect( 162, 56, 80, 26 ) ); 
    PushButton1_2->setText( tr( "Cancel"  ) );

    TextLabel1 = new QLabel( this, "TextLabel1" );
    TextLabel1->setGeometry( QRect( 14, 19, 92, 22 ) ); 
    TextLabel1->setText( tr( "Audio compression:"  ) );

    ComboBox1 = new QComboBox( FALSE, this, "ComboBox1" );
    ComboBox1->insertItem( tr( "None" ) );
    ComboBox1->insertItem( tr( "MP3" ) );
    ComboBox1->setGeometry( QRect( 112, 19, 130, 22 ) ); 

    PushButton1 = new QPushButton( this, "PushButton1" );
    PushButton1->setGeometry( QRect( 67, 56, 89, 26 ) ); 
    PushButton1->setText( tr( "Ok"  ) );

    // signals and slots connections
    connect( PushButton1, SIGNAL( clicked() ), this, SLOT( accept() ) );
    connect( PushButton1_2, SIGNAL( clicked() ), this, SLOT( reject() ) );
}

/*  
 *  Destroys the object and frees any allocated resources
 */
QtavOrderedDialogG::~QtavOrderedDialogG()
{
    // no need to delete child widgets, Qt does it all for us
}

#include "audc_p.moc"
