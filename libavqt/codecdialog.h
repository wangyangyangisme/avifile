#ifndef CODECCONFIG_H
#define CODECCONFIG_H

#include "okdialog.h"
#include <videoencoder.h>

#define HAVE_NEWQT

// ok - MOC can't work with macros
// thus using QWidget for QDial objects
class QLCDNumber;
class QListBox;
class QListView;
class QListViewItem;
class QLineEdit;
class QComboBox;
class QCheckBox;

class QavmCodecDialog : public QavmOkDialog
{
    Q_OBJECT;

    class Input: public QavmOkDialog
    {
	QLineEdit* m_pEdit;
        QString val;
    public:
	Input(QWidget* parent, const QString& title, const QString& defval);
	virtual void accept();
	float getFloat() const { return val.toFloat(); }
	int getInt() const { return val.toInt(); };
	const char* getString() const { return val.latin1(); }
    };

    class InputSelect: public QavmOkDialog
    {
	QComboBox* m_pBox;
	const avm::vector<avm::string>& _options;
	int _defval;
    public:
	InputSelect(QWidget* parent, const QString& title, const avm::vector<avm::string>& options, int defval);
	virtual void accept();
	int value() const { return _defval; }
    };


public:
    QavmCodecDialog( QWidget*, const avm::vector<avm::CodecInfo>& codecs,
		    avm::CodecInfo::Direction dir = avm::CodecInfo::Encode );
    ~QavmCodecDialog();
    avm::VideoEncoderInfo getInfo();

    int getCurrent() const;
    void setCurrent(int i);
public slots:
    virtual void about();
    virtual void accept();
    virtual void changeAttr( QListViewItem* item );
    virtual void clickedAttr( QListViewItem* item );
    virtual void selectCodec();
    virtual void shortNames(int);

    void codecUp() { codecMove( -1 ); }
    void codecDown() { codecMove( 1 ); }
    void codecTop() { codecMove( 0 ); }
    void codecBottom() { codecMove( -1000 ); }

    // Qt2.0 compatibility
    void rightClickedAttr( QListViewItem* item, const QPoint&, int );

protected:
    void addAttributes( const avm::CodecInfo&, const avm::vector<avm::AttributeInfo>& ci );
    void codecMove( int dir );
    void codecUpdateList();
    void createGui();
    void createLCD( QWidget* parent );
    void createMoveGroup( QWidget* parent );

    const avm::vector<avm::CodecInfo>& m_Codecs;
    avm::vector<unsigned> m_Order;
    avm::CodecInfo::Direction m_Direction;
    QLCDNumber* m_pLCDNumber1;
    QLCDNumber* m_pLCDNumber2;
    QWidget* m_pKeyframe;
    QWidget* m_pQuality;
    QCheckBox* m_pShortBox;
    QListBox* m_pListCodecs;
    QListView* m_pTabAttr;
};

#endif // CODECCONFIG_H
