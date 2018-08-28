/*
  TimerTable window for AviCap

  written 2003 by Alexander Rawass (alexannika@users.sourceforge.net)
*/

#include "timertable.h"
#include "timertable.moc"

#include "vidconf.h"
#include "v4lwindow.h"
#include "capproc.h"
#include "codecdialog.h"
#include "avicapwnd.h"

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
#include <qtabwidget.h>

#if QT_VERSION>=300
#include <qdatetimeedit.h>
#endif
#include <epgwindow.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h> // atof
#include <sys/vfs.h> //statfs



void TimerTableItem::checkWhen(){
    QDateTime currentdatetime=QDateTime::currentDateTime();
    QDate currentdate=QDate::currentDate();
    QTime currenttime=currentdatetime.time();
    QTime start_timeonly=start_time.time();
    QTime stop_timeonly=stop_datetime.time();


    if(when==0){
	//once, do nothing
    }
    else if(when==1){
	//tomorrow
	start_time=currentdatetime.addDays(1);
	when=0; //set back to once
    }
    else if(when==2){
	//day after tomorrow
	start_time=currentdatetime.addDays(2);
	when=0;
    }
    else if(when==3){
	//every
	if(day==0){
	    //every day

	    if(start_timeonly<currenttime && stop_timeonly>currenttime){
		// it is NOW
		start_time.setDate(currentdate);
	    }
	    else if(start_timeonly<currenttime){
		// it is in the past
		// so set it to tomorrow
		start_time.setDate(currentdate.addDays(1));
	    }
	    else{
		// the start time is in the future
		start_time.setDate(currentdate);
	    }
	}//end of day==0
	else{
	    // day 1..7 = Mon .. Sun
	    int currentday=currentdate.dayOfWeek();
	    if(currentday==day){
		// the same weekday as today
		if(start_timeonly<currenttime && stop_timeonly>currenttime){
		    // it is NOW
		    start_time.setDate(currentdate);
		}
		else if(start_timeonly<currenttime){
		    // it is in the past
		    // so set it to next day with the wanted weekday
		    start_time.setDate(currentdate);
		    do{
			start_time=start_time.addDays(1);
		    }while(start_time.date().dayOfWeek()!=day);
		}
		else{
		    // the start time is in the future
		    start_time.setDate(currentdate);
		}
	    }//end of 'currentday == day'
	    else{
		//not the same weekday as today
		start_time.setDate(currentdate);
		do{
		    start_time=start_time.addDays(1);
		}while(start_time.date().dayOfWeek()!=day);
	    }//end of 'currentday!=day'

	}//end of 'day == 1...7'
    }//end of 'when==3'

    else if(when==4){
	//next
    }
    else{
	printf("ERROR: unknown when\n");
    }

    setTable(channel_nr,start_time,stop_time,filename,description,when,day,named_codec);
}

void TimerTableItem::paintCell ( QPainter * p, const QColorGroup & cg, int column, int width, int align )
{
    //printf("paintcell!\n");
    QColorGroup new_cg=QColorGroup(cg);

    //  new_cg.setColor(QColorGroup::Base,QColor(0xf3,0xfc,0xc7));

    QDateTime currenttime=QDateTime::currentDateTime();

    QColor textcol;

    if(stop_datetime<currenttime){
	textcol=QColor(0,0,200);
	new_cg.setColor(QColorGroup::Highlight,QColor(0,0,150));
    }
    if(start_time>currenttime){
	textcol=QColor(0,0,0);
    }
    if(this==timer_window->getNextRecording()){
	textcol=QColor(0,0,0);
	new_cg.setColor(QColorGroup::Highlight,QColor(0,150,0));
	new_cg.setColor(QColorGroup::Base,QColor(180,255,180));
    }
    if(this==timer_window->getCurrentRecording()){
	textcol=QColor(0,0,0);
	new_cg.setColor(QColorGroup::Highlight,QColor(150,0,0));
	new_cg.setColor(QColorGroup::Base,QColor(255,160,160));
    }

    new_cg.setColor(QColorGroup::Text,textcol);

    //new_cg.setColor(QColorGroup::Foreground,QColor(255,0,0));
    //new_cg.setColor(QColorGroup::Background,QColor(0,255,0));
    //new_cg.setColor(QColorGroup::Button,QColor(255,0,255));
    //new_cg.setColor(QColorGroup::ButtonText,QColor(255,0,255));
    //new_cg.setColor(QColorGroup::Highlight,QColor(100,0,255));
    //new_cg.setColor(QColorGroup::HighlightedText,QColor(255,100,255));
    QCheckListItem::paintCell(p,new_cg,column,width,align);
}

int TimerTableItem::compare(QListViewItem *other,int col, bool ascend) const {
    if(other==NULL){
	return 0;
    }

    TimerTableItem *other_tti=(TimerTableItem *)other;
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
#warning "fixme: QListViewItem::compare"
	printf("fixme: QListViewItem::compare for QT23\n");
	result=0;
#endif

    }

    return result;
}

/* **************************************** */

// sets an entry in the table to new values

void TimerTableItem::setTable(int ch,QDateTime start,QTime stop,QString fname,QString desc,int whentime,int whatday,QString codec) {
    channel_nr=ch;
    start_time=start;
    stop_time=stop;
    filename=fname;
    description=desc;
    when=whentime;
    day=whatday;
    named_codec=codec;

    stop_datetime=timer_window->calc_stopDateTime(start_time,stop_time);

#if QT_VERSION>=300
    QString start_timestr=start_time.toString(my_dateformat);
#else
    QString weekday=start_time.date().dayName(start_time.date().dayOfWeek());
    QString start_timestr=dateTimeToString(start_time)+QString(" ")+weekday;
#endif

    QString stop_timestr=stop_time.toString(my_timeformat);
    QString numstr;

    //setText(0,QString::number(channel_nr));
    setText(0,description);
    //setText(1,QString::number(channel_nr+1)+" - "+QString(my_v4lwin->getXawtvStation(channel_nr).c_str()));
    setText(1,numstr.sprintf("%02d",channel_nr+1)+" - "+QString(my_v4lwin->getXawtvStation(channel_nr).c_str()));
    setText(2,start_timestr);
    setText(3,stop_timestr);
    setText(4,named_codec);
    setText(5,filename);
}

