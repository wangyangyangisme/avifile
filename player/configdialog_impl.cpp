#include "configdialog_impl.h"
#include "configdialog_impl.moc"

#include "playercontrol.h"

#include "aviplay.h"
#include "avm_creators.h"
#define DECLARE_REGISTRY_SHORTCUT
#include "configfile.h"
#undef DECLARE_REGISTRY_SHORTCUT

#include <qcolordialog.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qfont.h>
#include <qfontdialog.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qlistbox.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qspinbox.h>

#include <stdlib.h>
#include <stdio.h>


ConfigDialog_impl::ConfigDialog_impl(PlayerControl* control)
    : ConfigDialog(0, "Configuration", true),
    m_pControl(control), itemLast(0),
    m_bFontChanged(false), m_bAudioChanged(false), m_bVideoChanged(false)
{
    char* fs = 0;
    char* http_proxy = 0;
    unsigned int audio_resampling_rate;
    int subtitle_async;
    unsigned int rate;
    bool audio_resampling;
    bool use_yuv;
    bool use_http_proxy;
    bool autorepeat;
    bool quality_auto;
    bool preserve_aspect;
    bool display_frame_pos;
    bool video_buffered;
    bool video_direct;
    bool video_dropping;
    avm::IAviPlayer* p = m_pControl->getPlayer(); // shortcut

    p->Get(p->ASYNC_TIME_MS, &m_iAsyncOriginal,
	   p->SUBTITLE_FONT, &fs,
	   p->SUBTITLE_ASYNC_TIME_MS, &subtitle_async,
	   p->AUDIO_STREAM, &m_iAudioStreamOriginal,
	   p->HTTP_PROXY, &http_proxy,
	   p->USE_HTTP_PROXY, &use_http_proxy,
	   p->USE_YUV, &use_yuv,
	   p->AUDIO_RESAMPLING, &audio_resampling,
	   p->AUDIO_RESAMPLING_RATE, &audio_resampling_rate,
	   p->AUDIO_PLAYING_RATE, &rate,
	   p->AUTOREPEAT, &autorepeat,
	   p->VIDEO_QUALITY_AUTO, &quality_auto,
	   p->VIDEO_PRESERVE_ASPECT, &preserve_aspect,
	   p->DISPLAY_FRAME_POS, &display_frame_pos,
	   p->VIDEO_BUFFERED, &video_buffered,
	   p->VIDEO_DIRECT, &video_direct,
	   p->VIDEO_DROPPING, &video_dropping,
	   0);

    m_iSubAsyncOriginal = subtitle_async;
    m_pAsyncSlider->setValue(m_iAsyncOriginal / 10);
    onAsyncValChanged(0);

    if (fs)
    {
	char* n = strstr(fs, ":qtfont=");
	m_pFontName->setText( (n) ? n + 8 : fs );
	free(fs);
    }
    onFontChanged();
    m_bFontChanged = false;

    if (subtitle_async < 0)
    {
	m_pSubNegative->setChecked( true );
	subtitle_async *= -1;
    }
    else
	m_pSubNegative->setChecked( false );

    subtitle_async /= 1000;
    m_pSubAsyncMin->setValue( subtitle_async / 60 );
    m_pSubAsyncSec->setValue( subtitle_async % 60 );

    m_pDefAudio->setValue( m_iAudioStreamOriginal );

    if (http_proxy)
    {
	if (strlen(http_proxy) > 0)
	    m_pProxyName->setText( http_proxy );
        free(http_proxy);
    }

    if (use_http_proxy)
    {
	m_pUseProxy->setChecked(true);
	m_pProxyName->setEnabled(true);
    }

    m_pHwaccel->setChecked(use_yuv);

#if QT_VERSION >= 300
    avm::string usedefth = RS("theme", "default");
    m_pNoDefaultTheme->setChecked( (usedefth != "default") );
    m_pThemeList->setEnabled( m_pNoDefaultTheme->isChecked() );
    m_pThemeList->setCurrentText( usedefth.c_str() );
#endif
#if QT_VERSION > 220
    char b[50];
    sprintf(b, "%d", audio_resampling_rate);
    m_pAudioResamplingRate->lineEdit()->setText(b);
#else
    for (unsigned i = 0; frequencyList[i]; i++)
	if (audio_resampling_rate <= frequencyList[i])
	    m_pAudioResamplingRate->setCurrentItem(i);

#endif
    for (unsigned i = 0; frequencyList[i]; i++)
	if (frequencyList[i] == rate)
	{
	    m_pAudioPlayingRate->setCurrentItem(i);
            break;
	}

    m_pAudioResampling->setChecked(audio_resampling);
    onAudioResamplingToggled();

    m_pAutorepeat->setChecked(autorepeat);
    m_pQualityAuto->setChecked(quality_auto);
    m_pPreserveAspect->setChecked(preserve_aspect);
    m_pDisplayFramePos->setChecked(display_frame_pos);
    m_pVideoBuffered->setChecked(video_buffered);
    m_pVideoDirect->setChecked(video_direct);
    m_pVideoDropping->setChecked(video_dropping);

    avm::StreamInfo* si = p->GetVideoStreamInfo();
    fourcc_t fcc = (si) ? si->GetFormat() : avm::CodecInfo::ANY;
    delete si;
    avm::vector<const avm::CodecInfo*> infos;
    avm::CodecInfo::Get(infos, avm::CodecInfo::Video, avm::CodecInfo::Decode, fcc);

    int sel=0;
    m_pVideoList->clear();
    avm::vector<const avm::CodecInfo*>::iterator it;
    for (it = infos.begin(); it != infos.end(); it++)
	m_pVideoList->insertItem((*it)->GetName());

    si = p->GetAudioStreamInfo();
    fcc = (si) ? si->GetFormat() : avm::CodecInfo::ANY;
    delete si;
    infos.clear();
    avm::CodecInfo::Get(infos, avm::CodecInfo::Audio, avm::CodecInfo::Decode, fcc);
    m_pAudioList->clear();
    for (it = infos.begin(); it != infos.end(); it++)
	m_pAudioList->insertItem((*it)->GetName());

    connect(m_pAsyncSlider, SIGNAL(valueChanged(int)), this, SLOT(onAsyncValChanged(int)));

    connect(m_pSubAsyncMin, SIGNAL(valueChanged(int)), this, SLOT(onSubAsyncValChanged(int)));
    connect(m_pSubAsyncSec, SIGNAL(valueChanged(int)), this, SLOT(onSubAsyncValChanged(int)));

    connect(m_pUseProxy, SIGNAL(clicked()), this, SLOT(onProxyToggled()));
    connect(m_pAudioResampling, SIGNAL(clicked()), this, SLOT(onAudioResamplingToggled()));
    connect(m_pFontButton, SIGNAL(clicked()), this, SLOT(onFont()));

    connect(m_pSubBGColor, SIGNAL(clicked()), this, SLOT(onBGColor()));
    connect(m_pSubFGColor, SIGNAL(clicked()), this, SLOT(onFGColor()));

    connect(m_pDefAudio, SIGNAL(valueChanged(int)), this, SLOT(onAudioStreamChanged(int)));

    connect(m_pFontName, SIGNAL(returnPressed()), this, SLOT(onFontChanged()));
    //connect(m_pVideoList, SIGNAL(selectionChanged(QListBoxItem*)), this, SLOT(onSelectionChanged(QListBoxItem*)));

#if QT_VERSION > 220
    connect(m_pVideoList, SIGNAL(currentChanged(QListBoxItem*)), this, SLOT(onSelectionChanged(QListBoxItem*)));
    connect(m_pVideoList, SIGNAL(pressed(QListBoxItem*)), this, SLOT(onListPressed(QListBoxItem*)));
    connect(m_pVideoList, SIGNAL(clicked(QListBoxItem*)), this, SLOT(onListClicked(QListBoxItem*)));

    connect(m_pAudioList, SIGNAL(currentChanged(QListBoxItem*)), this, SLOT(onSelectionChanged(QListBoxItem*)));
    connect(m_pAudioList, SIGNAL(pressed(QListBoxItem*)), this, SLOT(onListPressed(QListBoxItem*)));
    connect(m_pAudioList, SIGNAL(clicked(QListBoxItem*)), this, SLOT(onListClicked(QListBoxItem*)));
#endif
#if QT_VERSION >= 300
    connect(m_pNoDefaultTheme, SIGNAL(clicked()), this, SLOT(onNoDefaultTheme()));
#endif
    // 3.0
    //connect(m_pVideoList, SIGNAL(mouseButtonPressed(int, QListBoxItem*, const QPOint&)), this,
    //        SLOT(onMouseButtonPressed(int, QListBoxItem*, const QPOint& )));
}

