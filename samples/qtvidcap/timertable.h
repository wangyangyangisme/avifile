/*
 TimerTable window for AviCap

 written 2003 by Alexander Rawass (alexannika@users.sourceforge.net)
 */

#ifndef TIMERTABLE_H
#define TIMERTABLE_H

#include <avm_stl.h>
#include <avm_locker.h>

#include <qdialog.h>
#include <qdatetime.h>
#include <qlistview.h>
#include <qtimer.h>

class v4lxif;
class V4LWindow;

class QPopupMenu;
class QLabel;
class QListView;
class QComboBox;
class QCheckBox;
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

class TimerTable;
class EpgWindow;
class EpgProgram;

#define WATCHDOG_TIMERTABLE "/tmp/avicap-watchdog/timertable"

QDateTime dateTimeFromString(QString datestr);
QString dateTimeToString(QDateTime dt);

#if QT_VERSION<300
class ADateTimeEdit : public QLineEdit
{
public:
    ADateTimeEdit(QWidget *w,QString name);
    void setDateTime(QDateTime datetime);
    void setTime(QTime time);
    void updateFromWidget();
    QDateTime dateTime() { updateFromWidget(); printf("datetime: %s\n",datetime.toString().latin1()); return datetime; };
    QTime time() { updateFromWidget(); return datetime.time(); };
    QString toString(QDateTime dt);

private:
    QDateTime datetime;

#define QDateTimeEdit ADateTimeEdit
#define ATimeEdit ADateTimeEdit
#define QTimeEdit ATimeEdit
};
#endif

#if QT_VERSION>=300
static QString my_timeformat="hh:mm:ss";
static QString my_dateformat=("dd MMM yyyy  "+my_timeformat+"  ddd");
#else
#define my_timeformat
#define my_dateformat
#endif

enum TimerMode { TimerOff,TimerWaiting,TimerRecording };

//#define my_tabletype QListView
//#define my_tableitemtype QListViewItem

class TimerTableItem: public QCheckListItem
{

public:
    TimerTableItem(QListView *parent,TimerTable *timer_win,V4LWindow *v4lw,int ch,QDateTime start,QTime stop,QString fname,QString description,int when,int day,QString codec);
    //TimerTableItem(QWidget *parent);
    ~TimerTableItem();

    void  setTable(int ch,QDateTime start,QTime stop,QString fname,QString description,int when,int day,QString codec);

    void checkWhen();

    int  compare(QListViewItem *other,int col, bool ascend) const;
    void paintCell ( QPainter * p, const QColorGroup & cg, int column, int width, int align );


private:
    QDateTime start_time;
    QTime  stop_time;
    QDateTime stop_datetime;
    int channel_nr;
    QString filename;
    QString description;
    int when,day;
    QString named_codec;

    TimerTable *timer_window;
    V4LWindow *my_v4lwin;

public:
    QDateTime getStartTime() const { return start_time; };
    QTime getStopTime() const { return stop_time; };
    QDateTime getStopDateTime() const { return stop_datetime; };
    QString getFilename() const { return filename; };
    int getWhen() const { return when; };
    int getDay() const { return day; };
    int getChannel() const { return channel_nr; };
    QString getNamedCodec() const { return named_codec; };
    QString getDescription() const { return description; };

};

class TimerTable: public QDialog
{
    Q_OBJECT;
public:

    TimerTable( QWidget* parent, v4lxif* v4l, V4LWindow* w);
    ~TimerTable();
    void save();
    void load();
    void startTimer(bool set_shutdown);
    QDateTime calc_stopDateTime(QDateTime start_datetime,QTime stop_time);
    void setEpgWindow(EpgWindow *win);
    TimerTableItem *firstTableItem() { return (TimerTableItem *)table->firstChild(); };
public slots:
    void addEpgRecording(EpgProgram *prog);
    void button_new_pressed();
    void button_remove_pressed();
    void button_modify_pressed();
    void button_start_pressed();
    void item_clicked(QListViewItem *item);
    void timer();

signals:
    void nextRecordingChanged(TimerTableItem *next_recording);
    void currentRecordingChanged(TimerTableItem *current_recording);
    void timertableChanged();

protected:

private:
    void recreateCodecBox(QString set_name);
    void init_entry_widgets();
    void set_status_label();
    TimerTableItem* find_first_recording();
    TimerTableItem* set_tableitem(TimerTableItem *item);
    void stop_current_recording();
    QString add_current_time_info();
    QString add_current_recording_info();
    QString add_next_recording_info();
    QString add_sanity_check_info();
    void check_for_shutdown();
    QString find_filename(QString filename);

    TimerMode timer_mode;

    v4lxif *my_v4lxif;
    V4LWindow *my_v4lw;
    EpgWindow *epgwin;

    QSplitter *splitter1;
    //QGroupBox  *edit_box;
    QWidget  *edit_box;
    QGroupBox  *all_box;

    QListView *table;
    QComboBox *channelbox;
    QComboBox *codecbox;

    QDateTimeEdit *begin_timeedit;
    QTimeEdit *end_timeedit;
    QLineEdit *filename_edit;
    QLineEdit *description_edit;

    QButtonGroup *edit_buttons;
    QPushButton *new_button;
    QPushButton *modify_button;
    QPushButton *remove_button;

    QButtonGroup *rec_buttons;
    QPushButton *rec_start_button;
    QPushButton *rec_stop_button;
    QCheckBox *shutdown_checkbox;

    QFrame *status_frame;
    QLabel *status_label;

    QComboBox *when_combobox;
    QComboBox *day_combobox;

    QMessageBox *shutdown_message;
    QDateTime shutdown_time;

    TimerTableItem *next_recording,*last_next_recording;
    TimerTableItem *current_recording,*last_current_recording;
    TimerTableItem *last_item_clicked;

    QTimer *my_timer;

public:
    TimerTableItem *getCurrentRecording() { return current_recording; };
    TimerTableItem *getNextRecording() { return next_recording; };
};

#endif