/* **************************************** */

TimerTableItem::TimerTableItem(QListView *parent,TimerTable *timer_win,V4LWindow *v4lw,int ch,QDateTime start,QTime stop,QString fname,QString desc,int whentime,int whatday,QString codec) : QCheckListItem(parent,"test",QCheckListItem::CheckBox){
    timer_window=timer_win;
    my_v4lwin=v4lw;

    setTable(ch,start,stop,fname,desc,whentime,whatday,codec);
}

/* **************************************** */

TimerTableItem::~TimerTableItem(){
}

void TimerTable::setEpgWindow(EpgWindow *win){
    epgwin=win;

    connect((const QObject *)win,SIGNAL(emitAddRecording(EpgProgram *)),this,SLOT(addEpgRecording(EpgProgram *)));
}

void TimerTable::addEpgRecording(EpgProgram *prog){
    printf("signal got\n");
    QDateTime start=prog->start;
    QDateTime stop=prog->stop;

    start=start.addSecs(-RI("EpgMinBeforeProgram",5)*60);
    stop=stop.addSecs(RI("EpgMinBeforeProgram",5)*60);

    TimerTableItem *item=new TimerTableItem(table,this,my_v4lw,prog->channelnr-1,start,stop.time(),"epgprogram.avi",prog->title,0,0,RS("NamedCodecsCurrent","-Default-"));

    avml(AVML_DEBUG, "added recording from EPG: %s from %s to %s\n",
	 prog->title.latin1(),start.toString().latin1(),
	 stop.toString().latin1());
}

/* **************************************** */

// creates the timertable

TimerTable::TimerTable( QWidget* parent, v4lxif* v4l, V4LWindow* w)
: QDialog( NULL, "Avicap Timertable", false)
{
    my_v4lxif=v4l;
    my_v4lw=w;

    avml(AVML_DEBUG, tr("TimerTable started\n"));

    setMinimumSize(600,700);
    //setMaximumSize(800,800);
    setCaption( tr("AviCap Timertable") );

    timer_mode=TimerOff;
    next_recording=NULL;
    current_recording=NULL;
    last_item_clicked=NULL;
    shutdown_message=NULL;
    epgwin=NULL;


    splitter1=new QSplitter(QSplitter::Vertical,this);
    splitter1->setOpaqueResize(true);

    table=new QListView(splitter1);
    table->setBaseSize(600,200);
    table->setMinimumSize(600,200);
    //  table->setMaximumSize(800,800);
    table->setSelectionMode(QListView::Single);
    connect(table,SIGNAL(pressed(QListViewItem *)),this,SLOT(item_clicked(QListViewItem *)));

    //table->addColumn("ChNr");
    table->addColumn(tr("Description"));
    table->addColumn(tr("Channel Name"));
    table->addColumn(tr("Start Recording"));
    table->addColumn(tr("End Rec"));
    table->addColumn(tr("Codec"));
    //table->setColumnAlignment(3,Qt::AlignRight);
    table->setColumnAlignment(3,Qt::AlignHCenter);
    table->addColumn(tr("Filename"));
    //table->setItemMargin(5);
    table->setAllColumnsShowFocus(true);
    table->setShowSortIndicator(true);
    table->setSorting(2,true);
    table->sort();

    all_box=new QGroupBox(1,Qt::Horizontal,splitter1);
    all_box->setBaseSize(600,600);
    all_box->setMinimumSize(600,600);
    //  all_box->setMaximumSize(600,800);

    splitter1->setResizeMode(all_box,QSplitter::Stretch);
    splitter1->setResizeMode(table,QSplitter::Stretch);

    //edit_box=new QGroupBox(2,Qt::Horizontal,all_box);
    edit_box=new QWidget(all_box);
    //edit_box=new QGroupBox(splitter1);
    edit_box->setBaseSize(600,200);


    QGridLayout* gl = new QGridLayout( edit_box, 1, 1 );
    gl->setSpacing( 5 );
    gl->setMargin( 5 );

    int row=0;

    when_combobox=new QComboBox(edit_box);
    gl->addWidget(when_combobox,row,0);
    when_combobox->insertItem(tr("once"));
    when_combobox->insertItem(tr("tomorrow"));
    when_combobox->insertItem(tr("day after tomorrow"));
    when_combobox->insertItem(tr("every"));
    when_combobox->insertItem(tr("next"));

    day_combobox=new QComboBox(edit_box);
    gl->addWidget(day_combobox,row,1);

    day_combobox->insertItem(tr("day"));
    for(int i=1; i<=7;i++){
#if QT_VERSION>=300
	day_combobox->insertItem(QDate::longDayName(i));
#else
	QDate bogus;
	day_combobox->insertItem(bogus.dayName(i));
#endif
    }

    row++;
    QLabel *l=new QLabel(tr("Start Date/Time:"),edit_box);
    gl->addWidget(l,row,0);

    begin_timeedit=new QDateTimeEdit(edit_box,"start_time");
    gl->addWidget(begin_timeedit,row,1);

    row++;
    QLabel *l2=new QLabel(tr("End Time:"),edit_box);
    gl->addWidget(l2,row,0);
    end_timeedit=new QTimeEdit(edit_box,"end_time");
    gl->addWidget(end_timeedit,row,1);

    row++;
    QLabel *l3=new QLabel(tr("Channel:"),edit_box);
    gl->addWidget(l3,row,0);
    channelbox=new QComboBox(edit_box,"channel");
    gl->addWidget(channelbox,row,1);

    if (my_v4lw->getStations())
    {
	char cbuf[100];

	for (int i = 0; i < my_v4lw->getStations(); i++){
	    sprintf(cbuf,"%-2d %s",i+1,my_v4lw->getXawtvStation(i).c_str());
	    channelbox->insertItem(cbuf);
	}
    }

    row++;
    QLabel *l444=new QLabel(tr("Codec:"),edit_box);
    gl->addWidget(l444,row,0);
    codecbox=new QComboBox(edit_box,"codec");
    gl->addWidget(codecbox,row,1);


    QString named_codecs_current=RS("NamedCodecsCurrent","-Default-");
    recreateCodecBox(named_codecs_current);





    row++;
    QLabel *l4=new QLabel(tr("Filename:"),edit_box);
    gl->addWidget(l4,row,0);
    filename_edit=new QLineEdit(edit_box,"filename");
    gl->addWidget(filename_edit,row,1);

    row++;
    QLabel *l5=new QLabel(tr("Description"),edit_box);
    gl->addWidget(l5,row,0);
    description_edit=new QLineEdit(edit_box,"description");
    gl->addWidget(description_edit,row,1);


    edit_box->hide();

    edit_buttons=new QButtonGroup(3,Qt::Horizontal,all_box);

    new_button=new QPushButton(tr("new"),edit_buttons);
    modify_button=new QPushButton(tr("modify"),edit_buttons);
    remove_button=new QPushButton(tr("remove"),edit_buttons);

    rec_buttons=new QButtonGroup(2,Qt::Horizontal,all_box);

    rec_start_button=new QPushButton(tr("start timer"),rec_buttons);

    shutdown_checkbox=new QCheckBox(tr("shutdown when allowed"),rec_buttons);

    connect(new_button,SIGNAL(pressed()),this,SLOT(button_new_pressed()));
    connect(modify_button,SIGNAL(pressed()),this,SLOT(button_modify_pressed()));
    connect(remove_button,SIGNAL(pressed()),this,SLOT(button_remove_pressed()));

    connect(rec_start_button,SIGNAL(pressed()),this,SLOT(button_start_pressed()));

    QWidget *wi=new QWidget(all_box);
    QVBoxLayout *lo=new QVBoxLayout(wi);
    status_label=new QLabel(wi);
    lo->addWidget(status_label);

    init_entry_widgets();

    load();
    if(table->childCount()>0){
	edit_box->show();

	TimerTableItem *first_item=(TimerTableItem *)table->firstChild();
	table->setCurrentItem(first_item);
	table->setSelected(first_item,true);
	last_item_clicked=first_item;

	item_clicked(first_item);
    }

    set_status_label();

    next_recording=find_first_recording();
    if(next_recording){
	table->ensureItemVisible(next_recording);
	table->setCurrentItem(next_recording);
	table->setSelected(next_recording,true);
	last_item_clicked=next_recording;

	item_clicked(next_recording);
    }
    else{
	printf("no next recording at startup\n");
    }

    my_timer=new QTimer(this);
    connect(my_timer,SIGNAL(timeout()),SLOT(timer()));
    my_timer->start(1000);
}

