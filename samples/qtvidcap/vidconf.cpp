#include "vidconf.h"
#include "vidconf.moc"

#include "v4lwindow.h"
#include "capproc.h"
#include "codecdialog.h"

#include <avm_fourcc.h>
#include <utils.h>
#include <avm_cpuinfo.h>
#include <avm_creators.h>
#define DECLARE_REGISTRY_SHORTCUT
#include <configfile.h>
#undef DECLARE_REGISTRY_SHORTCUT

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qfiledialog.h>
#include <qhgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qlistview.h>
#include <qmessagebox.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <qtabwidget.h>
#include <qvbuttongroup.h>
#include <qvgroupbox.h>

#include <stdio.h>
#include <stdlib.h> // atof

#include "v4lxif.h"

static const struct restab {
    int id;
    int w,h;
    const char * lname;
    const char * sname;
} cv_dimensions[] = {
    { 0, 160, 120, "160 x 120 (1/4 NTSC)", "160 x 120" },
    { 1, 192, 144, "192 x 144 (1/4 PAL)" , "192 x 144" },
    { 2, 320, 240, "320 x 240 (1/2 NTSC)", "320 x 240" },
    { 3, 384, 288, "384 x 288 (1/2 PAL)" , "384 x 288" },
    { 4, 400, 300, "400 x 300 (default)" , "400 x 300" },
    { 5, 512, 384, "512 x 384 (3/4 NTSC)", "512 x 384" },
    { 6, 576, 432, "576 x 432 (3/4 PAL)" , "576 x 432" },
    { 7, 640, 480, "640 x 480 (NTSC)"    , "640 x 480" },
    { 8, 768, 576, "768 x 576 (PAL)"     , "768 x 576" },
    { -1, 0,  5, NULL }, /* do not channge anything below */
    { -1, 0,  5, NULL },
    { -1, 0,  5, NULL },
};


