/*
 EPG (Electronic Program Guide) window for AviCap

 written 2003 by Alexander Rawass (alexannika@users.sourceforge.net)
 */


#include <unistd.h>
#ifndef WIN32
#include <pwd.h>
#endif

#include "vidconf.h"
#include "v4lwindow.h"
#include "capproc.h"
#include "codecdialog.h"

#include <avm_fourcc.h>
#include <avm_cpuinfo.h>
#include <avm_creators.h>
#include <avm_output.h>
#include <utils.h>
#define DECLARE_REGISTRY_SHORTCUT
#include <configfile.h>
#undef DECLARE_REGISTRY_SHORTCUT

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qfiledialog.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qtabwidget.h>
#include <qlistview.h>
#include <qcombobox.h>
#include <qsplitter.h>
#include <qgroupbox.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qbuttongroup.h>
#include <qmessagebox.h>
#include <qpainter.h>
#include <qhgroupbox.h>
#include <qvgroupbox.h>
#include <qhbox.h>
#include <qvbox.h>
#include <qcolordialog.h>
#include <qstringlist.h>
#include <qregexp.h>
#include <qpopupmenu.h>
#include <qtextstream.h>

#include <qdom.h>

#include <stdio.h>
#include <stdlib.h> // atof
#include <qtabwidget.h>

#include <stdio.h>
#include <stdlib.h> // atof
#include <sys/vfs.h> //statfs
#include <time.h>

#if QT_VERSION>=300
#include <qtextedit.h>
#endif

#if 1

//#include <qdatetimeedit.h>
#include "epgwindow.h"
#include "timertable.h"

#include <qtable.h>

int calcTimeDiff(){

    struct tm local;
    time_t absolute;

    time(&absolute);
    localtime_r(&absolute,&local);

    //printf("gmtoff=%ld \n",local.tm_gmtoff);
    return local.tm_gmtoff;

}

void EpgWindow::lock(bool lock){
    conf_dir_edit->setReadOnly(lock);
    regexp_edit->setReadOnly(lock);
    if(lock){
	conf_channel_view->hide();
    }
    else{
	conf_channel_view->show();
    }
}

void EpgWindow::addEntry(QDateTime start_date,QDateTime stop_date,QString channel_str,QString title,QString desc){
    //  printf("addEntry %s\n",title.latin1());
    EpgTableItem *item=new EpgTableItem(ptable,this,start_date,stop_date,channel_str, title, desc);

    QDate sdate=start_date.date();

    EpgProgram *new_program=new EpgProgram();
    new_program->start=start_date;
    new_program->stop=stop_date;
    new_program->title=title;
    new_program->desc=desc;
    new_program->blockmode=regexp_unset;

    bool stored_program=false;
    while(!stored_program){
	bool found_day=false;

	//iterate over dates
	//EpgDayList::iterator id;
	QValueListIterator<EpgDay *> id;
	for(id=epglist.begin();!found_day && id!=epglist.end();id++){
	    EpgDay *day=*id;
	    if(sdate==day->date){
		found_day=true;
		//we iterate over channels

		bool found_channel=false;
		//EpgChannelList::iterator ic;
		EpgChannelListIterator ic;
		for(ic=day->channels.begin();!found_channel && ic!=day->channels.end();ic++){
		    EpgChannel *ch=*ic;
		    if(ch->chid==channel_str){
			found_channel=true;

			new_program->channelnr=ch->channelnr;
			ch->programs.append(new_program);
			stored_program=true;
			//found=true;
		    }
		}
		if(!found_channel){
		    // add a new channel

		    //printf("creating new channel %s (%s) in day %s\n",channel_str.latin1(),findNameToId(channel_str).latin1(),day->date.toString().latin1());
		    EpgChannel *newch=new EpgChannel();
		    newch->chid=channel_str;
		    day->channels.append(newch);

		    //search for the EpgChannelId
		    EpgChIdListIterator icht;
		    bool found_chid=false;
		    for(icht=epgChannels.begin();!found_chid && icht!=epgChannels.end();icht++){
			EpgIdChannel *epgidch=*icht;

			if(epgidch->epgid==newch->chid){
			    found_chid=true;
			    newch->channelnr=epgidch->chnr;
			    newch->idchannel=epgidch;
			}
		    }
		}
	    }
	}
	if(!found_day){
	    // add a new day
	    //printf("creating a new day %s\n",sdate.toString().latin1());
	    EpgDay *newday=new EpgDay();
	    newday->date=sdate;
	    epglist.append(newday);

	    QString daydesc=sdate.toString(epg_dateformat);
	    if(QDate::currentDate()==sdate){
		daydesc=daydesc+tr(" Today");
	    }
	    day_selector->insertItem(daydesc);
	    int itemnr=day_selector->count()-1;
	    //printf("day got nr %d\n",itemnr);
	    newday->itemnr=itemnr;
	    if(QDate::currentDate()==sdate){
		today_item=itemnr;
	    }
	    //    day_selector->changeItem(QString().sprintf("%d",itemnr)+daydesc,itemnr);
	}
    }//end while !stored_program

}


void EpgWindow::doChannel(QDomElement &elem){
    //  printElem(elem);

    EpgIdChannel *new_channel=new EpgIdChannel();

    new_channel->epgid=elem.attribute("id");
    if(new_channel->epgid.isNull()){
	printf("didnt get attribute in channel\n");
	delete new_channel;
	return;
    }

    QDomNode n=elem.firstChild();

    while(!n.isNull()){
	QDomElement e=n.toElement();
	if(!e.isNull()){
	    if(e.tagName()=="display-name"){
		new_channel->epgname=e.text();
	    }
	    else{
		//ignore
	    }
	}
	n=n.nextSibling();
    }


    epgChannels.append(new_channel);

    QCheckListItem *item=new QCheckListItem(ctable,"test",QCheckListItem::CheckBox);
    item->setText(0,new_channel->epgname);
#if 0
    EpgChannelStrip *newstrip=new EpgChannelStrip(gridview,this,new_channel);
    gridlayout->addWidget(newstrip);
    //  newstrip->repaint();

#endif

    new_channel->strip=NULL;
    QHBox *hbox=new QHBox(conf_channel_view);
#if 0
    new_channel->conf_channel_label=new QLabel(tr("Provider ")+provider+" EPGid "+new_channel->epgid+" "+new_channel->epgname+" is ",hbox);
#endif

    new_channel->conf_channel_label=new QLabel("EPGid "+new_channel->epgid+" ("+new_channel->epgname+") is ",hbox);
    //  conf_channel_lay->addWidget(new_channel->conf_channel_label,channel_row,0);

    new_channel->conf_channel_box=new QComboBox(hbox);
    //  conf_channel_lay->addWidget(new_channel->conf_channel_box,channel_row,1);

    new_channel->conf_channel_box->insertItem(tr("not available"));
    for(int i=0;i<ACnumChannels();i++){
	new_channel->conf_channel_box->insertItem(QString().sprintf("%02d - %s",i+1,ACchannelName(i).latin1()));
    }
#if 0
    int chnr=RI(QString().sprintf("Epg-%s-%s",provider.latin1(),new_channel->epgid.latin1()),0);
#endif
    int chnr=RI(QString().sprintf("Epg-%s-%s","Provider",new_channel->epgid.latin1()),0);

    new_channel->conf_channel_box->setCurrentItem(chnr);

    new_channel->chnr=chnr;

    //connect(new_channel->conf_channel_box,SIGNAL(activated(int)),this,channelMappingChanged(int));

    QPushButton *color_button=new QPushButton(tr("Color"),hbox);
    //  color_button->setToggleButton(false);

    new_channel->color_button=color_button;

    QString colstr=RS(QString().sprintf("Epg-%s-%s-Color","Provider",new_channel->epgid.latin1()),"#DDDDDD");
    QColor color=QColor(colstr);
    new_channel->color=color;
#if QT_VERSION>=300
    color_button->setBackgroundMode(Qt::PaletteBackground);
    //color_button->setBackgroundMode(Qt::FixedColor);
    color_button->setPaletteBackgroundColor(color);
#else
#warning "no color_button->setBackgroundMode(Qt::PaletteBackground)"
#endif

    connect(color_button,SIGNAL(pressed()),this,SLOT(colorSelector()));

    channel_row++;
}

