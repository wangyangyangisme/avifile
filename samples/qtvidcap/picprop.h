#ifndef PICTUREPROPDIALOG_H
#define PICTUREPROPDIALOG_H

#include "picprop_p.h"

class v4lxif;

class PicturePropDialog : public PicPropDialog
{ 
    Q_OBJECT;
    v4lxif* m_pDev;
    static const char* labels[PROP_LAST];
public:
    PicturePropDialog(v4lxif* pDev);
    //QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
    ~PicturePropDialog();
public slots:
    void changeBrightness(int);
    void changeContrast(int);
    void changeHue(int);
    void changeSaturation(int);
    void accept();
};

#endif // PICTUREPROPDIALOG_H