/* **************************************** */

TimerTable::~TimerTable(){
    printf("TimerTable has been exited\n");
    my_timer->stop();
    save();
}

/* **************************************** */

QString TimerTable::add_current_time_info(){
    QString result;

    result=QString().sprintf(tr("Current Time : %s\n"),QDateTime::currentDateTime().toString(my_dateformat).latin1());

    return result;
}

/* **************************************** */

QString TimerTable::add_next_recording_info(){
    QString result;
    QDateTime currenttime=QDateTime::currentDateTime();

    if(next_recording){
	int chnr=next_recording->getChannel();
	avm::string chname=my_v4lw->getXawtvStation(chnr);
	QString fname=next_recording->getFilename();
	QString desc=next_recording->getDescription();
	QDateTime starttime=next_recording->getStartTime();

	int diff_in_secs=currenttime.secsTo(starttime);
	//printf("%d secs to wait\n",diff_in_secs);

	QString etastring;
	if(diff_in_secs>60*60*24){
	    // more than a day
	    int days=diff_in_secs/(60*60*24);
	    etastring.sprintf("more than %d days",days);
	}
	else{
	    QTime notime=QTime();
	    QTime eta=notime.addSecs(diff_in_secs);

	    etastring=eta.toString(my_timeformat);
	}
	result=QString().sprintf(tr("Next Recording: %s  #%d  %s  to %s\n")+
				 tr("\tstarts at %s in %s\n"),
				 desc.latin1(),chnr+1,chname.c_str(),fname.latin1(),
				 starttime.toString(my_dateformat).latin1(),etastring.latin1());

    }
    else{
	result=QString().sprintf(tr("No Next Recording\n"));
    }

    return result;
}

/* **************************************** */

QString TimerTable::add_current_recording_info(){
    QString result;
    QDateTime currenttime=QDateTime::currentDateTime();

    if(current_recording){
	QDateTime stoptime=current_recording->getStopDateTime();

	int diff_in_secs=currenttime.secsTo(stoptime);
	//printf("%d secs to go\n",diff_in_secs);

	QTime notime=QTime();

	QTime eta=notime.addSecs(diff_in_secs);

	int chnr=current_recording->getChannel();

	avm::string chname=my_v4lw->getXawtvStation(chnr);

	QString fname=current_recording->getFilename();
	QString desc=current_recording->getDescription();

	//      sprintf(sbuf,tr("Currently Recording: stops at %s in %s\nChannel Nr %d  %s  %s\nFilename %s\n"),stoptime.toString(my_dateformat).latin1(),eta.toString(my_timeformat).latin1(),chnr+1,chname.c_str(),desc.latin1(),fname.latin1());
	result=QString().sprintf(tr("Current Recording: %s  #%d  %s  to %s\n\tstops at %s in %s\n"),
				 desc.latin1(),chnr+1,chname.c_str(),fname.latin1(),
				 stoptime.toString(my_dateformat).latin1(),eta.toString(my_timeformat).latin1());

    }
    else{
	result=QString().sprintf(tr("No Current Recording\n"));
    }

    return result;
}

