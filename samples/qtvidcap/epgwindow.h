/*
 EPG window for AviCap

 written 2003 by Alexander Rawass (alexannika@users.sourceforge.net)
 */

#ifndef EPGWINDOW_H
#define EPGWINDOW_H

#include <avm_stl.h>
#include <avm_locker.h>

#include <qwidget.h>
#include <qtimer.h>
#include <qlistview.h>
#include <qdatetime.h>
#include <qdom.h>
#include <qvaluelist.h>
#include <qregexp.h>

class v4lxif;
class V4LWindow;

class QPopupMenu;
class QLabel;
class QListView;
class QComboBox;
class QDateTimeEdit;
class QTimeEdit;
class QSplitter;
class QGroupBox;
class QPushButton;
class QButtonGroup;
class QLineEdit;
class QListViewItem;
class QFrame;
class QMessageBox;
class QHBoxLayout;
class QVGroupBox;
class QHGroupBox;
class QHBox;
class QVBox;
class QTable;
class QSpinBox;

#if QT_VERSION >=300
class QTextEdit;
#else
#include <qmultilineedit.h>
#define QTextEdit QMultiLineEdit
#endif

class TimerTable;
class TimerTableItem;

#if QT_VERSION>=300
static QString epg_timeformat="hh:mm";
static QString epg_dateformat="dd MMM yyyy (ddd)";
static QString epg_startdateformat=("dd MMM yyyy  "+epg_timeformat+"  ddd");
static QString epg_stopdateformat=epg_timeformat;
#else
#define epg_timeformat
#define epg_dateformat
#define epg_startdateformat
#define epg_stopdateformat
#endif

static const QString block_string="block:";
static const QString high_string="high:";
static const QString both_string="both:";
static const QString title_string="title:";
static const QString desc_string="desc:";

static int strip_x_default=100;
static int strip_y_default=60*24;

static float zoom_increment=0.5;

class EpgProgram {
public:
    QDateTime start;
    QDateTime stop;
    QString title;
    QString desc;
    int blockmode;
    int y_stop,y_start;
    int channelnr;
};

typedef QValueList<EpgProgram *> EpgProgramList;
typedef QValueListIterator<EpgProgram *> EpgProgramListIterator;

class EpgIdChannel;

class EpgChannel {
public:
    QString chid;
    int channelnr;
    EpgIdChannel *idchannel;
    EpgProgramList programs;
};

typedef QValueList<EpgChannel *> EpgChannelList;
typedef  QValueListIterator<EpgChannel *> EpgChannelListIterator;


class EpgDay {
public:
    QDate date;
    int itemnr;
    EpgChannelList channels;
};

typedef QValueList<EpgDay *> EpgDayList;
typedef QValueListIterator<EpgDay *> EpgDayListIterator;

class EpgChannelStrip;

class EpgIdChannel {
public:
    QString epgid;
    QString epgname;
    QString username;
    int chnr;
    EpgChannelStrip *strip;
    QLabel *conf_channel_label;
    QComboBox *conf_channel_box;
    QPushButton *color_button;
    QColor color;
};

typedef QValueList<EpgIdChannel *> EpgChIdList;
typedef QValueListIterator<EpgIdChannel *> EpgChIdListIterator;

class EpgWindow;

class EpgRegexp {
public:
    QRegExp regexp;
    QString regstr;
    int blockmode;
    int wheremode;
};
typedef QValueList<EpgRegexp *> EpgRegexpList;
typedef QValueListIterator<EpgRegexp *> EpgRegexpListIterator;

enum blockmode_t { regexp_errorblock,regexp_unset,regexp_nomatch,regexp_highlight,regexp_block };
enum wheremode_t { regexp_errorwhere,regexp_title,regexp_desc,regexp_both };

class EpgChannelStrip : public QWidget {
    Q_OBJECT

public:
    EpgChannelStrip(QWidget *parent,EpgWindow *win,EpgIdChannel *new_channel);
    ~EpgChannelStrip() { return ; };

    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
    void drawStrip(int x,int y);
    void mouseMoveEvent(QMouseEvent *e);
    void leaveEvent(QEvent *e);
    int blockMode(EpgProgram *prog);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);

    public slots:
	void addblock();
	void addhighlight();
	void addrecording();