/*
 *  Constructs a VidConfig which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
VidConfig::VidConfig( QWidget* parent, v4lxif* v4l, V4LWindow* w)
    : VidcapConfigDialog( parent, "Configure QtVidcap", true, 0 ), m_pV4lWin(w)
{
    _savename="";
    _regnumber=-1;

    _CapDevice->setText( RS("CaptureDevice", "/dev/video") );
    _AudDevice->setText( RS("AudioDevice", "/dev/dsp") );
    //ask hw
    //fixme
    for (int i = 0; cv_dimensions[i].id >= 0; i++)
	_Resolution->insertItem(cv_dimensions[i].lname);

    _Resolution->setCurrentItem( RI("Resolution", 0) );
    _VideoColorMode->setCurrentItem( RI("ColorMode", 0) );

    _VideoChannel->clear();
    if (v4l)
	for(int j=0; j<v4l->capCapChannelC(); j++)
	    _VideoChannel->insertItem(v4l->capChannelName(j));
    _VideoChannel->setCurrentItem( RI("Channel", 1) );
    _Colorspace->setCurrentItem( RI("Colorspace", 0) );
    _FileName->setText( RS("FileName", "./movie.avi") );

    cb_never_overwrite->setChecked(RI("NeverOverwriteFiles",0));

    QString regname = QString().sprintf("DirPool-%02d-Path", 0);
    QString pathname=RS(regname,"NONE");
    for (int i = 0; pathname!="NONE";)
    {
	regname=QString().sprintf("DirPool-%02d-KeepFree",i);
	int keepfree=RI(regname,500);

	regname=QString().sprintf("DirPool-%02d-Active",i);
	int ison=RI(regname,1);

	DirPoolItem *new_item=new DirPoolItem(lv_dirpool,pathname,keepfree);
	new_item->setOn(ison);
	new_item->updateSpaceLeft();

	i++;
	regname=QString().sprintf("DirPool-%02d-Path",i);
	pathname=RS(regname,"NONE");
    }

    _SegmentSize->setText( QString().sprintf("%d", RI("SegmentSize", 1000*1000)) );
    _HaveSegmented->setChecked( RI("IsSegmented", 1) );
    _HaveAudio->setChecked( RI("HaveAudio", 1) );
    _listChan->setCurrentItem( RI("SndChannels", 0) );
    _listFreq->setCurrentItem( RI("Frequency", 0) );
    _listSamp->setCurrentItem( RI("SampleSize", 0) );

    _info.compressor = RI("Compressor", fccDIV3);
    _info.cname = RS("Codec", "select");
    _info.quality = RI("Quality", 9500);
    _info.keyfreq = RI("Keyframe", 250);

    named_codecs_cb->setEditable(false);
    {
      int i=0;

      QString regname=QString().sprintf("NamedCodecs-%02d-Savename",i);
      QString nc_savename=RS(regname,"NONE");

      while(nc_savename!="NONE"){
	//named_codecs_cb->insertItem(nc_savename);

	i++;

	 regname=QString().sprintf("NamedCodecs-%02d-Savename",i);
	 nc_savename=RS(regname,"NONE");
      }

      QString named_codecs_current=RS("NamedCodecsCurrent","");

      if(named_codecs_current==""){
	_savename="";
	_regnumber=-1;
      }
      else{
	for(int j=0;j<named_codecs_cb->count();j++){
	  if(named_codecs_cb->text(j)==named_codecs_current){
	    named_codecs_cb->setCurrentItem(j);
	    _regnumber=j-1;
	    _savename=named_codecs_current;
	  }
	}
	printf("regnumber=%d, named_codecs_current=%s\n",_regnumber,named_codecs_current.latin1());
      }
    }

    updateCodecName();

    _chkTime->setChecked( RI("LimitTime", 0) );
    _chkFileSize->setChecked( RI("LimitSize", 0) );
    _sizeLimit->setText( QString().sprintf("%d", RI("SizeLimit", 2000000)) );
    _timeLimit->setText( QString().sprintf("%d", RI("TimeLimit", 3600)) );
    _fps->setEditText( QString().sprintf("%2.3f", RI("FPS", 25000) / 1000.0) );

    int shutdown_mode = RI("ShutdownMode",0);
    //cb_allow_shutdown->setChecked(RI("ShutdownAllow",0));

    switch (shutdown_mode)
    {
    case 0:
	rb_shutdown_never->setChecked(true);
	break;
    case 1:
	rb_shutdown_last->setChecked(true);
	break;
    case 2:
	rb_shutdown_inbetween->setChecked(true);
	break;
    default:
	printf("ShutdownMode invalid value\n");
	rb_shutdown_never->setChecked(true);
	break;
    }

    sb_min_timespan->setValue( RI("ShutdownMinTimespan",20) );
    sb_reboot_timespan->setValue( RI("ShutdownRebootTimespan",5) );
    sb_shutdown_grace->setValue( RI("ShutdownGraceTime",5) );

    cb_log->setChecked( RI("LogToFile",0) );

    if (RI("Password-Locked",0))
	bt_password_lock->setText(tr("press to unlock"));
    else
	bt_password_lock->setText(tr("press to lock"));

    connect(bt_password_set,SIGNAL(pressed()),this,SLOT(set_password()));
    connect(bt_password_lock,SIGNAL(pressed()),this,SLOT(lock_password()));

    connect(named_codecs_cb,SIGNAL(activated(int)),this,SLOT(changed_named_codec(int)));
}

/*
 *  Destroys the object and frees any allocated resources
 */
VidConfig::~VidConfig()
{
    // no need to delete child widgets, Qt does it all for us
}


void VidConfig::accept()
{
    savePage(0);
    savePage(1);
    savePage(2);
    savePage(3);
    savePage(4);
    savePage(5);

    // save only when OK, not APPLY
    avm::RegSave();

    if (m_pV4lWin) m_pV4lWin->setupDevice();
    QDialog::accept();
}

