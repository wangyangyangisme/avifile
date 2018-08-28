#ifndef QTAVORDEREDDIALOG_H
#define QTAVORDEREDDIALOG_H

#include "audc_p.h"

class AvOrderedDialog : public QtavOrderedDialogG
{ 
    Q_OBJECT
public:
    AvOrderedDialog( QWidget* parent);
    ~AvOrderedDialog();
    void accept();
};

#endif // AUDIOCOMP_H