private:
    void calcYfromTime(int &y_start,int &y_stop,QTime starttime,QTime stoptime);

    EpgIdChannel *chid;
    EpgWindow *epgwin;
    EpgProgram *last_mouse_program;
    QPopupMenu *popmenu;
    EpgProgram *pop_program;

    bool mid_move;
    int mid_move_x,mid_move_y;
};

class EpgTableItem : public QCheckListItem {
public:
    EpgTableItem(QListView *parent,EpgWindow *epgwin,QDateTime start_date,QDateTime stop_date,QString channel_str,QString title,QString desc);
    ~EpgTableItem();

    int compare(QListViewItem *other,int col, bool ascend) const ;

    QDateTime getStartTime() const { return start_date; };
    QDateTime getStopDateTime() const { return stop_date; };

    QDateTime start_date,stop_date;
    QString channel,title,desc,chname;

    EpgWindow *epgwin;
};

class EpgWindow : public QWidget {
    Q_OBJECT;

public:
    EpgWindow(V4LWindow *v4l,TimerTable *timer);
    ~EpgWindow();
    QString findNameToId(QString channel);
    void makeConfigWidget(QWidget *parent);
    void addRecording(EpgProgram *prog);
    bool isBlocked(int chnr);
    void lock(bool lock);
    void calcStripSize();
    void resizeStrips();

    EpgDayList epglist;
    EpgChIdList epgChannels;
    int current_date_item;
    bool regexp_dirty;
    TimerTable *timertable;

    EpgRegexpList regexpList;
    TimerTableItem *next_recording,*current_recording;

    QLabel *bottom_label;
    QTextEdit *bottom_text;
    QWidget *gridview;
    QScrollView *gridscroll;
    QHeader *gridheader;
    QTable *gridtable;
    QTextEdit *regexp_edit;
    int zoom_step;

    int strip_y_size,strip_x,strip_y,hours_before,hours_after;
    int strip_y_before,strip_y_after,strip_y_today;
    int strip_y_alldays,y_subtract;

signals:
    void emitAddRecording(EpgProgram *prog);

public slots:
    void display_toggled();
    void day_selected(int nr);
    void okSettings();
    void colorSelector();
    void rereadRegexps();
    void show_config();
    void show_regexps();
    void setCurrentRecording(TimerTableItem *titem);
    void setNextRecording(TimerTableItem *titem);
    void timertableModified();
    void timer();
    void zoomIn();
    void zoomOut();

private:
    void load();
    void load(QString fname);
    void parse(QDomDocument &dom);
    void printElem(QDomElement &elem);

    void doProgramme(QDomElement &elem);
    void doChannel(QDomElement &elem);

    QDateTime parseDate(QString datestr);
    void initWidgets();
    void makeChannelStrips();

    void addEntry(QDateTime start_date,QDateTime stop_date,QString channel_str,QString title,QString desc);

    QTimer *epg_timer;

    QListView *ctable,*ptable;
    QComboBox *day_selector;
    QPushButton *display_toggle;
    QPushButton *config_button,*regexp_button;
    QWidget *centralw;
    QVGroupBox *regexp_box;
    QPushButton *zoom_in_button,*zoom_out_button;
    QHBoxLayout *gridlayout;

    QString provider;
    int channel_row;

    QLineEdit *conf_dir_edit;
    QScrollView *conf_channel_scroll;
    QGridLayout *conf_channel_lay;
    QVBox *conf_channel_view;
    QVGroupBox *config_widget;
    QPushButton *ok_button;

    QSpinBox *hours_before_spinbox,*hours_after_spinbox;
    QSpinBox *mins_before_spinbox,*mins_after_spinbox;

    QPushButton *reg_ok_button,*reg_reread_button;

    int display_mode;
    int today_item;

    V4LWindow *v4lwin;

    long local_timediff;

    int ACnumChannels(){ return v4lwin->getStations(); };
    QString ACchannelName(int i) { return QString(v4lwin->getXawtvStation(i).c_str()); };
};

#endif