#if 0
void EpgWindow::channelMappingChanged(int nr){
}
#endif

QDateTime EpgWindow::parseDate(QString datestr){

    QString yearstr=datestr.mid(0,4);
    QString monstr=datestr.mid(4,2);
    QString daystr=datestr.mid(6,2);

    QString hourstr=datestr.mid(8,2);
    QString minstr=datestr.mid(10,2);
    QString secstr=datestr.mid(12,2);

    QDateTime datetime;

    datetime.setDate(QDate(yearstr.toInt(),monstr.toInt(),daystr.toInt()));
    datetime.setTime(QTime(hourstr.toInt(),minstr.toInt(),secstr.toInt()));

    //allow for localtime

    datetime=datetime.addSecs(local_timediff);

    //  printf("parsed %s to %s\n",datestr.latin1(),datetime.toString().latin1());

    return datetime;
}

void EpgWindow::doProgramme(QDomElement &elem){
    //  printElem(elem);

    QString start_str=elem.attribute("start");
    QString stop_str=elem.attribute("stop");
    QString channel_str=elem.attribute("channel");

    if(start_str.isNull() || stop_str.isNull() || channel_str.isNull()){
	printf("didnt get attribute in programme\n");
	return;
    }

    QDateTime start_date=parseDate(start_str);
    QDateTime stop_date=parseDate(stop_str);
    QString title;
    QString desc;

    QDomNode n=elem.firstChild();

    while(!n.isNull()){
	QDomElement e=n.toElement();
	if(!e.isNull()){
	    if(e.tagName()=="title"){
		title=e.text();
	    }
	    else if(e.tagName()=="desc"){
		desc=e.text();
	    }
	    else{
		//ignore
	    }
	}
	n=n.nextSibling();
    }

    addEntry(start_date,stop_date,channel_str,title,desc);
}

void EpgWindow::printElem(QDomElement &elem){
    printf("tagname=%s ",elem.tagName().latin1());

    QDomNamedNodeMap attrmap=elem.attributes();

    for(uint i=0; i<attrmap.length();i++){
	QDomNode node=attrmap.item(i);
	printf("%s=%s  ",node.nodeName().latin1(),node.nodeValue().latin1());
    }
    printf("\n");
}

void EpgWindow::parse(QDomDocument &dom){
    QDomElement tv_elem=dom.documentElement();

    printElem(tv_elem);
    if(tv_elem.tagName()!="tv"){
	printf("error: node tv expected\n");
	return;
    }

    QDomNode n=tv_elem.firstChild();

    while(!n.isNull()){
	QDomElement e=n.toElement();
	if(!e.isNull()){
	    if(e.tagName()=="channel"){
		doChannel(e);
	    }
	    else if(e.tagName()=="programme"){
		doProgramme(e);
	    }
	    else{
		printf("unexpected tag\n");
		printElem(e);
		return;
	    }
	}
	n=n.nextSibling();
    }
}

void EpgWindow::load(){
    load(RS("Epg-ProviderFilename","/tmp/epg.xml"));
}

void EpgWindow::load(QString fname){
    printf("loading epg xml file\n");

    QDomDocument epgxml("epg xml");
    QFile epgfile(fname);
    if(!epgfile.open(IO_ReadOnly)){
	epgfile.close();
	printf("could not openfile\n");
	return;
    }
    printf("before setcontent\n");
    if( !epgxml.setContent(&epgfile)){
	epgfile.close();
	return;
    }
    printf("after setcontent\n");
    epgfile.close();

    provider=fname;

    printf("before parse\n");
    parse(epgxml);
    printf("after parse\n");

}

