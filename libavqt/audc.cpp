#include "audc.h"
#include <qcombobox.h>

/*
 *  Constructs a Test which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
AvOrderedDialog::AvOrderedDialog( QWidget* parent )
{
}

/*
 *  Destroys the object and frees any allocated resources
 */
AvOrderedDialog::~AvOrderedDialog()
{
    // no need to delete child widgets, Qt does it all for us
}

void AvOrderedDialog::accept()
{
}

#include "audc.moc"
