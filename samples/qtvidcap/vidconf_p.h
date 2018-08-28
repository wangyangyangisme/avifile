#ifndef VIDCAPCONFIGDIALOG_H
#define VIDCAPCONFIGDIALOG_H

#include "okdialog.h"

class QVButtonGroup;
class QButtonGroup;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QTabWidget;
class QWidget;
class QGroupBox;
class QListView;

class VidcapConfigDialog : public QavmOkDialog
{
    Q_OBJECT;

public:
    VidcapConfigDialog( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
    ~VidcapConfigDialog();

    QTabWidget* tabWidget;
    QComboBox* _Colorspace;
    QComboBox* _Resolution;
    QLineEdit* _CapDevice;
    QLineEdit* _AudDevice;
    QComboBox* _VideoColorMode;
    QComboBox* _VideoChannel;
    QLineEdit* _FileName;
    QPushButton* _ChangeFile;
    QComboBox* _fps;
    QCheckBox* _HaveSegmented;
    QLineEdit* _SegmentSize;
    QComboBox* _listSamp;
    QCheckBox* _HaveAudio;
    QComboBox* _listFreq;
    QComboBox* _listChan;
    QLabel* _codecName;
    QPushButton* _ChangeCodec;
    QButtonGroup* ButtonGroup1;
    QCheckBox* _chkFileSize;
    QLineEdit* _timeLimit;
    QLineEdit* _sizeLimit;
    QCheckBox* _chkTime;

    //shutdown

    //QCheckBox *cb_allow_shutdown;
    QVButtonGroup *rb_buttons;
    QRadioButton *rb_shutdown_never;
    QRadioButton *rb_shutdown_last;
    QRadioButton *rb_shutdown_inbetween;

    QSpinBox *sb_min_timespan;
    QSpinBox *sb_reboot_timespan;
    QSpinBox *sb_shutdown_grace;
    
    QCheckBox *cb_never_overwrite;

    QGroupBox *gb_dirpool;
    QListView *lv_dirpool;
    QPushButton *bt_add_dirpool,*bt_rem_dirpool;
    QLineEdit *le_dirpool_name;
    QSpinBox *qs_dirpool_minfree;

    QCheckBox *cb_log;
    QPushButton *bt_password_set,*bt_password_lock;

    QComboBox *named_codecs_cb;

public slots:
    virtual void apply() = 0;
    virtual void change_codec() = 0;
    virtual void change_filename() = 0;
    virtual void toggle_audio(bool) = 0;
    virtual void toggle_limitsize(bool) = 0;
    virtual void toggle_limittime(bool) = 0;
    virtual void toggle_segmented(bool) = 0;

    virtual void dirpool_add() = 0;
    virtual void dirpool_rem() = 0;
};

#endif // VIDCAPCONFIGDIALOG_H