void EpgWindow::initWidgets(){
    setCaption(tr("EPG Browser"));
    setBaseSize(600,600);
    //setMaximumSize(800,700);

    QWidget *topwin=this;
    //    QVBoxLayout *vb1=new QVBoxLayout(topwin->layout());
    QHBoxLayout *lay1=new QHBoxLayout(topwin);

    ctable=new QListView(topwin);
    lay1->addWidget(ctable);

    ctable->addColumn(tr("Channel"));
    ctable->setMaximumSize(150,1000);
    ctable->setAllColumnsShowFocus(true);
    ctable->setShowSortIndicator(true);
    ctable->setSorting(0,true);
    ctable->sort();

    centralw=new QWidget(topwin);
    lay1->addWidget(centralw);
    QVBoxLayout *lay4=new QVBoxLayout(centralw);

    QWidget *topstrip=new QWidget(centralw);
    lay4->addWidget(topstrip);
    QHBoxLayout *lay6=new QHBoxLayout(topstrip);
    lay6->setMargin(5);
    lay6->setSpacing(5);

    day_selector=new QComboBox(topstrip);
    lay6->addWidget(day_selector);

    zoom_in_button=new QPushButton(topstrip);
    zoom_in_button->setText(tr("ZoomIn"));
    lay6->addWidget(zoom_in_button);

    zoom_out_button=new QPushButton(topstrip);
    zoom_out_button->setText(tr("ZoomOut"));
    lay6->addWidget(zoom_out_button);

    display_toggle=new QPushButton(topstrip);
    display_toggle->setText(tr("toggle display"));
    lay6->addWidget(display_toggle);

    config_button=new QPushButton(topstrip);
    config_button->setText(tr("EPG Config"));
    lay6->addWidget(config_button);

    regexp_button=new QPushButton(topstrip);
    regexp_button->setText(tr("Edit Regexps"));
    lay6->addWidget(regexp_button);

    //  QVBoxLayout *lay5=new QVBoxLayout(centralw);

    gridtable=new QTable(centralw);
    lay4->addWidget(gridtable);
#if 0
    gridscroll=new QScrollView(centralw);
    lay4->addWidget(gridscroll);
    QHBoxLayout *lay3=new QHBoxLayout(gridscroll->viewport());

    gridheader=new QHeader(gridscroll);
    gridheader->setOrientation(Qt::Horizontal);
    //  gridscroll->addChild(gridheader);

    lay3->addWidget(gridheader);

    gridview=new QWidget(gridscroll->viewport());
    gridscroll->addChild(gridview);
    lay3->addWidget(gridview);

    //gridview->setMinimumSize(300,strip_y);
    //gridscroll->setMaximumSize(600,strip_y);
    //gridscroll->resizeContents(2000,strip_y);
    //gridscroll->viewport()->resize(1000,strip_y);
    gridscroll->setResizePolicy(QScrollView::AutoOneFit);
    gridscroll->setVScrollBarMode(QScrollView::AlwaysOn);
    gridscroll->setHScrollBarMode(QScrollView::AlwaysOn);

    gridlayout=new QHBoxLayout(gridview);
    gridlayout->setMargin(5);
    gridlayout->setSpacing(5);
#endif

    ptable=new QListView(centralw);
    lay4->addWidget(ptable);

    ptable->addColumn(tr("Title"));
    ptable->addColumn(tr("Channel"));
    ptable->addColumn(tr("Start Date"));
    ptable->addColumn(tr("Stop Date"));
    ptable->addColumn(tr("Description"));
    ptable->setAllColumnsShowFocus(true);
    ptable->setShowSortIndicator(true);
    ptable->setSorting(2,true);
    ptable->sort();
    ptable->hide();
#if 0
    bottom_label=new QLabel("Title\nDescription\n",centralw);
    bottom_label->setAlignment(Qt::WordBreak);
    lay4->addWidget(bottom_label);
#endif
    bottom_text=new QTextEdit(centralw);
#if QT_VERSION>=300
    bottom_text->setTextFormat(Qt::PlainText);
#endif
    bottom_text->setReadOnly(TRUE);
    lay4->addWidget(bottom_text);

    //show();

    //  resizeStrips();

    connect(day_selector,SIGNAL(activated(int)),this,SLOT(day_selected(int)));
    connect(display_toggle,SIGNAL(pressed()),this,SLOT(display_toggled()));
    connect(config_button,SIGNAL(pressed()),this,SLOT(show_config()));
    connect(regexp_button,SIGNAL(pressed()),this,SLOT(show_regexps()));

    connect(timertable,SIGNAL(currentRecordingChanged(TimerTableItem *)),this,SLOT(setCurrentRecording(TimerTableItem *)));
    connect(timertable,SIGNAL(nextRecordingChanged(TimerTableItem *)),this,SLOT(setNextRecording(TimerTableItem *)));
    connect(timertable,SIGNAL(timertableChanged()),this,SLOT(timertableModified()));

    connect(zoom_in_button,SIGNAL(pressed()),this,SLOT(zoomIn()));
    connect(zoom_out_button,SIGNAL(pressed()),this,SLOT(zoomOut()));

    display_mode=0;
}

void EpgWindow::show_config(){
    config_widget->show();
}

void EpgWindow::show_regexps(){
    printf("showing regexps\n");
    regexp_box->show();
}

void EpgWindow::display_toggled(){
    if(display_mode==0){
	ptable->show();
	gridtable->hide();
	display_mode=1;
    }
    else if(display_mode==1){
	ptable->hide();
	gridtable->show();
	display_mode=0;
    }
}

void EpgWindow::day_selected(int nr){
    if(current_date_item!=nr){
	//a new day selected
	regexp_dirty=true;
    }
    else{
	//just a normal redraw
    }

    current_date_item=nr;
    //printf("selected day %d\n",nr);
    day_selector->setCurrentItem(nr);
#if 0
    gridview->repaint();
    gridview->update();
#endif
    //  gridtable->repaint();
    //gridtable->update();

    resizeStrips();

    regexp_dirty=false;
}

void EpgWindow::resizeStrips(){
    calcStripSize();

    //  int strip_y=strip_y_size;

    EpgChIdListIterator it;
    for(it=epgChannels.begin();it!=epgChannels.end();it++){
	EpgIdChannel *idch=*it;
	if(idch->strip!=NULL){
	    //printf("check1\n");
	    gridtable->setColumnWidth(idch->chnr-1,strip_x);
	    //printf("check2\n");
	    gridtable->setRowHeight(idch->chnr-1,strip_y_size);
	    //printf("check3\n");
	    idch->strip->setFixedSize(strip_x,strip_y_size);
	    //printf("check4\n");
	    //idch->strip->resize(strip_x,strip_y_size);
	    //idch->strip->updateGeometry();
	    idch->strip->repaint();
	}
    }

}

void EpgWindow::makeChannelStrips(){
    gridtable->setNumRows(1);
    gridtable->setNumCols(ACnumChannels());
    gridtable->setVScrollBarMode(QScrollView::AlwaysOn);
    gridtable->setHScrollBarMode(QScrollView::AlwaysOn);
    gridtable->setRowHeight(0,strip_y_default);

    for(int i=1;i<ACnumChannels();i++){
	gridtable->horizontalHeader()->setLabel(i-1,ACchannelName(i-1));
	EpgChIdListIterator ic;
	bool found_channel=false;
	for(ic=epgChannels.begin();!found_channel && ic!=epgChannels.end();ic++){
	    EpgIdChannel *channelid=*ic;
	    if(channelid->chnr==i){
		found_channel=true;
#if 0
		EpgChannelStrip *newstrip=new EpgChannelStrip(gridview,this,channelid);
		gridlayout->addWidget(newstrip);
#endif

		EpgChannelStrip *newstrip=new EpgChannelStrip(gridtable,this,channelid);
		gridtable->setCellWidget(0,i-1,newstrip);
		gridtable->setColumnWidth(i-1,strip_x_default);

		channelid->strip=newstrip;

		//,strip_x);
	    }
	}
    }

}

void EpgWindow::addRecording(EpgProgram *prog){
    if(prog){
	emit emitAddRecording(prog);
    }
}

void EpgWindow::timertableModified(){
    day_selected(current_date_item);
}

void EpgWindow::setNextRecording(TimerTableItem *titem){
    next_recording=titem;
    day_selected(current_date_item);
}

