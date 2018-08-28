#ifndef PICPROPDIALOG_H
#define PICPROPDIALOG_H

#include <qdialog.h>

class QLabel;
class QSlider;

class PicPropDialog : public QDialog
{
    Q_OBJECT;
protected:
    enum {
	PROP_BRIGHTNESS,
	PROP_CONTRAST,
	PROP_SATURATION,
	PROP_HUE,
        PROP_LAST,
    };
public:
    PicPropDialog( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );

    QSlider* m_pSlider[PROP_LAST];
    QLabel* m_pLabel[PROP_LAST];
};

#endif // PICPROPDIALOG_H