void VidConfig::apply()
{
#if QT_VERSION>=219
    savePage(tabWidget->currentPageIndex());
#else
    savePage(0);
    savePage(1);
    savePage(2);
    savePage(3);
    savePage(4);
    savePage(5);
#endif
    if (m_pV4lWin) m_pV4lWin->setupDevice();
}

void VidConfig::savePage(int page)
{
    switch(page)
    {
    case 0:
	WS("CapDev", _CapDevice->text());
	WS("AudioDevice", _AudDevice->text());
	WI("Resolution", _Resolution->currentItem());
	WI("Channel", _VideoChannel->currentItem());
	WI("ColorMode", _VideoColorMode->currentItem());
	WI("Colorspace", _Colorspace->currentItem());
	break;
    case 1:
	WS("FileName", _FileName->text());
	WI("NeverOverwriteFiles",cb_never_overwrite->isChecked());
	WI("SegmentSize", _SegmentSize->text());
	WI("IsSegmented", _HaveSegmented->isChecked());
	WI("FPS", int(atof(_fps->currentText()) * 1000));

	{
	    DirPoolItem *item=(DirPoolItem *)lv_dirpool->firstChild();
	    int i=0;

	    QString regname;
	    while(item!=NULL){
		regname=QString().sprintf("DirPool-%02d-Path",i);
		WS(regname,item->getPath());

		regname=QString().sprintf("DirPool-%02d-KeepFree",i);
		WI(regname,item->getKeepFree());

		regname=QString().sprintf("DirPool-%02d-Active",i);
		WI(regname,item->isOn());

		i++;
		item=(DirPoolItem *)item->nextSibling();
	    }

	    regname=QString().sprintf("DirPool-%02d-Path",i);
	    WS(regname,"NONE");

	}

	break;
    case 2:
	WI("Compressor", _info.compressor);
	WS("Codec", _info.cname);
	WI("Quality", _info.quality);
	WI("Keyframe", _info.keyfreq);
	WI("HaveAudio", _HaveAudio->isChecked());
	WI("SampleSize", _listSamp->currentItem());
	WI("Frequency", _listFreq->currentItem());
	WI("SndChannels", _listChan->currentItem());
	{
	  WS("NamedCodecsCurrent",_savename);
	  //WS("NamedCodecsCurrent-Compressor",_

	}
	break;
    case 3:
	WI("TimeLimit", _timeLimit->text());
	WI("SizeLimit", _sizeLimit->text());
	WI("LimitTime", _chkTime->isChecked());
	WI("LimitSize", _chkFileSize->isChecked());
	break;
    case 4:
	{
	    int shutdown_mode=rb_buttons->id(rb_buttons->selected());
	    printf("shutdown_mode =%d\n",shutdown_mode);

	    WI("ShutdownMode",shutdown_mode);
	    WI("ShutdownMinTimespan",sb_min_timespan->value());
	    WI("ShutdownRebootTimespan",sb_reboot_timespan->value());
	    WI("ShutdownGraceTime",sb_shutdown_grace->value());
	}
	break;
    case 5:
	WI("LogToFile",cb_log->isChecked());
	break;
    }

}
/*
 * public slot
 */
void VidConfig::change_filename()
{
    QString str = QFileDialog::getOpenFileName( _FileName->text(),
					       tr( "*.avi;*.AVI" ),
					       0, 0
#if QT_VERSION > 220
					       , tr( "Select destination file" )
#endif
					      );
    if (!str.isNull())
	_FileName->setText(str);
}
/*
 * public slot
 */