void EpgWindow::setCurrentRecording(TimerTableItem *titem){
    current_recording=titem;
    day_selected(current_date_item);
}

void EpgWindow::zoomIn(){
    zoom_step++;
    day_selected(current_date_item);
}
void EpgWindow::zoomOut(){
    zoom_step--;
    day_selected(current_date_item);
}

void EpgWindow::timer(){
    day_selected(current_date_item);
}

bool EpgWindow::isBlocked(int channelnr){
    EpgDay *day=NULL;
    EpgDayListIterator id;
    QDateTime currentdatetime=QDateTime::currentDateTime();

    for(id=epglist.begin(); id!=epglist.end();id++){
	day=*id;

	bool found_channel=false;
	EpgChannelListIterator ic;
	for(ic=day->channels.begin();!found_channel && ic!=day->channels.end();ic++){
	    EpgChannel *ch=*ic;
	    if(ch->channelnr==channelnr){
		found_channel=true;

		bool found_prog=false;
		EpgProgramListIterator ip;
		for(ip=ch->programs.begin();!found_prog && ip!=ch->programs.end();ip++){
		    EpgProgram *prog=*ip;

		    if(prog->start<currentdatetime && currentdatetime<prog->stop){
			//current program
			if(ch->idchannel->strip->blockMode(prog)== regexp_block){
			    return true;
			}
		    }
		}//for programs
	    }//if channel
	}//for channels
    }//for all days

    return false;

}

EpgWindow::EpgWindow(V4LWindow *v4l,TimerTable *timer) : QWidget(0) {
    v4lwin=v4l;
    timertable=timer;
    channel_row=0;
    today_item=-1;
    regexp_dirty=true;
    current_recording=NULL;
    next_recording=NULL;
    zoom_step=0;
    strip_y_size=strip_y_default;

    avml(AVML_DEBUG, "epgwindow started\n");

    local_timediff=calcTimeDiff();
    //printf("timediff is %ld\n",local_timediff);

    initWidgets();
    makeConfigWidget(0);

    load();
    makeChannelStrips();
    conf_channel_view->updateGeometry();
    conf_channel_scroll->updateGeometry();
    conf_channel_view->adjustSize();

    current_date_item=-1;

    day_selected(today_item);
    //  config_widget->show();

    epg_timer=new QTimer(this);
    connect(epg_timer,SIGNAL(timeout()),SLOT(timer()));
    epg_timer->start(1000*60); //every minute

}


EpgWindow::~EpgWindow()
{
}

QString EpgWindow::findNameToId(QString channel){
    EpgChIdListIterator it;
    for(it=epgChannels.begin();it!=epgChannels.end();it++){
	EpgIdChannel *idch=*it;
	if(idch->epgid==channel){
	    return idch->epgname;
	}
    }
    return "noname";
}

EpgTableItem::~EpgTableItem()
{
}

EpgTableItem::EpgTableItem(QListView *parent,EpgWindow *epgwindow,QDateTime start,QDateTime stop,QString channelstr,QString titlestr,QString descstr) : QCheckListItem(parent,"test",QCheckListItem::CheckBox){

    start_date=start;
    stop_date=stop;
    channel=channelstr;
    title=titlestr;
    desc=descstr;
    epgwin=epgwindow;

    QString startstr=start_date.toString(epg_startdateformat);
    QString stopstr=stop_date.toString(epg_stopdateformat);

    chname=epgwin->findNameToId(channel);

    setText(0,title);
    setText(1,chname);
    setText(2,startstr);
    setText(3,stopstr);
    setText(4,desc);
}

int EpgTableItem::compare(QListViewItem *other,int col, bool ascend) const {
    if(other==NULL){
	return 0;
    }

    EpgTableItem *other_tti=(EpgTableItem *)other;
    int result=0;

    if(col==2){
	//start time
	if(getStartTime()==other_tti->getStartTime()){
	    result=0;
	}
	else{
	    result=(getStartTime()>other_tti->getStartTime()) ? -1 : 1 ;
	}
    }
    else if(col==3){
	//stop time
	if(getStopDateTime()==other_tti->getStopDateTime()){
	    result=0;
	}
	else{
	    result=(getStopDateTime()>other_tti->getStopDateTime()) ? -1 : 1;
	}
    }
    else{
#if QT_VERSION>=300
	result=QListViewItem::compare(other,col,ascend);
#else
#warning "no QListViewItem::compare"
#endif
    }

    return result;
}


EpgChannelStrip::EpgChannelStrip(QWidget *parent,EpgWindow *win,EpgIdChannel *new_channel) : QWidget(parent){

    //  printf("strip created for %s\n",new_channel->epgname.latin1());

    //setMinimumSize(strip_x,strip_y);
    //setBaseSize(strip_x,strip_y);
    setFixedSize(strip_x_default,strip_y_default);
    setBackgroundColor(white);
    resize(strip_x_default,strip_y_default);
    updateGeometry();
    setMouseTracking(true);

    chid=new_channel;
    epgwin=win;
    last_mouse_program=NULL;
    popmenu=NULL;
    mid_move=false;

    //  QVBoxLayout *lay1=new QVBoxLayout(this);

    //QHeader *head=new QHeader(this);
    //lay1->addWidget(head);

    //head->addLabel(new_channel->epgname);

    show();
}

int EpgChannelStrip::blockMode(EpgProgram *prog){
    int j=0;
    EpgRegexpListIterator it;

#if QT_VERSION<300
#define exactMatch(str) match(str)
#endif

    for(it=epgwin->regexpList.begin();it!=epgwin->regexpList.end();it++){
	EpgRegexp *epgreg=*it;

	if(epgreg->blockmode==regexp_errorblock || epgreg->wheremode==regexp_errorwhere){
	    return regexp_block;
	}
	if(epgreg->wheremode==regexp_both){
	    if(epgreg->regexp.exactMatch(prog->title) || epgreg->regexp.exactMatch(prog->desc)){
		return epgreg->blockmode;
	    }
	}
	else if(epgreg->wheremode==regexp_title){
	    if(epgreg->regexp.exactMatch(prog->title)){
		return epgreg->blockmode;
	    }
	}
	else if(epgreg->wheremode==regexp_desc){
	    if(epgreg->regexp.exactMatch(prog->desc)){
		return epgreg->blockmode;
	    }
	}
    }
    return regexp_nomatch;

#if QT_VERSION<300
#undef exactMatch
#endif
}