void ConfigDialog_impl::onSelectionChanged(QListBoxItem* item)
{
    if (m_Mutex.TryLock() < 0)
	return;

    //printf("ONSEL  %p   last: %p\n", item, itemLast);
    if (itemLast)
    {
	QListBox* lb = item->listBox();
        int i = lb->index(item);
	int ilast = lb->index(itemLast);

	//printf("ONSEL locked  %d <-> %d\n", i, ilast);
	if (i >= 0 && ilast >= 0 && labs(i - ilast) < 2)
	{
	    QString t = item->text().isNull() ? "" : item->text().ascii();
	    QString tlast = itemLast->text().isNull() ? "" : itemLast->text().ascii();
	    // swap two items
	    //printf("swap  %s  %s\n idx1 %d   idx2 %d\n", t.ascii(), tlast.ascii(), i, ilast);
            lb->changeItem(tlast, i);
            lb->changeItem(t, ilast);
	    //printf("New itemLast %p\n", itemLast);
	    itemLast = lb->item(i);
	    if (lb == m_pAudioList)
                m_bAudioChanged = true;
	    else if (lb == m_pVideoList)
                m_bVideoChanged = true;
	}
    }
    m_Mutex.Unlock();
}

void ConfigDialog_impl::onListPressed(QListBoxItem* item)
{
    //printf("onlistpressed\n");
    itemLast = item;//(!itemLast) ? item : 0;
}