void VidConfig::change_codec()
{
    int bpp;
    int w = 768;
    int h = 576;

    switch(_Colorspace->currentItem())
    {
    case cspYV12:
	bpp = fccYV12;
	break;
    case cspYUY2:
	bpp = fccYUY2;
	break;
    case cspI420:
	bpp = fccI420;
	break;
    case cspRGB15:
	bpp = 15;
	break;
    case cspRGB24:
	bpp = 24;
	break;
    case cspRGB32:
	bpp = 32;
	break;
    default:
        bpp = 0;
    }

    int i = _Resolution->currentItem();
    if (i >= 0 && i < 9)
    {
	w = cv_dimensions[i++].w;
	h = cv_dimensions[i++].h;
    }


    QString savename=named_codecs_cb->currentText();
    int cin=named_codecs_cb->currentItem();
    printf("current item nr is %d\n",cin);
    //    if(named_codecs_cb->text(cin)!=savename){
    //printf("new???\n");
    int regnumber=-1;
    if(cin>0){

      regnumber=0;
      {

      QString regname=QString().sprintf("NamedCodecs-%02d-Savename",regnumber);
      QString nc_savename=RS(regname,"NONE");

      while(!(savename==nc_savename || nc_savename=="NONE")){
	regnumber++;

	 regname=QString().sprintf("NamedCodecs-%02d-Savename",regnumber);
	 nc_savename=RS(regname,"NONE");

      }

 


      if(nc_savename=="NONE"){
	WS(regname,savename);

	//WS("NamedCodecsCurrent",savename);
	named_codecs_cb->insertItem(savename);
	int newcount=named_codecs_cb->count();
	named_codecs_cb->setCurrentItem(newcount-1);
      }
      }
    }
    avm::string mycname;
    if(savename=="-Default-"){
      _savename="";
      mycname=_info.cname;
    }
    else{
      _savename=savename;
      mycname=RS(QString().sprintf("NamedCodecs-%02d-Codec",regnumber),"select");
    }

    printf("vidconfig: regnum=%d savename=%s mycname=%s\n",regnumber,_savename.c_str(),mycname.c_str());

    BITMAPINFOHEADER bih;
    bih.biCompression = 0xffffffff;
    // just to fill video_codecs list
    avm::CreateDecoderVideo(bih, 0, 0);

    // Select those codecs which accepts give format
    avm::BitmapInfo bi(w, h, bpp);
    avm::vector<avm::CodecInfo> codecs;
    avm::vector<avm::CodecInfo>::const_iterator it;
    int sel = 0;
    i = 0;
    for (it = video_codecs.begin(); it != video_codecs.end(); it++)
    {
      //printf("CHECKING %s  %x\n", it->GetName(), bpp);
	if (!(it->direction & avm::CodecInfo::Encode))
	    continue;
	avm::IVideoEncoder* enc = avm::CreateEncoderVideo(*it, bi);
	if (!enc)
	    continue;
	// checking if this code is the one we have asked for...
	bool ok = (strcmp(it->GetName(), enc->GetCodecInfo().GetName()) == 0);
	//printf("OK CHEK %d  %s  %s\n", ok, it->GetName(), enc->GetCodecInfo().GetName());
	avm::FreeEncoderVideo(enc);
	if (!ok)
	    continue;
	codecs.push_back(*it);
	if (mycname == it->GetName())
	    sel = i;
	i++;
    }

    //    ListCodecs(codecs);

    //printf("SEL %d\n", sel);
    QavmCodecDialog conf(this, codecs);
    conf.setCurrent(sel);

    int gx = RI("CodecGeometryX", conf.x());
    int gy = RI("CodecGeometryY", conf.y());
    int gw = RI("CodecGeometryWidth", conf.width());
    int gh = RI("CodecGeometryHeight", conf.height());
    int gm = RI("CodecGeometryMaximized", 0);

    conf.resize( gw, gh );
    conf.move( gx, gy);
    if (gm)
	conf.showMaximized();

    if (conf.exec() == QDialog::Accepted)
    {
	WI("CodecGeometryX", conf.x());
	WI("CodecGeometryY", conf.y());
	WI("CodecGeometryWidth", conf.width());
	WI("CodecGeometryHeight", conf.height());
#if QT_VERSION > 220
	WI("CodecGeometryMaximized", conf.isMaximized());
#endif

	if(regnumber>=0){
	  _named_info=conf.getInfo();
	

	  QString regname=QString().sprintf("NamedCodecs-%02d-Compressor",regnumber);
	  WI(regname,_named_info.compressor);

	  regname=QString().sprintf("NamedCodecs-%02d-Codec",regnumber);
	  WS(regname,_named_info.cname);

	  regname=QString().sprintf("NamedCodecs-%02d-Quality",regnumber);
	  WI(regname,_named_info.quality);

	  regname=QString().sprintf("NamedCodecs-%02d-Keyframe",regnumber);
	  WI(regname,_named_info.keyfreq);

	  _regnumber=regnumber;
	  printf("setting named compressor regnumber=%d to %s\n",regnumber,_named_info.cname.c_str());
	  //	  _savename=
	}
	else{
	  _info = conf.getInfo();
	  printf("normal\n");
	  //_savename="";
	}
        updateCodecName();

    }
}