/* **************************************** */

QString TimerTable::add_sanity_check_info(){
    QString result;

    TimerTableItem *item1=(TimerTableItem *)table->firstChild();

    bool insane=false;
    while(!insane && item1!=NULL){
	if(item1->isOn()){
	    TimerTableItem *item2=(TimerTableItem *)table->firstChild();

	    while(!insane && item2!=NULL){
		if(item2!=item1 && item2->isOn()){
		    QDateTime start1=item1->getStartTime();
		    QDateTime stop1=item1->getStopDateTime();

		    QDateTime start2=item2->getStartTime();
		    QDateTime stop2=item2->getStopDateTime();

		    if((start1<start2 && start2<stop1) || (start2<start1 && start1<stop2)){
			// do I have to do more sanity checks???
			avm::string chn1=my_v4lw->getXawtvStation(item1->getChannel());
			avm::string chn2=my_v4lw->getXawtvStation(item2->getChannel());

			result=QString().sprintf(tr("Sanity Check failed:\n")+ \
						 "a) %-10s\t#%d  %-10s " +tr("from")+" %s to %s\n" + \
						 "b) %-10s\t#%d  %-10s "+tr("from")+" %s to %s\n",
						 item1->getDescription().latin1(),item1->getChannel()+1,chn1.c_str(), \
						 item1->getStartTime().toString(my_dateformat).latin1(), \
						 item1->getStopDateTime().toString(my_dateformat).latin1(),
						 item2->getDescription().latin1(),item2->getChannel()+1,chn2.c_str(), \
						 item2->getStartTime().toString(my_dateformat).latin1(), \
						 item2->getStopDateTime().toString(my_dateformat).latin1());
			//strcat(bigbuf,sbuf);
			insane=true;
		    }
		}
		item2=(TimerTableItem *)item2->nextSibling();
	    }
	}

	item1=(TimerTableItem *)item1->nextSibling();
    }

    return result;
}

/* **************************************** */

void TimerTable::set_status_label(){

    QDateTime currenttime=QDateTime::currentDateTime();

    if(timer_mode==TimerOff){
	QString curtim=QString().sprintf(tr("Current Time: %s\nTimer not activated\n"),currenttime.toString(my_dateformat).latin1());
	//add_next_recording_info(bigbuf);
	//add_sanity_check_info(bigbuf);

	status_label->setText(curtim);
    }
    else if(timer_mode==TimerWaiting){
	QString label;
	label=add_current_time_info();
	label+=add_next_recording_info();
	label+=add_sanity_check_info();

	status_label->setText(label);
    }
    else if(timer_mode==TimerRecording){
	QString label;
	label=add_current_time_info();
	label+=add_current_recording_info();
	label+=add_next_recording_info();
	label+=add_sanity_check_info();

	status_label->setText(label);
    }
}

/* **************************************** */

// initializes the entry widgets with default values

void TimerTable::init_entry_widgets(){
    QDateTime current=QDateTime::currentDateTime();

    begin_timeedit->setDateTime(current);

    end_timeedit->setTime(QTime::currentTime());

    filename_edit->setText("./movie.avi");

    channelbox->setCurrentItem(0);

    codecbox->setCurrentItem(0);

    description_edit->setText("untitled");

    when_combobox->setCurrentItem(0);
    day_combobox->setCurrentItem(0);
}

/* **************************************** */

void TimerTable::item_clicked(QListViewItem *lvitem){
    TimerTableItem *item=(TimerTableItem *)lvitem;

    if(item!=NULL){
	if(item==next_recording){
	    printf("next recording clicked\n");
	    description_edit->setBackgroundColor(QColor(150,0,0));
	    //description_edit->setBackgroundMode(Qt::PaletteDark);
	    //description_edit->setPaletteBackgroundColor(QColor(150,0,0));
	    //description_edit->setPaletteForegroundColor(QColor(150,0,0));
	    //description_edit->setEraseColor(QColor(150,0,0));
	    //QPalette pal(QColor(150,0,0),QColor(0,150,0));
	    //QPalette pal;
	    //description_edit->setPalette(pal);
	}

	begin_timeedit->setDateTime(item->getStartTime());

	end_timeedit->setTime(item->getStopTime());

	filename_edit->setText(item->getFilename());

	channelbox->setCurrentItem(item->getChannel());

	description_edit->setText(item->getDescription());

	when_combobox->setCurrentItem(item->getWhen());
	day_combobox->setCurrentItem(item->getDay());

	recreateCodecBox(item->getNamedCodec());

	last_item_clicked=item;
    }
}

void TimerTable::recreateCodecBox(QString set_name){
    while(codecbox->count()>0){
	codecbox->removeItem(0);
    }

  codecbox->insertItem("-Default-");
  codecbox->setEditable(false);
  codecbox->setCurrentItem(0);

    {
	int i=0;

	QString regname=QString().sprintf("NamedCodecs-%02d-Savename",i);
	QString nc_savename=RS(regname,"NONE");

	while(nc_savename!="NONE"){
	    regname=QString().sprintf("NamedCodecs-%02d-Codec",i);
	    QString nc_codec=RS(regname,"NONE");

	 //codecbox->insertItem(nc_savename);

	    i++;

	    regname=QString().sprintf("NamedCodecs-%02d-Savename",i);
	    nc_savename=RS(regname,"NONE");
	}


	for(int j=0;j<codecbox->count();j++){
	    if(codecbox->text(j)==set_name){
		codecbox->setCurrentItem(j);
	    }
	}
    }

}


/* **************************************** */

void TimerTable::button_new_pressed(){
    edit_box->show();
    init_entry_widgets();
    TimerTableItem *newitem=set_tableitem(NULL);

    table->setCurrentItem(newitem);
    table->setSelected(newitem,true);
    last_item_clicked=newitem;
}