void EpgWindow::calcStripSize(){
    strip_x=strip_x_default+(int)(((float)strip_x_default)*(zoom_increment*zoom_step));
    strip_y=strip_y_default+(int)(((float)strip_y_default)*(zoom_increment*zoom_step));

    int pixel_per_hour=strip_y_default/24;
    hours_before=RI("EpgHoursBeforeDay",0);
    hours_after=RI("EpgHoursAfterDay",0);

    strip_y_before=0;
    strip_y_after=0;
    strip_y_today=strip_y_default;

    if(hours_before<0){
	strip_y_before=-hours_before*60;
    }
    else{
	strip_y_today-=hours_before*60;
    }
    if(hours_after>0){
	strip_y_after=hours_after*60;
    }
    else{
	strip_y_today-=-hours_after*60;
    }

    strip_y_alldays=strip_y_before+strip_y_today+strip_y_after;

    y_subtract=0;
    if(hours_before>0){
	y_subtract=hours_before*60;
    }

    strip_y_size=strip_y_alldays;
}

void EpgChannelStrip::calcYfromTime(int &y_start,int &y_stop,QTime starttime,QTime stoptime){
    y_start=(starttime.hour()*60)+starttime.minute();

    y_stop=(stoptime.hour()*60)+stoptime.minute();

    if(y_stop<y_start){
	y_stop+=60*24;
    }

}

