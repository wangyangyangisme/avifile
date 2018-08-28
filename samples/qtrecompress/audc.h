#ifndef AUDIOCOMP_H
#define AUDIOCOMP_H

#include "audc_p.h"
#include "recompressor.h"

#include "audioencoder.h"

class AudioCodecConfig : public AudioCompress
{ 
    Q_OBJECT;
    AudioEncoderInfo info;
public:
    AudioCodecConfig( QWidget* parent, const AudioEncoderInfo& fmt );
    AudioEncoderInfo GetInfo();
    virtual void accept();
};

#endif // AUDIOCOMP_H