void ConfigDialog_impl::onListClicked(QListBoxItem* item)
{
    //printf("onlistclicked\n");
    itemLast = 0;
}

void ConfigDialog_impl::onMouseButtonPressed(int button, QListBoxItem* item, const QPoint& pos)
{
    printf("Onmouse  %d %s\n", button, item->text().ascii());
}

void ConfigDialog_impl::onProxyToggled()
{
    m_pProxyName->setEnabled(m_pUseProxy->isChecked());
}

void ConfigDialog_impl::onAudioResamplingToggled()
{
    bool s = m_pAudioResampling->isChecked();
    m_pAudioResamplingRate->setEnabled(s);
    m_pAudioPlayingRate->setEnabled(s);
}

void ConfigDialog_impl::onNoDefaultTheme()
{
    m_pThemeList->setEnabled( m_pNoDefaultTheme->isChecked() );
}

void ConfigDialog_impl::apply()
{
    avm::IAviPlayer* p = m_pControl->getPlayer();
#if QT_VERSION >= 300
    if (m_pNoDefaultTheme->isChecked())
    {
	qApp->setStyle( m_pThemeList->currentText() );
	WS("theme", m_pThemeList->currentText());
    }
    else
	WS("theme", "default");
#endif

    if (m_bFontChanged)
    {
	QString raw = m_defFont.rawName();
	if (raw.find('-') == -1) {
	    raw = m_defFont.family();
	    QString s;
            //encoding=iso10646-1:
	    s.sprintf("-%d:weight=", m_defFont.pointSize());
	    raw += s;
            int w = m_defFont.weight();
	    if (w <= 25)
		raw += "light";
	    else if (w <= 50)
		raw += "medium";
	    else if (w <= 63)
		raw += "demibold";
	    else if (w <= 75)
		raw += "bold";
	    else
		raw += "black";

	    if (m_defFont.italic())
		raw += ":slant=italic";
	    else
                raw += ":slant=roman";
	}

#if QT_VERSION >= 300
	raw += ":qtfont=";
	raw += m_defFont.toString();
#endif
	p->Set(p->SUBTITLE_FONT, raw.ascii(), 0);
    }

    avm::string vcodecs;
    for (unsigned i = 0; i < m_pVideoList->count(); i++)
    {
	if (vcodecs.size() > 0)
	    vcodecs += ",";
        vcodecs += m_pVideoList->text(i);
    }

    avm::string acodecs;
    for (unsigned i = 0; i < m_pAudioList->count(); i++)
    {
	if (acodecs.size() > 0)
	    acodecs += ",";
        acodecs += m_pAudioList->text(i);
    }

    m_iAsyncOriginal = m_pAsyncSlider->value() * 10;

    int sub_async = m_pSubAsyncMin->value()*60 + m_pSubAsyncSec->value();
    sub_async *= (m_pSubNegative->isChecked()) ? -1000 : 1000;
    m_iSubAsyncOriginal = sub_async;

    p->Set(p->ASYNC_TIME_MS, m_iAsyncOriginal,
	   p->SUBTITLE_ASYNC_TIME_MS, sub_async,
	   p->USE_YUV, m_pHwaccel->isChecked(),
	   p->AUDIO_STREAM, m_pDefAudio->value(),
	   p->AUDIO_RESAMPLING, m_pAudioResampling->isChecked(),
	   p->AUDIO_RESAMPLING_RATE,
#if QT_VERSION > 220
	   atoi(m_pAudioResamplingRate->lineEdit()->text().ascii()),
#else
	   frequencyList[m_pAudioResamplingRate->currentItem()],
#endif
	   p->AUDIO_PLAYING_RATE,
	     frequencyList[m_pAudioPlayingRate->currentItem()],
	   p->USE_HTTP_PROXY, m_pUseProxy->isChecked(),
	   p->HTTP_PROXY, m_pProxyName->text().ascii(),
	   p->DISPLAY_FRAME_POS, m_pDisplayFramePos->isChecked(),
	   p->AUTOREPEAT, m_pAutorepeat->isChecked(),
	   p->VIDEO_PRESERVE_ASPECT, m_pPreserveAspect->isChecked(),
	   p->VIDEO_QUALITY_AUTO, m_pQualityAuto->isChecked(),
	   p->VIDEO_BUFFERED, m_pVideoBuffered->isChecked(),
	   p->VIDEO_DIRECT, m_pVideoDirect->isChecked(),
	   p->VIDEO_DROPPING, m_pVideoDropping->isChecked(),
           0);

    if (m_bVideoChanged)
	p->Set(p->VIDEO_CODECS, vcodecs.c_str(), 0);
    if (m_bAudioChanged)
	p->Set(p->AUDIO_CODECS, acodecs.c_str(), 0);

    m_pControl->setDisplayFramePos(m_pDisplayFramePos->isChecked());

    m_bVideoChanged = m_bAudioChanged = m_bFontChanged = false;
}