/* **************************************** */

// helper routine to either
// create a new TimerTableItem or
// modify an existing one

TimerTableItem * TimerTable::set_tableitem(TimerTableItem *item){
    QDateTime start_datetime=begin_timeedit->dateTime();
    QTime stop_time=end_timeedit->time();

    int channel_nr=channelbox->currentItem();

    QString filename=filename_edit->text();
    QString desc=description_edit->text();

    QString stationname=my_v4lw->getXawtvStation(channel_nr).c_str();


    int when=when_combobox->currentItem();
    int day=day_combobox->currentItem();

    QString codec=codecbox->currentText();

    QDateTime currentdatetime=QDateTime::currentDateTime();
    QDate currentdate=QDate::currentDate();
    QTime currenttime=currentdatetime.time();
    QTime start_time=start_datetime.time();

    //QString start_timestr=start_datetime.toString(my_dateformat);
    //QString stop_timestr=stop_time.toString(my_timeformat);


    if(item!=NULL){
	item->setTable(channel_nr,start_datetime,stop_time,filename,desc,when,day,codec);
	avml(AVML_DEBUG, "modified item %s from %s to %s\n",
	     desc.latin1(),start_datetime.toString().latin1(),
	     stop_time.toString().latin1());
    }
    else{
	item=new TimerTableItem(table,this,my_v4lw,channel_nr,start_datetime,stop_time,filename,desc,when,day,codec);
	avml(AVML_DEBUG, "new item %s from %s to %s\n",
	     desc.latin1(),start_datetime.toString().latin1(),
	     stop_time.toString().latin1());
    }
    table->sort();
    table->ensureItemVisible(item);

    return item;
}

/* **************************************** */

void TimerTable::button_modify_pressed(){
    //    TimerTableItem *item=(TimerTableItem *)table->currentItem();

    TimerTableItem *item=last_item_clicked;

    if(item!=NULL){
	avml(AVML_DEBUG, "item modified\n");
	set_tableitem(item);
    }

    save();

    emit timertableChanged();
}

/* **************************************** */

void TimerTable::button_remove_pressed(){
    TimerTableItem *item=(TimerTableItem *)table->currentItem();

    if(item==current_recording){
	avml(AVML_DEBUG, "can't remove - it's the current recording\n");
    }
    else{
	if(item==next_recording){
	    next_recording=NULL;
	}
	if(item==last_item_clicked){
	    last_item_clicked=NULL;
	}
	table->takeItem(item);
	avml(AVML_DEBUG, "removed item %s\n",item->getDescription().latin1());
	delete item;

	save();

	emit timertableChanged();
    }
}

/* **************************************** */

// gives back the first recording that should be done,
// starting from this time
// old recordings and disabled recordings are ignored

TimerTableItem  *TimerTable::find_first_recording(){
    TimerTableItem *item=(TimerTableItem *)table->firstChild();

    QDateTime currenttime=QDateTime::currentDateTime();
    QDateTime found_date=currenttime;

    TimerTableItem *found_item=NULL;

    while(item!=NULL){
	if(item->isOn()){
	    QDateTime new_date=item->getStartTime();

	    if(new_date>=currenttime){
		// this recordings starts in the future
		//if(new_date<found_date || found_date==currenttime){
		if(new_date<found_date || found_item==NULL){
		    // if the new date is less in the future than the
		    // previuosly found date
		    // or if we didn;t find any yet
		    found_date=new_date;
		    found_item=item;
		    //printf("we found in the future: %s %s\n",found_date.toString().latin1(),found_item->getDescription().latin1());
		}
	    }
	    else{
		// this recording starts in the past
		// but it might still end in the future

		QDateTime stop_date=item->getStopDateTime();
		if(stop_date>currenttime && item!=current_recording){
		    //&& current_recording==NULL){

		    // the stop date is in the future and
		    // it's not the current recording
		    found_date=new_date;
		    found_item=item;
		    //printf("we found at present: %s %s\n",found_date.toString().latin1(),found_item->getDescription().latin1());
		}
		else{
		    // no, it also ended in the past

		    //make sure that items in the past can never be enabled
		    item->setOn(false);
		}
	    }//end else newdate>=currenttime
	}//end if item(ison)

	if(item!=current_recording && item->getStopDateTime()<currenttime){
	    item->checkWhen();
	    table->sort();
	}

	item=(TimerTableItem *)item->nextSibling();
    }



    return found_item;
}

/* **************************************** */

// helper routine
// gets called when user presses 'start timer'
// or by automatic 'avicap -timer'

void TimerTable::startTimer(bool set_shutdown){
    if(set_shutdown){
	//we have been started automatically
	avml(AVML_DEBUG, "timertable started with -timer\n");
#if 0
	int fd=open(WATCHDOG_TIMERTABLE,O_WRONLY|O_CREAT);
	if(fd==-1){
	    avml(AVM_WARN, "error while creating watchdog file\n");
	}
	else{
	    char wbuf[100];
	    strcpy(wbuf,"ok\n");
	    write(fd,wbuf,strlen(wbuf));
	}
#endif
	shutdown_checkbox->setChecked(true);
    }

    if(table->childCount()>0){

	TimerTableItem *first_item=find_first_recording();

	if(first_item!=NULL){
	    next_recording=first_item;

	    timer_mode=TimerWaiting;
	    rec_start_button->setText("Stop Timer");
	}
    }
    else{
	printf("no programs, timer not started\n");
    }

}

/* **************************************** */

void TimerTable::button_start_pressed(){
    save();

    if(timer_mode==TimerOff){
	avml(AVML_DEBUG, "started timer\n");
	startTimer(false);
    }
    else if(timer_mode==TimerRecording){
	avml(AVML_WARN, "can't switch off timer while recording with timer\n");
    }
    else if(timer_mode==TimerWaiting){
	timer_mode=TimerOff;
	rec_start_button->setText(tr("Start Timer"));
	next_recording=NULL;
	avml(AVML_DEBUG, "stopped timer\n");
    }
    set_status_label();
}

