#ifndef AUDIOCOMPRESS_H
#define AUDIOCOMPRESS_H

#include "okdialog.h"
class QComboBox;

class AudioCompress : public QavmOkDialog
{ 
    Q_OBJECT;

public:
    QComboBox* m_pComboBox;

    AudioCompress( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
};

#endif // AUDIOCOMPRESS_H
