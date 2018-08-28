#ifndef OKDIALOG_H
#define OKDIALOG_H

#include <qdialog.h>

class QGridLayout;

// base resizable dialog with Ok Apply Cancel buttons
class QavmOkDialog : public QDialog
{
    Q_OBJECT;
    bool m_bApplyEnabled;
public:
    QavmOkDialog( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
    QGridLayout* gridLayout() { return m_pGl; }
    void setApplyEnabled(bool enabled) { m_bApplyEnabled = enabled; }
    void setApplyName(const QString& apply) { m_Apply = apply; }
    void setOkDefault(bool enabled) { m_bOkDefault = enabled; }
    virtual int exec(); // overload
public slots:
    virtual void apply();

protected:
    QGridLayout* m_pGl;
    QString m_Apply;
    bool m_bOkDefault;
};

#endif // OKDIALOG_H