int find_keepfree(QString dirname){
    int i=0;
    QString regname;
    QString fullname;

    regname=QString().sprintf("DirPool-%02d-Path",i);
    QString path=RS(regname,"NONE");

    while(path!="NONE"){
	regname=QString().sprintf("DirPool-%02d-Active",i);
	int act=RI(regname,1);
	if(act){
	    regname=QString().sprintf("DirPool-%02d-KeepFree",i);
	    int keepfree=RI(regname,500);

	    if(path==dirname){
		return keepfree;
	    }
	}

	i++;

	regname=QString().sprintf("DirPool-%02d-Path",i);
	path=RS(regname,"NONE");

    }
    return 0;
}

QString find_best_dir(){

    int i=0;
    QString regname;
    QString fullname;

    regname=QString().sprintf("DirPool-%02d-Path",i);
    QString path=RS(regname,"NONE");
    long found_free_space=0;
    QString found_path;

    while(path!="NONE"){
	regname=QString().sprintf("DirPool-%02d-Active",i);
	int act=RI(regname,1);

	regname=QString().sprintf("DirPool-%02d-KeepFree",i);
	int keepfree=RI(regname,500);

	//printf("path=%s active=%d keepfree=%d\n",path.latin1(),act,keepfree);
	if(act){
	    //evaluate

	    int free_space=free_diskspace(path.latin1());

	    free_space-=keepfree;
	    //printf("free space for avicap %dMB on %s\n",free_space,path.latin1());
	    if(free_space>found_free_space){
		//printf("found free space for avicap %dMB on %s\n",free_space,path.latin1());
		found_free_space=free_space;
		found_path=path;
	    }
	}// end of 'if act'

	i++;

	regname=QString().sprintf("DirPool-%02d-Path",i);
	path=RS(regname,"NONE");
    }

    if(found_free_space<=0){
	return "";
    }

    return found_path;
}

/* **************************************** */

// gets called every second
// check if the state has changed and what we've got to do next

void TimerTable::timer(){
    static long timer_counter=0;

    if (timer_counter%60==0)
	avml(AVML_DEBUG, "timer is alive\n");

    timer_counter++;

    next_recording=find_first_recording();

    if(next_recording){
	//    printf("next_recording: %s\n",next_recording->getDescription().latin1());
    }

    if(next_recording && timer_mode==TimerWaiting){
	// we have a pending recording and the timer is waiting for
	// pending recordings

	// now check if it's time to start this recording
	QDateTime  start_time=next_recording->getStartTime();
	QDateTime currenttime=QDateTime::currentDateTime();
	int secs_to_go=currenttime.secsTo(start_time);

	int secs_in_poweroff=secs_to_go  \
	    - RI("ShutdownRebootTimespan",5)*60  \
	    - RI("ShutdownGraceTime",3)*60;

	//    if(secs_to_go<10){
	if(start_time<currenttime){
	    avml(AVML_DEBUG, "starting new recording\n");
	    // we have to start a new pending recording
	    // we start recording actually 10 secs before the time

	    // stop old recording if necessary
	    stop_current_recording();

	    my_v4lw->setXawtv(next_recording->getChannel());

	    QString savename=next_recording->getNamedCodec();
	    if(savename=="-Default-"){
		savename="";
	    }
	    //what now?

	    CaptureConfig *conf=new CaptureConfig();
	    conf->load();
	    conf->setNamedCodec(savename.latin1());
	    QString filename=next_recording->getFilename();
	    conf->setFilename(filename.latin1());

	    my_v4lw->captureAVI(conf); //make the new capture window

	    AviCapDialog *dialog=my_v4lw->getCaptureDialog();

	    if(dialog!=NULL){
		dialog->start(); // run the thing
		timer_mode=TimerRecording;
		current_recording=next_recording;
	    }
	    else{
		avml(AVML_WARN, "FATAL: recording could not be started\n");
	    }
	}// 10 secs to go
	else if(secs_in_poweroff > RI("ShutdownMinTimespan",20)*60){
	    // if the computer would be in off-state for the allowed time
	    // we check if we should do a shutdown
	    check_for_shutdown();
	}
    }
    else if(timer_mode==TimerRecording && current_recording){
	//we are currently recording
	//check if we have to switch off current recording

	// check if there is still a recording actually running
	// (user might have pressed stop or close)
	AviCapDialog *dialog=my_v4lw->getCaptureDialog();
	CaptureProcess *capproc=NULL;
	if(dialog){
	    capproc=dialog->getCaptureProcess();
	}

	if(dialog==NULL || capproc==NULL){
	    // no recording, so nothing to switch off

	    // we disable the timertable item, so that the recording
	    // won't start again, or stop unwanted recordings
	    current_recording->setOn(false);
	}
	QDateTime  stop_time=current_recording->getStopDateTime();
	QDateTime currenttime=QDateTime::currentDateTime();

	//int secs_to_go=currenttime.secsTo(stop_time);
	//printf("%d secs to go to stop recording\n",secs_to_go);

	if(dialog==NULL || capproc==NULL || currenttime  > stop_time || \
	   (next_recording && next_recording!=current_recording && next_recording->getStartTime()<currenttime)){
	    // if our stop time is in the past OR
	    // if a next recording has been found with a stop time in the past

	    // this means, a programmed recording that clashes with a second
	    // programmed recording will get stopped and the second one
	    // started
	    // it's your own fault if there are 'insane' recordings
	    // have a look at the sanity check more often ;-)

	    //stop current recording, if there is
	    stop_current_recording();
	    // we go back to wait for pending recordings
	    current_recording=NULL;
	    timer_mode=TimerWaiting;
	    avml(AVML_DEBUG, "stopped current recording\n");
	}
    }
    else if(timer_mode==TimerWaiting && next_recording==NULL && current_recording==NULL){
	// no more recordings, can we shutdown?

	check_for_shutdown();
    }

    set_status_label();

    table->repaint();
    QListViewItem *item=table->firstChild();
    while(item!=NULL){
	table->repaintItem(item);
	item=item->nextSibling();
    }

    if(last_current_recording!=current_recording){
	emit currentRecordingChanged(current_recording);
	avml(AVML_DEBUG, "current recording changed\n");
    }
    if(last_next_recording!=next_recording){
	emit nextRecordingChanged(next_recording);
	avml(AVML_DEBUG, "next recording changed\n");
    }
    last_current_recording=current_recording;
    last_next_recording=next_recording;
}