void VidConfig::updateCodecName()
{
    char ft[4];
    char str[256];

      printf("_savename=%s\n",_savename.c_str());
    if(_savename==""){
      sprintf(str, "%s: %.4s", _info.cname.c_str(),
	      avm_set_le32(ft, _info.compressor));
    }
    else{
    
     int regnumber=0;
     QString regname=QString().sprintf("NamedCodecs-%02d-Savename",regnumber);
      QString nc_savename=RS(regname,"NONE");

      while(!(QString(_savename)==nc_savename || nc_savename=="NONE")){
	regnumber++;

	 regname=QString().sprintf("NamedCodecs-%02d-Savename",regnumber);
	 nc_savename=RS(regname,"NONE");
      }

      if(nc_savename=="NONE"){
	sprintf(str,"error - unknown");
      }
      else{
	printf("found regnumber: %d\n",regnumber);
      avm::string cname=RS(QString().sprintf("NamedCodecs-%02d-Codec",regnumber),"select");
      int compressor=RI(QString().sprintf("NamedCodecs-%02d-Compressor",regnumber),fccDIV3);
      sprintf(str, "%s: %.4s", cname.c_str(),
	      avm_set_le32(ft, compressor));
      }
      
    }
    _codecName->setText(str);
}
/*
 * public slot
 */
void VidConfig::toggle_audio(bool res)
{
    _listSamp->setEnabled(res);
    _listFreq->setEnabled(res);
    _listChan->setEnabled(res);
}
/*
 * public slot
 */
void VidConfig::toggle_segmented(bool res)
{
    _SegmentSize->setEnabled(res);
}

void VidConfig::toggle_limitsize(bool res)
{
    _sizeLimit->setEnabled(res);
}

void VidConfig::toggle_limittime(bool res)
{
    _timeLimit->setEnabled(res);
}

void VidConfig::dirpool_add(){
    QString path=le_dirpool_name->text();
    int keepfree=qs_dirpool_minfree->value();

    if(path[0]=='/'){
	DirPoolItem *new_item=new DirPoolItem(lv_dirpool,path,keepfree);
	int df=new_item->updateSpaceLeft();
    }
    else{
	printf("only absolute paths");
    }
}


void VidConfig::dirpool_rem(){
    DirPoolItem *item=(DirPoolItem *)lv_dirpool->selectedItem();

    if(item){
	lv_dirpool->takeItem(item);
	delete item;
    }
}

void VidConfig::lock_password(){
    QString password = RS("Password-Password","NONE");
    if (password == "NONE")
	return;

    int locked = RI("Password-Locked",0);
    if (locked)
    {
	//unlock now

	QMessageBox *pwd_dia=new QMessageBox(0);
	QVBoxLayout *lay=new QVBoxLayout(pwd_dia);
	lay->setAutoAdd(true);

	QVGroupBox *pwd_win=new QVGroupBox("Get Password",pwd_dia);
	QLineEdit *pwd_edit1=new QLineEdit(pwd_win);
	pwd_edit1->setEchoMode(QLineEdit::Password);

	pwd_dia->show();
	int res=pwd_dia->exec();

	if(pwd_edit1->text()==RS("Password-Password","NONE")){
	    //now unlock
	    WI("Password-Locked",0);
	    bt_password_lock->setText("press to lock");
	}

	delete pwd_edit1;
	delete pwd_win;
	delete pwd_dia;
    }
    else
    {
	//now lock
	WI("Password-Locked",1);
	bt_password_lock->setText("press to unlock");
    }

    m_pV4lWin->lock(RI("Password-Locked",0));

    bt_password_lock->setOn(false);
    bt_password_lock->setDown(false);
}