void EpgChannelStrip::drawStrip(int mx,int my){
    QColor colorNow=QColor(255,255,160); //light yellow
    QColor colorMouse=QColor(230,140,140); //brownish
    QColor colorBlock=QColor(100,100,100); //dark grey
    QColor colorHigh=QColor(220,170,230); // purple
    QColor colorTimer=QColor(0,0,255); //blue
    QColor colorNextRec=QColor(0,255,0); //green
    QColor colorCurRec=QColor(255,0,0); //red

    epgwin->calcStripSize();

    int strip_x=epgwin->strip_x;
    int strip_y=epgwin->strip_y;
    int hours_after=epgwin->hours_after;
    int hours_before=epgwin->hours_before;

    int strip_y_before=epgwin->strip_y_before;
    int strip_y_today=epgwin->strip_y_today;
    int strip_y_after=epgwin->strip_y_after;
    int strip_y_alldays=epgwin->strip_y_alldays;
    int strip_y_size=epgwin->strip_y_size;
    int y_subtract=epgwin->y_subtract;

    if(my!=-1){
	if(last_mouse_program){
	    if(last_mouse_program->y_start<=my && my<=last_mouse_program->y_stop-1){
		//	printf("returned early\n");
		return;
	    }
	}
    }
    QPainter paint(this);
    paint.setWindow(0,0,strip_x,strip_y_alldays);
    if(my==-1){
	paint.eraseRect(0,-20,strip_x,strip_y_alldays);

	paint.setPen(Qt::black);
	//paint.drawText(0,20,chid->epgname);
	paint.lineTo(0,0);
	paint.lineTo(strip_x,strip_y_alldays);
	paint.lineTo(0,strip_y_alldays);
	paint.lineTo(strip_x,0);
    }

    EpgProgram *mouse_program=NULL;

    int  today=epgwin->current_date_item;
    int yesterday=today-1;
    int tomorrow=today+1;

    int startday=yesterday;
    int stopday=tomorrow;
    if(hours_before>0){
	startday=today;
    }
    if(hours_after<0){
	stopday=today;
    }
    //printf("yesterday=%d today=%d tomorrow=%d\n",yesterday,today,tomorrow);
    //printf("hours_before=%d hours_after=%d\n",hours_before,hours_after);
    //printf("startday=%d stopday=%d\n",startday,stopday);
    //printf("strip_y_before=%d strip_y_today=%d strip_y_after=%d\n"
    //	 strip_y_before,strip_y_today,strip_y_after);
    //printf("strip_y_alldays=%d\n",strip_y_alldays);

    for(int daynr=startday;daynr<=stopday;daynr++){

	bool found_day=false;
	EpgDay *day=NULL;
	//printf("drawing strip %s for day %d\n",chid->epgname.latin1(),daynr);
	EpgDayListIterator id;
	for(id=epgwin->epglist.begin();!found_day && id!=epgwin->epglist.end();id++){
	    day=*id;
	    if(day->itemnr==daynr){
		found_day=true;
	    }
	}

	if(!found_day){
	    printf("ERROR: day not found\n");
	    return;
	}

	bool found_channel=false;
	int row=0;
	printf("drawing day %s nr=%d  channel %s\n",day->date.toString().latin1(),day->itemnr,chid->epgname.latin1());

	EpgChannelListIterator ic;
	for(ic=day->channels.begin();!found_channel && ic!=day->channels.end();ic++){
	    EpgChannel *ch=*ic;
	    if(ch->chid==chid->epgid){
		found_channel=true;
		//printf("found my channel\n");

		//paint.drawText(0,0,chid->epgname);
		//we iterate over programs

		bool found_prog=false;
		EpgProgramListIterator ip;
		for(ip=ch->programs.begin();!found_prog && ip!=ch->programs.end();ip++){
		    EpgProgram *prog=*ip;

		    QColor color=chid->color;

		    QTime starttime=prog->start.time();
		    QTime stoptime=prog->stop.time();
		    int y_start,y_stop;

		    calcYfromTime(y_start,y_stop,starttime,stoptime);

		    int drawit=false;
		    if(daynr==yesterday && starttime.hour()>(24+hours_before)){
			y_start=y_start-(strip_y_default-strip_y_before);
			y_stop=y_stop-(strip_y_default-strip_y_before);
			drawit=true;
		    }
		    else if(daynr==tomorrow && starttime.hour()<hours_after){
#if 0
			y_start=y_start+strip_y_before+strip_y_default;
			y_stop=y_stop+strip_y_before+strip_y_default;
#endif
			y_start=y_start+strip_y_before+strip_y_today;
			y_stop=y_stop+strip_y_before+strip_y_today;

			drawit=true;
		    }
		    else if(daynr==today){
			//today

			y_start=y_start+strip_y_before-y_subtract;
			y_stop=y_stop+strip_y_before-y_subtract;

#if 1
			if((hours_before<=0 && hours_after>=0) || \
			   (hours_before>0 && starttime.hour()>hours_before) ||
			   (hours_after<0 && starttime.hour()>(24+hours_after))){
			    drawit=true;
			}
#endif
			drawit=true;
		    }

		    if(drawit){
			//printf("start=%s (%d)  stop=%s (%d)\n",starttime.toString().latin1(),y_start,stoptime.toString().latin1(),y_stop);

			prog->y_start=y_start;
			prog->y_stop=y_stop;

			int ydiff=y_stop-y_start;
			int y_currenttime=-1;
			QDateTime currentdatetime=QDateTime::currentDateTime();
			if(prog->start<currentdatetime && currentdatetime<prog->stop){
			    color=colorNow;
			    y_currenttime=strip_y_before+(currentdatetime.time().hour()*60)+currentdatetime.time().minute()-y_subtract;
			    //printf("current prog %s yc=%d\n",prog->title.latin1(),y_currenttime);
			    //printf("strip_y_before=%d y_subtract=%d\n",strip_y_before,y_subtract);
			}

			QString startstr=starttime.toString(epg_timeformat);
			QString stopstr=stoptime.toString(epg_timeformat);
			QFont font;
			bool repaint_prog=false;
			if(prog==last_mouse_program){
			    if(my!=-2 && (y_start<=my && my<=y_stop-1)){
				repaint_prog=false;
			    }
			    else{
				repaint_prog=true;
			    }
			}
			if(my!=-2 && (y_start<=my && my<=y_stop-1) && prog!=last_mouse_program){
			    printf("%s %d < %d < %d %s-%s\n",prog->title.latin1(),y_start,my,y_stop,startstr.latin1(),stopstr.latin1());
			    QString datestr=prog->start.date().toString();
			    QString fulltext=datestr+" / "+startstr+"-"+stopstr+"  "+prog->title+prog->desc;
			    epgwin->bottom_text->setText(fulltext);
			    //color=color.dark(130);
			    color=colorMouse;
			    mouse_program=prog;
			    repaint_prog=true;
			    //found_prog=true;
			}
			if(my==-1 || repaint_prog){ //true){ //my==-1 || (y_start<my && my<y_stop)){

			    int bmode=regexp_unset;
			    if(epgwin->regexp_dirty || prog->blockmode==regexp_unset){
				//printf("\nre-evaluating blockmode for %s\n",prog->title.latin1());
				bmode=blockMode(prog);
			    }
			    else{
				bmode=prog->blockmode;
			    }
			    //printf("bmode is %d\n",bmode);
			    if(bmode==regexp_block){
				//block
				color=colorBlock;
			    }
			    else if(bmode==regexp_highlight){
				//highlight
				color=colorHigh;
			    }
			    prog->blockmode=bmode;

			    if(row==0){
				color=color.dark(110);
				row=1;
			    }
			    else{
				row=0;
			    }


			    TimerTableItem *titem=epgwin->timertable->firstTableItem();
			    int by_start=y_start;
			    int by_stop=y_stop;
			    int by_diff=by_stop-by_start;
			    QColor timercol=colorTimer;
			    //const QBrush tbrush(timercol,Qt::BDiagPattern);
			    QBrush tbrush(timercol,Qt::Dense6Pattern);
			    QBrush pbrush(color,Qt::SolidPattern);
			    QBrush xbrush;
			    while(titem!=NULL){
				if((titem->getChannel()+1)==prog->channelnr){
				    if(titem==epgwin->next_recording){
					xbrush=QBrush(colorNextRec,Qt::Dense5Pattern);
				    }
				    else if(titem==epgwin->current_recording){
					xbrush=QBrush(colorCurRec,Qt::Dense5Pattern);
				    }
				    else{
					xbrush=tbrush;
				    }
				    if(titem->getStopDateTime()>prog->start && titem->getStopDateTime()<prog->stop){
					//titem ends at the start of my block
					QDateTime tidatetime=titem->getStopDateTime();
					QDateTime mydatetime=prog->start;

					int timediff_secs=mydatetime.secsTo(tidatetime);
					int timediff_mins=timediff_secs/60;
#if 0
					QTime titime=titem->getStopDateTime().time();

					int t_end=(titime.hour()*60)+titime.minute();
					if(t_end<y_start){
					    t_end+=60*24;
					}
#endif
					int t_end=y_start+timediff_mins;

					paint.fillRect(0,y_start,strip_x,t_end-y_start,pbrush);
					paint.fillRect(0,y_start,strip_x,t_end-y_start,xbrush);
					by_start=t_end;
					//printf("%s ends at start of my block y_start=%d t_end=%d\n",prog->title.latin1(),y_start,t_end);
				    }
				    if(titem->getStartTime()>prog->start && titem->getStartTime()<prog->stop){
					//titem begins at the end of my block
					QDateTime tidatetime=titem->getStartTime();
					QDateTime mydatetime=prog->stop;

					int timediff_mins=(tidatetime.secsTo(mydatetime)/60);

					//printf("%s begins at end of my block\n",prog->title.latin1());
#if 0
					QTime titime=titem->getStartTime().time();
					int t_begin=(titime.hour()*60)+titime.minute();
#endif
					int t_begin=y_stop-timediff_mins;

					paint.fillRect(0,t_begin,strip_x,y_stop-t_begin,pbrush);
					paint.fillRect(0,t_begin,strip_x,y_stop-t_begin,xbrush);
					by_stop=t_begin;
				    }
				    if(titem->getStartTime()<prog->start && titem->getStopDateTime()>prog->stop){
					//we are completely recorded
					paint.fillRect(0,by_start,strip_x,by_diff,pbrush);
					pbrush=xbrush;
				    }

				}
				titem=(TimerTableItem *)titem->nextSibling();
			    }




			    by_diff=by_stop-by_start;
			    if(ydiff>=24){
				paint.fillRect(0,by_start,strip_x,by_diff,pbrush);
				font=QFont("Times",12);
				paint.setFont(font);
				paint.drawText(0,y_start+12,prog->title);
				paint.drawText(0,y_start+24,startstr+"-"+stopstr);
			    }
			    else if(ydiff>=10){
				paint.fillRect(0,by_start,strip_x,by_diff,pbrush);
				font=QFont("Times",10);
				paint.setFont(font);
				paint.drawText(0,y_start+10,prog->title);
				//paint.drawText(0,y_start+20,startstr+"-"+stopstr);
			    }
			    else if(ydiff>=5){
				paint.fillRect(0,by_start-2,strip_x,by_diff+2,pbrush);
				font=QFont("Times",10);
				paint.setFont(font);
				paint.drawText(0,y_start+5,prog->title);
			    }
			    else{
				//no text, too small
			    }
			    if(y_currenttime!=-1){
				QPen oldpen=paint.pen();
				QColor newcolor=color.light(110);
				paint.setPen(newcolor);
				paint.drawLine(0,y_currenttime,strip_x,y_currenttime);
				paint.setPen(oldpen);
			    }
			}
		    }//end if drawit

		}//iter programs
	    }//end we have found our channel
	}//iter channels
    }//end for day

    if(mouse_program){
	last_mouse_program=mouse_program;
    }

    paint.end();
    //  epgwin->regexp_dirty=false;
}

void EpgChannelStrip::resizeEvent(QResizeEvent *event){
    //printf("strip: resizeEvent()\n");
    //resize(strip_x,strip_y);
    //  updateGeometry();
    //  drawStrip(-1,-1);
}

void EpgChannelStrip::paintEvent(QPaintEvent *event){
    //printf("strip: paintEvent()\n");
    //resize(strip_x,strip_y);
    //setFixedSize(strip_x,strip_y);
    //adjustSize();
    //  epgwin->gridtable->adjustSize();
    drawStrip(-1,-1);
}

