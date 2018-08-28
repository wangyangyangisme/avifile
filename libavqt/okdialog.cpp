#include "okdialog.h"
#include "okdialog.moc"

#include <qlayout.h>
#include <qhbox.h>
#include <qpushbutton.h>

QavmOkDialog::QavmOkDialog( QWidget* parent,  const char* name, bool modal, WFlags fl )
    :QDialog( parent, name, modal, fl ), m_bApplyEnabled(false),
    m_bOkDefault(true)
{
    setCaption( name );
    m_pGl = new QGridLayout( this, 1, 1 );
    m_pGl->setMargin( 5 );
    m_pGl->setSpacing( 5 );
}

void QavmOkDialog::apply()
{
    // to be overloaded
}

int QavmOkDialog::exec()
{
    QGridLayout* tgl =  new QGridLayout( 0, 1, 3 );
    int col = 1;
    QPushButton* ok = new QPushButton( tr( "&Ok" ), this );
    if (m_bOkDefault)
	ok->setDefault( TRUE );
    else
	ok->setAutoDefault( TRUE );
    tgl->addWidget( ok, 0, col++ );
    if (m_bApplyEnabled)
    {
	QPushButton* apply = new QPushButton( tr( "&Apply" ), this );
	connect( apply, SIGNAL( clicked() ), this, SLOT( apply() ) );
	tgl->addWidget( apply, 0, col++ );
    }

    QPushButton* cancel = new QPushButton( tr( "&Cancel" ), this );
    tgl->addWidget( cancel, 0, col );
    tgl->setColStretch( 0, 1 );

    m_pGl->addMultiCell( tgl, m_pGl->numRows(), m_pGl->numRows(),
			       0, m_pGl->numCols() - 1 );

    connect( ok, SIGNAL( clicked() ), this, SLOT( accept() ) );
    connect( cancel, SIGNAL( clicked() ), this, SLOT( reject() ) );

    return QDialog::exec();
}