void VidConfig::set_password()
{
    QMessageBox *pwd_dia=new QMessageBox(0);
    QVBoxLayout *lay=new QVBoxLayout(pwd_dia);
    lay->setAutoAdd(true);

    QVGroupBox *pwd_win=new QVGroupBox("Set Password",pwd_dia);
    //  pwd_dia->setExtension(pwd_win);
    //lay->addWidget(pwd_dia);

    QLineEdit *pwd_edit1=new QLineEdit(pwd_win);
    QLineEdit *pwd_edit2=new QLineEdit(pwd_win);
    pwd_edit1->setEchoMode(QLineEdit::Password);
    pwd_edit2->setEchoMode(QLineEdit::Password);

    do{
	pwd_dia->show();
	int res=pwd_dia->exec();
    }while(pwd_edit1->text()!=pwd_edit2->text());

    WS("Password-Password",pwd_edit1->text());

    delete pwd_edit1;
    delete pwd_edit2;
    delete pwd_win;
    delete pwd_dia;

    bt_password_set->setOn(false);
    bt_password_set->setDown(false);
}

void VidConfig::changed_named_codec(int nr){
  _savename=named_codecs_cb->text(nr);
    if(_savename=="-Default-"){
      _savename="";
    }
    else{
      _savename=QString(_savename.c_str());
    }
    _regnumber=nr-1;

    updateCodecName();


    //    printf("changed codec to %s\n",global_savename.c_str());
}


void VidConfig::ListCodecs(avm::vector<avm::CodecInfo>& codec_list)
{
    avm::vector<avm::CodecInfo>::iterator it;
    int i=0;
    int sel=0;

    for (it = codec_list.begin(); it != codec_list.end(); it++)
    {
	if ( it->kind == avm::CodecInfo::DShow_Dec)
	    continue; //probably not a usable codec..

	if(!(it->direction & avm::CodecInfo::Encode))
	    continue;

	printf("%s\n", it->GetName());

	avm::vector<avm::AttributeInfo> encinfo = it->encoder_info;
	avm::vector<avm::AttributeInfo>::const_iterator inf_it;

	for(inf_it = encinfo.begin(); inf_it != encinfo.end(); inf_it++)
	{
	    switch(inf_it->kind)
	    {
	    case avm::AttributeInfo::Integer:
		{
		    int defval=0;
		    if (avm::CodecGetAttr(*it, inf_it->GetName(), &defval) == 0)
			printf("  %s=%d\n", inf_it->GetName(), defval);
		    else
			printf("  %s=(no default)\n", inf_it->GetName());

		    //avm::RegWriteInt("trala-"+_savename,inf_it->GetName(),defval);
		}
		break;
	    case avm::AttributeInfo::Select:
		{
		    int defval;
		    avm::vector<avm::string>::const_iterator sit;
		    avm::CodecGetAttr(*it, inf_it->GetName(), &defval);
                    printf("  %s = ", inf_it->GetName());
		    printf(" %s ", (defval < (int)inf_it->options.size()) ?
			   inf_it->options[defval].c_str() : "unknown");

		    printf(" (");
		    for (sit = (inf_it->options).begin(); sit != (inf_it->options).end(); sit++)
			printf("%s ", sit->c_str());

		    printf(")\n");
		}
		break;
	    case avm::AttributeInfo::String:
		{
		    const char* def_str;
		    avm::CodecGetAttr(*it, it->GetName(), &def_str);
		    printf(" %s = '%s'\n", inf_it->GetName(), def_str);
		}
		break;
	    }
	}

	i++;
    }
}
