#ifndef CONFIGDIALOG_IMPL_H
#define CONFIGDIALOG_IMPL_H

#include "configdialog.h"
#include "avm_locker.h"

class QListBoxItem;
class PlayerControl;
class QListBoxItem;

class ConfigDialog_impl : public ConfigDialog
{
    Q_OBJECT;

public:
    ConfigDialog_impl(PlayerControl*);
public slots:
    virtual void accept();
    virtual void reject();
    virtual void apply();

    void onAsyncValChanged(int);
    void onSubAsyncValChanged(int);
    void onAudioResamplingToggled();
    void onNoDefaultTheme();
    void onAudioStreamChanged(int);
    void onBGColor();
    void onFGColor();
    void onFont();
    void onFontChanged();
    void onProxyToggled();
    void onSelectionChanged(QListBoxItem*);
    void onListPressed(QListBoxItem*);
    void onListClicked(QListBoxItem*);
    void onMouseButtonPressed(int, QListBoxItem*, const QPoint&);
private:

    PlayerControl* m_pControl;
    QFont m_defFont;

    QListBoxItem* itemLast;
    avm::PthreadMutex m_Mutex;

    // copy of properties
    int m_iAsyncOriginal;
    int m_iSubAsyncOriginal;
    int m_iAudioStreamOriginal;

    bool m_bFontChanged;
    bool m_bAudioChanged;
    bool m_bVideoChanged;
};

#endif // CONFIGDIALOG_IMPL_H