void EpgChannelStrip::leaveEvent(QEvent *e){
    //printf("leaveevent\n");
    drawStrip(-1,-2);
    last_mouse_program=NULL;
}

void EpgChannelStrip::mouseReleaseEvent(QMouseEvent *e){
    if(e->button()==Qt::MidButton){
	//printf("mid released while moving mouse\n");
	mid_move=false;
    }
}

void EpgChannelStrip::mouseMoveEvent(QMouseEvent *e){
    //  return;
    //  printf("mousemove\n");
    drawStrip(-1,e->y());
    if(mid_move){
	int x=e->globalX();
	int y=e->globalY();
	int dx=x-mid_move_x;
	int dy=y-mid_move_y;
	//    printf("mid pressed while moving mouse x=%d y=%d mx=%d my=%d dx=%d dy=%d\n",x,y,mid_move_x,mid_move_y,dx,dy);
	//epgwin->gridtable->scrollBy(dx,dy);
	//dx=(dx>=0)?1:-1;
	//dy=(dy>=0)?1:-1;
	epgwin->gridtable->scrollBy(-dx,-dy);
	mid_move_x=x;
	mid_move_y=y;
    }
}

void EpgChannelStrip::addrecording(){
    if(popmenu && pop_program){
	epgwin->addRecording(pop_program);
    }
}

void EpgChannelStrip::addblock(){
    if(popmenu && pop_program){
	epgwin->regexp_edit->append("title:block:^"+pop_program->title+"$");
	epgwin->okSettings();
    }
}
void EpgChannelStrip::addhighlight(){
    if(popmenu && pop_program){
	epgwin->regexp_edit->append("title:high:^"+pop_program->title+"$");
	epgwin->okSettings();
    }
}

void EpgChannelStrip::mousePressEvent(QMouseEvent *e){
    if(e->button()==Qt::MidButton){
	//printf("midbutton press\n");
	mid_move=true;
	mid_move_x=e->globalX();
	mid_move_y=e->globalY();
    }
    else if(e->button()==Qt::RightButton){

	if(popmenu){
	    delete popmenu;
	    popmenu=NULL;
	    pop_program=NULL;
	}

	if(last_mouse_program){
	    pop_program=last_mouse_program;
	    popmenu=new QPopupMenu();

	    QString startstr=last_mouse_program->start.time().toString(epg_timeformat);
	    QString stopstr=last_mouse_program->stop.time().toString(epg_timeformat);
	    QString header=startstr+"-"+stopstr+" "+last_mouse_program->title;
	    popmenu->insertItem(header);

	    QString regstr="^"+last_mouse_program->title+"$";

	    popmenu->insertItem(tr("block     title ")+regstr,this,SLOT(addblock()));
	    popmenu->insertItem(tr("highlight title ")+regstr,this,SLOT(addhighlight()));
	    popmenu->insertItem(tr("add to recordings"),this,SLOT(addrecording()));

	    popmenu->insertItem(tr("Description"));
	    //popmenu->insertItem(last_mouse_program->desc);

	    popmenu->popup(mapToGlobal(e->pos()));
	}

    }//rightbutton
}


void EpgWindow::makeConfigWidget(QWidget *parent){
    config_widget=new QVGroupBox("EPG Settings",parent);
    config_widget->setBaseSize(600,400);
    config_widget->setCaption("EPG Config");

    QHBoxLayout *lay1=new QHBoxLayout(config_widget);

    QLabel *l1=new QLabel(tr("EPG xml filename\nchanges take effect only after restart of avicap"),config_widget);
    lay1->addWidget(l1);

    conf_dir_edit=new QLineEdit(config_widget);
    lay1->addWidget(conf_dir_edit);

    conf_dir_edit->setText(RS("Epg-ProviderFilename","/tmp/epg.xml"));

    QHBox *hbox_hours=new QHBox(config_widget);

    hours_before_spinbox=new QSpinBox(-24,24,1,hbox_hours);
    hours_before_spinbox->setPrefix(tr("day starts at "));
    hours_before_spinbox->setSuffix(tr(" hours"));
    hours_before_spinbox->setValue(RI("EpgHoursBeforeDay",0));

    hours_after_spinbox=new QSpinBox(-24,24,1,hbox_hours);
    hours_after_spinbox->setPrefix(tr("day ends at "));
    hours_after_spinbox->setSuffix(tr(" hours"));
    hours_after_spinbox->setValue(RI("EpgHoursAfterDay",0));

    QHBox *hbox_mins=new QHBox(config_widget);

    mins_before_spinbox=new QSpinBox(0,60,1,hbox_mins);
    mins_before_spinbox->setPrefix(tr("recording starts "));
    mins_before_spinbox->setSuffix(tr(" mins before program"));
    int minbefore=RI("EpgMinBeforeProgram",5);
    mins_before_spinbox->setValue(minbefore);


    mins_after_spinbox=new QSpinBox(0,60,1,hbox_mins);
    mins_after_spinbox->setPrefix(tr("recording ends "));
    mins_after_spinbox->setSuffix(tr(" mins after program"));
    mins_after_spinbox->setValue(RI("EpgMinAfterProgram",5));

    QVBoxLayout *lay2=new QVBoxLayout(config_widget);

    QVGroupBox *sbox=new QVGroupBox(tr("EPG Provider to Avicap channel mappings"),config_widget);
    conf_channel_scroll=new QScrollView(sbox);
    //  lay2->addWidget(conf_channel_scroll);

    //QHBoxLayout *lay20=new QHBoxLayout(conf_channel_scroll->viewport());
    conf_channel_view=new QVBox(conf_channel_scroll->viewport());
    conf_channel_scroll->addChild(conf_channel_view);
    //lay20->addWidget(conf_channel_view);

    conf_channel_scroll->setResizePolicy(QScrollView::AutoOneFit);
    conf_channel_scroll->setVScrollBarMode(QScrollView::AlwaysOn);
    conf_channel_scroll->setHScrollBarMode(QScrollView::AlwaysOn);
    conf_channel_scroll->setBaseSize(400,300);

    //conf_channel_lay=new QGridLayout(conf_channel_view,1,1);
    //conf_channel_lay->setSpacing(5);
    //conf_channel_lay->setMargin(5);

    regexp_box=new QVGroupBox(tr("Edit Regexps"),0);
    regexp_box->setCaption(tr("EPG Edit Regexps"));
    regexp_box->setBaseSize(400,600);
    regexp_edit=new QTextEdit(regexp_box);
#if QT_VERSION>=300
    regexp_edit->setTextFormat(Qt::PlainText);
#endif
    regexp_edit->setReadOnly(false);

    QHGroupBox *regexp_abox=new QHGroupBox(tr("actions"),regexp_box);
    reg_ok_button=new QPushButton(tr("Ok/Save"),regexp_abox);
    reg_reread_button=new QPushButton(tr("Re-read from file"),regexp_abox);

    connect(reg_ok_button,SIGNAL(pressed()),this,SLOT(okSettings()));
    connect(reg_reread_button,SIGNAL(pressed()),this,SLOT(rereadRegexps()));


    QHGroupBox *bbox=new QHGroupBox(tr("actions"),config_widget);

    ok_button=new QPushButton(tr("Ok/Save"),bbox);
    connect(ok_button,SIGNAL(pressed()),this,SLOT(okSettings()));

    rereadRegexps();
    //okSettings();

}

