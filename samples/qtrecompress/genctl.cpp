
#include "genctl.h"
#include "genctl.moc"

#include "recwnd.h"
#include "codecdialog.h"
#include "audc.h"
#include "conf.h"

#include <avm_except.h>
#include <avm_cpuinfo.h>
#include <configfile.h>

#include <qlistbox.h>
#include <qmessagebox.h>
#include <qfiledialog.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qradiobutton.h>

/*
 *  Constructs a QtRecompressorCtl which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 */
QtRecompressorCtl::QtRecompressorCtl( QWidget* parent )
    : QtRecompressor( parent, "Avi Recompressor" )
{
    kernel=new RecKernel;
    wnd=0;
    ictl=0;
    no_recurs=0;

    m_pListAllFilters->clear();
    for(int i=0;;i++)
    {
	Filter* fi=kernel->getFilter(i);
	if(!fi)break;
	//printf("FULLNAMEA %s\n", fi->fullname().c_str());
	m_pListAllFilters->insertItem( tr( fi->name() ) );
	fi->release();
    }

    connect(_lbFilters, SIGNAL(doubleClicked( QListBoxItem * )), this, SLOT(configFilterClicked()));

    connect(m_pLineLowFrames, SIGNAL(textChanged(const QString&)), this, SLOT(lo_fr_changed(const QString&)));
    connect(m_pLineHighFrames, SIGNAL(textChanged(const QString&)), this, SLOT(hi_fr_changed(const QString&)));
    connect(m_pLineLowSec, SIGNAL(textChanged(const QString&)), this, SLOT(lo_sec_changed(const QString&)));
    connect(m_pLineHighSec, SIGNAL(textChanged(const QString&)), this, SLOT(hi_sec_changed(const QString&)));
    updateFilters();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QtRecompressorCtl::~QtRecompressorCtl()
{
    //cout << "QtRecompressorCtl::~QtRecompressorCtl()" << endl;
    delete kernel;
    delete wnd;
    // no need to delete child widgets, Qt does it all for us
}

/*
 * public slot
 */
void QtRecompressorCtl::aboutFilterClicked()
{
    Filter* fi=kernel->getFilter(m_pListAllFilters->currentItem());
    if(!fi)return;
    fi->about();
    fi->release();
}

void QtRecompressorCtl::updateFilters()
{
    m_pListAllFilters->clearSelection();
    _lbFilters->clear();

    for (int i = 0; ; i++)
    {
	Filter* fi = kernel->getFilterAt(i);
	if (!fi)
	    break;
	//printf("FULLNAMEB %s\n", fi->fullname().c_str());
	_lbFilters->insertItem(fi->fullname().c_str());
	fi->release();
    }
}

void QtRecompressorCtl::addFilterClicked()
{
    Filter* fi=kernel->getFilter(m_pListAllFilters->currentItem());
    if(!fi)return;
    int id=kernel->addFilter(fi);
    fi->release();
    updateFilters();
}

void QtRecompressorCtl::configFilterClicked()
{
    Filter* fi= kernel->getFilterAt(_lbFilters->currentItem());
    if(!fi)return;
    fi->config();
    fi->release();
    updateFilters();
}

void QtRecompressorCtl::fileOpenVideo()
{
    QString result=QFileDialog::getOpenFileName(avm::RegReadString(g_pcProgramName, "OpenFile", "./movie.avi"),
						0, 0, 0
#if QT_VERSION > 220
						, tr( "Open source file" )
#endif
					       );
    if (result.isNull())
	return;
    if (kernel->openFile(result.ascii())!=0)
	return;
    avm::RegWriteString(g_pcProgramName, "OpenFile", result.ascii());
    m_pLabelSrc->setText(result);

    m_pComboStreams->clear();
    for (unsigned i = 0; i < kernel->getStreamCount(); i++)
	m_pComboStreams->insertItem(kernel->getStreamName(i).c_str());
    streamSelected(0);

    m_pComboStreams->setCurrentItem(0);
    m_pRadioCopy->setChecked(true);
    m_pRadioSeconds->setChecked(true);
    m_pRadioFrames->setChecked(false);
    streamLimitModeChanged(0);
    streamModeChanged(2);

    if(!wnd)
    {
	ictl=new QImageControl(0);
	ictl->show();
	kernel->setImageControl(ictl);
	wnd = new SourceControl(0, kernel);
	wnd->show();
    }
    else
    {
	kernel->setImageControl(ictl);
	wnd->updateSize();
    }
}

void QtRecompressorCtl::fileAppendVideo()
{
    QString result=QFileDialog::getOpenFileName(avm::RegReadString(g_pcProgramName, "AppendFile", "./movie.avi"),
						0, 0, 0
#if QT_VERSION > 220
						, tr( "Append source file" )
#endif
					       );
    if (result.isNull())
	return;
    if (kernel->openFile(result.ascii())!=0)
	return;
}

void QtRecompressorCtl::fileSaveAvi()
{
    QString result=QFileDialog::getSaveFileName(avm::RegReadString(g_pcProgramName, "SaveFile", "./dest.avi"),
						0, 0, 0
#if QT_VERSION > 220
						, tr( "Save destination file" )
#endif
					       );
    if(result.isNull())
	return;
    if(kernel->setDestFile(result.ascii())!=0)
	return;
    avm::RegWriteString(g_pcProgramName, "SaveFile", result.ascii());
    m_pLabelDst->setText(result);
}

void QtRecompressorCtl::fileQuit()
{
    close();
}

void QtRecompressorCtl::detailsClicked()
{
    avm::string details = kernel->aboutStream(m_pComboStreams->currentItem());
    QMessageBox::information( this, tr( "About stream" ), details.c_str());
}

void QtRecompressorCtl::downFilterClicked()
{
    uint_t c = _lbFilters->currentItem();
    kernel->moveDownFilter(c);
    updateFilters();
    uint d = c + ((c + 1) < _lbFilters->count());
    _lbFilters->setCurrentItem( d );
    _lbFilters->setSelected( d, TRUE );
}

void QtRecompressorCtl::upFilterSelected()
{
    uint_t c = _lbFilters->currentItem();
    kernel->moveUpFilter(_lbFilters->currentItem());
    updateFilters();
    uint_t d = c - (c > 0);
    _lbFilters->setCurrentItem( d );
    _lbFilters->setSelected( d, TRUE );
}


void QtRecompressorCtl::fileLoadConfig()
{
    QString result = QFileDialog::getOpenFileName(avm::RegReadString(g_pcProgramName, "OpenConf", "./test.conf"),
						  0, 0, 0
#if QT_VERSION > 220
						  , tr( "Open configuration" )
#endif
						 );
    if(result.isNull())return;
    try
    {
	kernel->loadConfig(result.ascii());
	avm::RegWriteString(g_pcProgramName, "OpenConf", result.ascii());

	m_pLabelSrc->setText(kernel->getSrcFile());
	m_pLabelDst->setText(kernel->getDestFile());
	m_pLabelConfig->setText(result.ascii());

        m_pComboStreams->clear();
	for(unsigned i = 0; i < kernel->getStreamCount(); i++)
	    m_pComboStreams->insertItem(kernel->getStreamName(i).c_str());
        streamSelected(0);
//    kernel->setStreamMode(
        m_pComboStreams->setCurrentItem(0);
	RecKernel::KernelMode mode;
	if (kernel->getStreamMode(0, mode) == 0)
	    streamModeChanged(mode);
	updateFilters();
/*   switch(mode)
	    {
	    case Ignore:
		m_pRadioRemove->setChecked(true);
		break;
	    case Copy:
		m_pRadioCopy->setChecked(true);
		break;
	    case Recompress:
		m_pRadioRecompress->setChecked(true);
		break;
	    }
	}    */
        m_pRadioSeconds->setChecked(true);
        streamLimitModeChanged(0);
//	streamModeChanged(0);
        if(!wnd)
        {
	    ictl=new QImageControl(0);
	    ictl->show();
	    kernel->setImageControl(ictl);
	    wnd = new SourceControl(0, kernel);
	    wnd->show();
	}
	else
	{
	    kernel->setImageControl(ictl);
	    wnd->updateSize();
	}
    }
    catch (FatalError& e)
    {
	QMessageBox::information(this, e.GetModule(), e.GetDesc());
    }
}

void QtRecompressorCtl::removeFilterClicked()
{
    kernel->removeFilter(_lbFilters->currentItem());
    updateFilters();
}

void QtRecompressorCtl::fileSaveConfig()
{
    QString result = QFileDialog::getSaveFileName(avm::RegReadString(g_pcProgramName, "OpenConf", "./test.conf"),
						  0, 0, 0
#if QT_VERSION > 220
						  , tr( "Save configuration as" )
#endif
						 );
    if(result.isNull())return;
    try
    {
	kernel->saveConfig(result.ascii());
	avm::RegWriteString(g_pcProgramName, "OpenConf", result.ascii());
         m_pLabelConfig->setText(result.ascii());
    }
    catch(FatalError& e)
    {
	QMessageBox::information(this, e.GetModule(), e.GetDesc());
    }
}


void QtRecompressorCtl::streamFormatClicked()
{
    int stream = m_pComboStreams->currentItem();
    avm::VideoEncoderInfo info;
    AudioEncoderInfo ainfo;
    WAVEFORMATEX wfmtx;
    QavmCodecDialog* config;
    AudioCodecConfig* ac;
    if(!kernel->isStream(stream))
	return;
    switch(kernel->getStreamType(stream))
    {
    case avm::IStream::Video:
	if (kernel->getCompress(stream, info) <= 0)
	    return;//shouldn't happen.
	else
	{
	    int sel = 0;
	    int i = 0;
	    avm::vector<avm::CodecInfo> codecs;
	    avm::vector<avm::CodecInfo>::iterator it;
	    for (it=video_codecs.begin(); it!=video_codecs.end(); it++)
	    {
		if(!(it->direction & avm::CodecInfo::Encode))
		    continue;
		//printf("inserting: %x, %s\n", it->fourcc, it->GetName());
		codecs.push_back(*it);
		if((it->fourcc==info.compressor) && (info.cname==it->GetName()))
		    sel=i;
		i++;
	    }

	    config = new QavmCodecDialog(this, codecs);
	    config->setCurrent(sel);




	    if (config->exec() == QDialog::Accepted)
		kernel->setCompress(stream, config->getInfo());
	    delete config;
	}
	break;
    case avm::IStream::Audio:
	if (kernel->getAudioCompress(stream, ainfo) != 0)
	    return;//shouldn't happen.
	ac = new AudioCodecConfig(this, ainfo);
	if (ac->exec() == QDialog::Accepted)
	    kernel->setAudioCompress(stream, ac->GetInfo());
	delete ac;
        break;
    default:
        printf("QtRecompressorCtl::streamFormatClicked() unsupported stream type\n");
    }
}

void QtRecompressorCtl::streamLimitModeChanged(int)
{
    if(m_pRadioSeconds->isChecked())
    {
	m_pLineLowSec->setEnabled(true);
	m_pLineHighSec->setEnabled(true);
	m_pLineLowFrames->setEnabled(false);
	m_pLineHighFrames->setEnabled(false);
    }
    else
    {
    	m_pLineLowSec->setEnabled(false);
	m_pLineHighSec->setEnabled(false);
	m_pLineLowFrames->setEnabled(true);
	m_pLineHighFrames->setEnabled(true);
    }
}

void QtRecompressorCtl::streamModeChanged(int)
{
    int stream=m_pComboStreams->currentItem();
    if (!kernel->isStream(stream))
	return;
    if (m_pRadioRemove->isChecked())
    {
	m_pButtonFormat->setEnabled(false);
	kernel->setStreamMode(stream, RecKernel::Remove);
	m_pRadioSeconds->setEnabled(false);
	m_pRadioFrames->setEnabled(false);
	m_pLineLowSec->setEnabled(false);
	m_pLineHighSec->setEnabled(false);
	m_pLineLowFrames->setEnabled(false);
	m_pLineHighFrames->setEnabled(false);
    }
    else if (m_pRadioCopy->isChecked())
    {
    	m_pButtonFormat->setEnabled(false);
	kernel->setStreamMode(stream, RecKernel::Copy);
	m_pRadioSeconds->setEnabled(true);
	m_pRadioFrames->setEnabled(true);
	streamLimitModeChanged(0);
    }
    else
    {
    	m_pButtonFormat->setEnabled(true);
	kernel->setStreamMode(stream, RecKernel::Recompress);
	m_pRadioSeconds->setEnabled(true);
	m_pRadioFrames->setEnabled(true);
	streamLimitModeChanged(0);
    }
}

void QtRecompressorCtl::streamSelected(int s)
{
    RecKernel::KernelMode mode;
    if(kernel->getStreamMode(s, mode)!=0)
	return;
    framepos_t start, end;
    switch(mode)
    {
	case RecKernel::Remove:
	    m_pRadioRemove->setChecked(true);
	    break;
	case RecKernel::Copy:
	    m_pRadioCopy->setChecked(true);
	    break;
	case RecKernel::Recompress:
	    m_pRadioRecompress->setChecked(true);
	    break;
    }
    streamModeChanged(/*unused*/0);
    kernel->getSelection(s, start, end);
    char str[256];
    sprintf(str, "%.3f", start * kernel->getFrameTime(s));
    m_pLineLowSec->setText(str);
    sprintf(str, "%.3f", end * kernel->getFrameTime(s));
    m_pLineHighSec->setText(str);
    sprintf(str, "%d", int(start));
    m_pLineLowFrames->setText(str);
    sprintf(str, "%d", int(end));
    m_pLineHighFrames->setText(str);
}

void QtRecompressorCtl::startRecompress()
{
    if (!wnd)
	return;
    wnd->hide();
    hide();
    RecWindow w(this, kernel);
    w.update();
    w.exec();
    fprintf(stderr, "Recompress finished");
    wnd->show();
    show();
}

void QtRecompressorCtl::lo_sec_changed(const QString& s)
{
    if(no_recurs)return;
    int stream=m_pComboStreams->currentItem();
    if(!kernel->isStream(stream))
	return;
    if(kernel->getFrameTime(stream)==0)return;
    int start = int(atof(s.ascii())/kernel->getFrameTime(stream));
    char q[128];
    sprintf(q, "%d", start);
    no_recurs=1;
    m_pLineLowFrames->setText(q);
    no_recurs=0;
    kernel->setSelectionStart(stream, start);
}

void QtRecompressorCtl::hi_sec_changed(const QString& s)
{
    if(no_recurs)return;
    int stream=m_pComboStreams->currentItem();
    if(!kernel->isStream(stream))
	return;
    if(kernel->getFrameTime(stream)==0)return;
    int end = int(atof(s.ascii())/kernel->getFrameTime(stream));
    char q[128];
    sprintf(q, "%d", end);
    no_recurs=1;
    m_pLineHighFrames->setText(q);
    no_recurs=0;
    kernel->setSelectionEnd(stream, end);
}

void QtRecompressorCtl::lo_fr_changed(const QString& s)
{
    if(no_recurs)return;
    int stream=m_pComboStreams->currentItem();
    if(!kernel->isStream(stream))
	return;
    char q[128];
    sprintf(q, "%.3f", atoi(s.ascii())*kernel->getFrameTime(stream));
    no_recurs=1;
    m_pLineLowSec->setText(q);
    no_recurs=0;
    kernel->setSelectionStart(stream, atoi(s.ascii()));
}

void QtRecompressorCtl::hi_fr_changed(const QString& s)
{
    if(no_recurs)return;
    int stream=m_pComboStreams->currentItem();
    if(!kernel->isStream(stream))
	return;
    char q[128];
    sprintf(q, "%.3f", atoi(s.ascii())*kernel->getFrameTime(stream));
    no_recurs=1;
    m_pLineHighSec->setText(q);
    no_recurs=0;
    kernel->setSelectionEnd(stream, atoi(s.ascii()));
}

void QtRecompressorCtl::videoFilters()
{

}

void QtRecompressorCtl::optionsPreferences()
{

}

void QtRecompressorCtl::helpContents()
{

}

void QtRecompressorCtl::helpChangelog()
{

}