void ConfigDialog_impl::accept()
{
    apply();
    ConfigDialog::accept();
}

void ConfigDialog_impl::reject()
{
    m_pAsyncSlider->setValue( m_iAsyncOriginal / 10 );
    m_pDefAudio->setValue( m_iAudioStreamOriginal );

    avm::IAviPlayer* p = m_pControl->getPlayer();
    if (p)
	p->Set(p->SUBTITLE_ASYNC_TIME_MS, m_iSubAsyncOriginal, 0);

    ConfigDialog::reject();
}

void ConfigDialog_impl::onAsyncValChanged(int)
{
    float value = (float)m_pAsyncSlider->value() / 100.0;
    QString t;
    t.sprintf("%2.2f s", value);
    m_pAsync->setText(t);

    avm::IAviPlayer* p = m_pControl->getPlayer();
    if (p)
	p->Set(p->ASYNC_TIME_MS, int(value * 1000), 0);
}

void ConfigDialog_impl::onSubAsyncValChanged(int)
{
    avm::IAviPlayer* p = m_pControl->getPlayer();
    if (p)
    {
	int sub_async = m_pSubAsyncMin->value() * 60 + m_pSubAsyncSec->value();
	sub_async *= (m_pSubNegative->isChecked()) ? -1000 : 1000;
	p->Set(p->SUBTITLE_ASYNC_TIME_MS, sub_async, 0);
    }
}

void ConfigDialog_impl::onAudioStreamChanged(int)
{
    avm::IAviPlayer* p = m_pControl->getPlayer();
    if (p)
	p->Set(p->AUDIO_STREAM, m_pDefAudio->value(), 0);
}

void ConfigDialog_impl::onFont()
{
    bool isOk;

    QFont font = QFontDialog::getFont(&isOk, m_defFont, this, "Select font");
#if QT_VERSION >= 300
    //printf("WITH FONT:%s\nWITH RAW:%s\n", font.toString().ascii(), font.rawName().ascii());
    //printf("Family %s  size: %d  bold: %d\n", m_defFont.family().ascii(), m_defFont.pointSize(), m_defFont.bold());
#endif

    if (isOk)
    {
	//printf("QFONT %s\n>>> %s\n", font.toString().ascii(), font.rawName().ascii());
	QString s = font.rawName().ascii();
#if QT_VERSION >= 300
	if (s[0] != '-')
	    s = font.toString().ascii();
#endif
	m_pFontName->setText( s );
	onFontChanged();
    }
}

void ConfigDialog_impl::onBGColor()
{
    QColor color = QColorDialog::getColor(QColor(0,0,0));
}

void ConfigDialog_impl::onFGColor()
{
#if QT_VERSION >= 300
    QRgb color = QColorDialog::getRgba(0);
#else
    QColor color = QColorDialog::getColor(QColor(0,0,0));
#endif
}

void ConfigDialog_impl::onFontChanged()
{
    QString s = m_pFontName->text();
#if QT_VERSION >= 300
    if (s[0] != '-')
	m_defFont.fromString( s );
    else
#endif
	m_defFont.setRawName( s );

    //printf("ONFONTCHANGE %s\n", s.ascii());
    if (!m_defFont.family().isNull())
	s = m_defFont.family().ascii();

    int i = s.find(':');
    if (i != -1)
	s[i] = 0;
    QString str;
    str.sprintf("%s, %d pt%s%s",
		s.isEmpty() ? "default" : s.ascii(),
		m_defFont.pointSize(),
		m_defFont.bold()?", bold":"",
		m_defFont.italic()?", italic":"");

    m_pFontType->setText( str );
    m_pPreview->setFont( m_defFont );
    m_bFontChanged = true;
    update();
}