/* **************************************** */

void TimerTable::check_for_shutdown(){

    int shutdown_mode=RI("ShutdownMode",0);
    if(shutdown_mode==0){
	// never shut down
	return;
    }

    bool shutdown=shutdown_checkbox->isChecked();

    if(shutdown){
	if(!shutdown_message){
	    // open up a new message
	    avml(AVML_DEBUG, "shutdown procedure started\n");
	    shutdown_message=new QMessageBox( tr( "AviCap Shutdown warning" ),
					     "System is going to shutdown\n\nin some seconds\n",
					     QMessageBox::Critical,QMessageBox::Abort,
					     QMessageBox::NoButton,QMessageBox::NoButton,this);
	    shutdown_message->show();
	    shutdown_time=QDateTime::currentDateTime();
	    shutdown_time=shutdown_time.addSecs(RI("ShutdownGraceTime",3)*60);
	}
	else{
	    // there is already a shutdown message open

	    QDateTime currenttime=QDateTime::currentDateTime();
	    int secsto=currenttime.secsTo(shutdown_time);
	    avml(AVML_DEBUG, "secs to shutdown: %d res=%d\n",
		 secsto,shutdown_message->result());
	    if(shutdown_message->result()==5){
		avml(AVML_DEBUG, "shutdown aborted by user\n");
		// the user has aborted shutdown
		delete shutdown_message;
		shutdown_message=NULL;
		shutdown_checkbox->setChecked(0);
	    }
	    else if (secsto>0)
	    {
		// we still have grace time before shutdown
		char buf[300];
		sprintf(buf,"System is going to shutdown\n\n in %d seconds\n",secsto);
		shutdown_message->setText(buf);
		shutdown_message->show();
	    }
	    else
	    {
		// it's time to do the shutdown
		avml(AVML_DEBUG, "shutdown T=0 secs\n");
		save();

		sleep(5);

		if(shutdown_mode==2){
		    // with nvram-wakeup
		    long timestamp=0;

		    if(next_recording){
			QDateTime nextdate=next_recording->getStartTime();

			//int timediff=calcTimeDiff(nextdate);

			// argl - you cant get a timestamp from a QDateTime
			// we have to calculate it ourselves
			QDateTime bigbang=QDateTime();
			bigbang.setTime_t(0);
			timestamp=bigbang.secsTo(nextdate);

			// check if system is running localtime different from UTC
			QDateTime checktime=QDateTime();
			checktime.setTime_t(timestamp);
			long timestamp2=bigbang.secsTo(checktime);
			// if timediff!=0, we're in localtime
			int timediff=timestamp-timestamp2;
			// I have no idea if this works on systems that their hw/rtc/system-clock with UTC

			avml(AVML_DEBUG,
			     "timestamp=%ld timestamp2=%ld diffsecs=%d diffmin=%d nextdate=%s bigbang=%s checktime=%s\n",
			     timestamp,timestamp2,timediff,timediff/60,
			     nextdate.toString(my_dateformat).latin1(),
			     bigbang.toString(my_dateformat).latin1(),
			     checktime.toString(my_dateformat).latin1());

			timestamp+=timediff;
			timestamp-=RI("ShutdownRebootTimespan",5)*60;

			QDateTime reboot_time=QDateTime();
			reboot_time.setTime_t(timestamp);

			avml(AVML_DEBUG, "system will reboot at timestamp %ld (%s)\n",
			     timestamp, reboot_time.toString(my_dateformat).latin1());
			//	    printf(
		    }//end of 'we have a next recording'

		    // set nvram timer with nvram-wakeup
		    // if no next recording, timestamp wil be 0
		    // and nvram-setup will disable RTC alarm wakeup

		    avml(AVML_DEBUG, "setting nvram\n");
		    char cbuf[300];
		    sprintf(cbuf,"sudo avicap-setnvram %ld",timestamp);
		    system(cbuf);

		    sleep(10);
		}// end of 'shutdown mode 2'

		// we do this in shutdown mode 1 and 2

		// SHUTDOWN NOW

		system("sudo avicap-shutdown &");

		//exit avicap
		exit(0);
	    }// end of 'its time do shutdown now'
	}// end of 'we have shutdown message open'
    }// end of 'shutdown is checked'
}

/* **************************************** */

void TimerTable::stop_current_recording(){
    // stop old recording
    AviCapDialog *dialog_old=my_v4lw->getCaptureDialog();
    if(dialog_old!=NULL){
	// there is a recording dialog open

	CaptureProcess *capproc=dialog_old->getCaptureProcess();

	if(capproc!=NULL){
	    // there's a recording running
	    avml(AVML_DEBUG, "current recording has been stopped\n");
	    dialog_old->stop();
	    dialog_old->close();
	}
    }
    // the dialog itself doesn't get closed

    //reset the codec
    // global_savename=RS("NamedCodecsCurrent","-Default-");
}

/* **************************************** */

// helper routine to calc the stop DateTime  from the given
// start DateTime and stop Time

QDateTime TimerTable::calc_stopDateTime(QDateTime start_datetime,QTime stop_time){
    QTime start_time=start_datetime.time();
    QDate start_date=start_datetime.date();

    QDateTime stop_datetime(start_date,stop_time);

    if(start_time > stop_time ){
	// the recording ends the next day
	stop_datetime=stop_datetime.addDays(1);
    }

    return stop_datetime;
}

