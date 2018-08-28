#include "audc.h"
#include "audc.moc"

#include <qcombobox.h>

/* 
 *  Constructs a AudioCodecConfig which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
AudioCodecConfig::AudioCodecConfig( QWidget* parent, const AudioEncoderInfo& fmt)
    : AudioCompress( 0, "Audio codec config", TRUE, 0 ), info(fmt)
{
}

void AudioCodecConfig::accept()
{
    if (m_pComboBox->currentItem()==1)
    {
	info.fmt = 0x55;
	info.bitrate = 128000;
    }
    return QDialog::accept();
}

AudioEncoderInfo AudioCodecConfig::GetInfo()
{
    return info;
}
