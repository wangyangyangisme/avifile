#ifndef VIDCONFIG_H
#define VIDCONFIG_H

//#include <sys/vfs.h>

#include "vidconf_p.h"
#include <videoencoder.h>

#include <qlistview.h>

class v4lxif;
class V4LWindow;

extern int free_diskspace(avm::string path);

class DirPoolItem : public QCheckListItem
{
    int keep_free; // in MB
    QString pathname;
public:
    DirPoolItem(QListView *li, QString path, int keepfree) :
	QCheckListItem( li, "test", QCheckListItem::CheckBox )
    {
	pathname=path;
	keep_free=keepfree;

	setText(0,pathname);
	setText(1, QString().sprintf("%5d MB",keep_free));
    }

    const QString& getPath() const { return pathname; };
    int getKeepFree() const { return keep_free; };
    int updateSpaceLeft()
    {
	int free_mb=free_diskspace(pathname.latin1());
	setText(2, QString().sprintf("%5d MB",free_mb));
	return free_mb;
    }
};

class VidConfig : public VidcapConfigDialog
{
    Q_OBJECT;

    avm::VideoEncoderInfo _info;
    avm::VideoEncoderInfo _named_info;
    avm::string _savename;
    int _regnumber;

    V4LWindow* m_pV4lWin;
public:
    VidConfig( QWidget* parent, v4lxif* v4l, V4LWindow* w );
    ~VidConfig();
    void savePage(int);
    QListView *getDirPool() { return lv_dirpool; };
public slots:
    virtual void change_filename();
    virtual void change_codec();
    virtual void toggle_audio(bool);
    virtual void toggle_segmented(bool);
    virtual void toggle_limitsize(bool);
    virtual void toggle_limittime(bool);
    virtual void updateCodecName();
    virtual void accept();

    virtual void apply();

    virtual void dirpool_add();
    virtual void dirpool_rem();

    void set_password();
    void lock_password();

    void changed_named_codec(int nr);

    void ListCodecs(avm::vector<avm::CodecInfo>& codec_list);
};

#endif // VIDCONFIG_H