/* **************************************** */

// loads settings from registry

void TimerTable::load(){
    int i=0;
    char buf[300];

    //    QDateTime currenttime=

    sprintf(buf,"TT-%02d-StartTime",i);
    avm::string starttimestr=RS(buf,"NONE");

    while(starttimestr!="NONE"){

	sprintf(buf,"TT-%02d-StopTime",i);
	avm::string stoptimestr=RS(buf,"NONE");

	QDateTime starttime;
	QTime stoptime;

	//starttime=QDateTime::fromString(starttimestr.c_str());
	//stoptime=QTime::fromString(stoptimestr.c_str());

	starttime=dateTimeFromString(starttimestr.c_str());
	stoptime=dateTimeFromString(stoptimestr.c_str()).time();



	sprintf(buf,"TT-%02d-Channel",i);
	int channel=RI(buf,0);

	sprintf(buf,"TT-%02d-When",i);
	int when=RI(buf,0);

	sprintf(buf,"TT-%02d-WhenDay",i);
	int day=RI(buf,0);

	sprintf(buf,"TT-%02d-Filename",i);
	avm::string filename=RS(buf,"./movie.avi");

	sprintf(buf,"TT-%02d-Description",i);
	avm::string desc=RS(buf,"untitled");

	sprintf(buf,"TT-%02d-NamedCodec",i);
	avm::string codec=RS(buf,"-Default-");

	TimerTableItem *item;
	item=new TimerTableItem(table,this,my_v4lw,channel,starttime,stoptime,QString(filename.c_str()),QString(desc.c_str()),when,day,QString(codec.c_str()));


	sprintf(buf,"TT-%02d-Active",i);
	int ison=RI(buf,1);

	item->setOn(ison);

	i++;

	sprintf(buf,"TT-%02d-StartTime",i);
	starttimestr=RS(buf,"NONE");
    }
}

/* **************************************** */

// saves settings into registry

void TimerTable::save(){
    int i=0;
    char buf[200];

    TimerTableItem *item=(TimerTableItem *)table->firstChild();

    while(item!=NULL){
	sprintf(buf,"TT-%02d-StartTime",i);
	//WS(buf,item->getStartTime().toString(my_dateformat).latin1());
	//WS(buf,item->getStartTime().toString().latin1());
	WS(buf,dateTimeToString(item->getStartTime()));
	sprintf(buf,"TT-%02d-StopTime",i);
	WS(buf,dateTimeToString(item->getStopDateTime()));

	sprintf(buf,"TT-%02d-Channel",i);
	WI(buf,item->getChannel());

	sprintf(buf,"TT-%02d-When",i);
	WI(buf,item->getWhen());

	sprintf(buf,"TT-%02d-WhenDay",i);
	WI(buf,item->getDay());

	sprintf(buf,"TT-%02d-Filename",i);
	WS(buf,item->getFilename().latin1());

	sprintf(buf,"TT-%02d-Description",i);
	WS(buf,item->getDescription().latin1());

	sprintf(buf,"TT-%02d-NamedCodec",i);
	WS(buf,item->getNamedCodec().latin1());

	sprintf(buf,"TT-%02d-Active",i);
	WI(buf,item->isOn());

	item=(TimerTableItem *)item->nextSibling();
	i++;
    }

    // make sure that's the last one...

    sprintf(buf,"TT-%02d-StartTime",i);
    WS(buf,"NONE");

    // actually save to file
    avm::RegSave();
    avml(AVML_DEBUG, "registry saved to file\n");
}

int free_diskspace(avm::string path){
    int free_space=0;

    struct statfs statbuf;
    int res=statfs(path.c_str(),&statbuf);
    if(res==-1){
	printf("cannot statfs: %s\n",path.c_str());
	free_space=0;
    }
    else{
	double long free_bytes=statbuf.f_bsize*statbuf.f_bavail;
	free_space=(int)(free_bytes/(1024*1024));
    }

    return free_space;
}

QString dateTimeToString(QDateTime dt)
{
    QString str;
    int year=dt.date().year();
    int month=dt.date().month();
    int day=dt.date().day();

    str=QString().sprintf("%04d-%02d-%02d/",year,month,day)+dt.time().toString();

    return str;
}

QDateTime dateTimeFromString(QString datestr)
{
    // str has to be like this: 2003-04-06/18:14:11
    QString yearstr=datestr.mid(0,4);
    QString monstr=datestr.mid(5,2);
    QString daystr=datestr.mid(8,2);

    QString hourstr=datestr.mid(11,2);
    QString minstr=datestr.mid(14,2);
    QString secstr=datestr.mid(17,2);

    QDateTime datetime;

    datetime.setDate(QDate(yearstr.toInt(),monstr.toInt(),daystr.toInt()));
    datetime.setTime(QTime(hourstr.toInt(),minstr.toInt(),secstr.toInt()));

    //  printf("strings %s %s %s    %s %s %s\n",
    //	 yearstr.latin1(),monstr.latin1(),daystr.latin1(),
    //	 hourstr.latin1(),minstr.latin1(),secstr.latin1());

    //printf("parsed %s to %s\n",datestr.latin1(),datetime.toString().latin1());
    return datetime;
}

#if QT_VERSION<300

ADateTimeEdit::ADateTimeEdit(QWidget *w,QString name) : QLineEdit(w)
{
    datetime=QDateTime::currentDateTime();
}

void ADateTimeEdit::updateFromWidget()
{
    QString wstr=text();
    datetime=dateTimeFromString(wstr);
}

QString ADateTimeEdit::toString(QDateTime dt)
{
    return dateTimeToString(dt);

}

void ADateTimeEdit::setTime(QTime newtime)
{
    datetime.setTime(newtime);
    setText(toString(datetime));
}

void ADateTimeEdit::setDateTime(QDateTime newdatetime)
{
    datetime=newdatetime;
    QString helper=toString(datetime);
    setText(helper);
}

#endif