void EpgWindow::colorSelector(){
    EpgChIdListIterator ic;
    int i=0;
    bool found_button=false;
    for(ic=epgChannels.begin();!found_button && ic!=epgChannels.end();ic++){
	EpgIdChannel *channelid=*ic;
	if(channelid->color_button->isDown()){
	    found_button=true;
	    //this button was pressed
#if QT_VERSION>=300
	    channelid->color=QColorDialog::getColor();
	    //channelid->color_button->setPaletteForegroundColor(channelid->color);
	    channelid->color_button->setPaletteBackgroundColor(channelid->color);
#else
	    channelid->color=QColorDialog::getColor(channelid->color);
#endif
	    channelid->color_button->setOn(false);
	    channelid->color_button->setDown(false);
	}
    }
}

void EpgWindow::rereadRegexps(){

    struct passwd* pwent = getpwuid(getuid());
    QString homedir = pwent->pw_dir;
    QString filename=homedir+"/.avm/avicap-regexps";

    QFile file(filename);
    bool ok=file.open(IO_ReadOnly);
    if(ok){
	QTextStream stream(&file);
	regexp_edit->setText(stream.read());
	file.close();
    }
    else{
	printf("cannot read regexp file\n");
    }

    okSettings();
}

void EpgWindow::okSettings(){
    EpgChIdListIterator ic;
    int i=0;
    WS("Epg-ProviderFilename",conf_dir_edit->text());
    for(ic=epgChannels.begin();ic!=epgChannels.end();ic++){
	EpgIdChannel *channelid=*ic;

	int channelnum=channelid->conf_channel_box->currentItem();
	channelid->chnr=channelnum;

#define WS_CH(sub,str,count) {  \
    WS(QString().sprintf("EpgChMap-%02d-%s",count,sub),str); \
    }

	//WS_CH("EpgId",channelid->epgid,i);
	//WS_CH("Provider",provider,i);
	//WS_CH("ChannelNum",QString::number(channelnum),i);
#if 0
	WI(QString().sprintf("Epg-%s-%s",provider.latin1(),channelid->epgid.latin1()),QString::number(channelnum));
	WS(QString().sprintf("Epg-%s-%s-Color",provider.latin1(),channelid->epgid.latin1()),channelid->color.name());
#endif
	WI(QString().sprintf("Epg-%s-%s","Provider",channelid->epgid.latin1()),QString::number(channelnum));
	WS(QString().sprintf("Epg-%s-%s-Color","Provider",channelid->epgid.latin1()),channelid->color.name());


	i++;
    }

    WI("EpgHoursBeforeDay",hours_before_spinbox->value());
    WI("EpgHoursAfterDay",hours_after_spinbox->value());

    WI("EpgMinBeforeProgram",mins_before_spinbox->value());
    WI("EpgMinAfterProgram",mins_after_spinbox->value());

    //write regexps to plain file
    struct passwd* pwent = getpwuid(getuid());
    QString homedir = pwent->pw_dir;
    QString filename=homedir+"/.avm/avicap-regexps";

    QFile file(filename);
    bool ok=file.open(IO_WriteOnly);
    if(ok){
	QTextStream stream(&file);
	stream << regexp_edit->text();
	file.close();
    }
    else{
	printf("cannot write regexp file\n");
    }


    QStringList strlist=QStringList::split("\n",regexp_edit->text(),false);
#if 0
    //delete old regexps
    for(uint k=0;k<regexpList.count();k++){
	EpgRegexp *epgreg=regexpList.last();
	delete epgreg;
	regexpList.pop_back();
    }
#endif

    while(regexpList.count()>0){
	QValueListIterator<EpgRegexp *> it;
	it=regexpList.begin();
	regexpList.remove(it);
    }
    //create new regexps
    int j=0;
    for (QStringList::Iterator it=strlist.begin(); it != strlist.end();it++ ) {
	QString str=*it;
	//printf("+%s+\n",str.latin1());
	//WS(QString().sprintf("Epg-Regexp-%02d",j),str);

	EpgRegexp *epgreg=new EpgRegexp();


	QString regexp0=str;

	if(regexp0[0]!='#'){
	    QString regexp;
	    int mode=regexp_errorwhere;;
	    if(regexp0.left(both_string.length())==both_string){
		mode=regexp_both;
		regexp=regexp0.right(regexp0.length()-both_string.length());
		//printf("both!\n");
	    }
	    else if(regexp0.left(title_string.length())==title_string){
		mode=regexp_title;
		regexp=regexp0.right(regexp0.length()-title_string.length());
	    }
	    else if(regexp0.left(desc_string.length())==desc_string){
		mode=regexp_desc;
		regexp=regexp0.right(regexp0.length()-desc_string.length());
	    }
	    else{
		printf("error in regexp: %s\n",regexp0.latin1());
		mode=regexp_errorwhere;
	    }
	    int bmode=regexp_block;

	    if(regexp.left(block_string.length())==block_string){
		//printf("checking block for %s\n",regexp.latin1());
		epgreg->regstr=regexp.right(regexp.length()-block_string.length());
		//printf("regexp is +%s+\n",reg.latin1());

		bmode=regexp_block;
	    }
	    else if(regexp.left(high_string.length())==high_string){
		//printf("checking high for %s\n",regexp.latin1());
		epgreg->regstr=regexp.right(regexp.length()-high_string.length());
		//printf("regexp is +%s+\n",reg.latin1());

		bmode=regexp_highlight;
	    }
	    else{
		printf("error in regexp: %s\n",regexp.latin1());
		bmode=regexp_errorblock;
	    }
	    epgreg->regexp=QRegExp(epgreg->regstr,false,false);
	    epgreg->wheremode=mode;
	    epgreg->blockmode=bmode;

	    regexpList.append(epgreg);
	}
	j++;
    }

    //    WS(QString().sprintf("Epg-Regexp-%02d",j),"NONE");

    avm::RegSave();

    regexp_dirty=true;

    day_selected(current_date_item);

    //  WS_CH("EpgId","NONE",i);
}


#include "epgwindow.moc"

#else
#warning "no epgwindow"
#endif
