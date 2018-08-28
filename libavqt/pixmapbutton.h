#include <qbitmap.h>
#include <qtoolbutton.h>

// maybe use QToolButton
class QavmPixmapButton: public QToolButton
//QPushButton
//QToolButton
{
public:
    QavmPixmapButton(const char* ficon, QWidget* _parent)
	:QToolButton( _parent )
	//:QPushButton( _parent )
    {
	QString file = QString::fromLatin1( PIXMAP_PATH "/" )
	    + QString::fromLatin1( ficon )
	    + QString::fromLatin1( ".ppm" );
	QPixmap p(file);
	p.setMask(p.createHeuristicMask());
	setIconSet(QIconSet(p));
#if QT_VERSION >= 300
//        setFlat(TRUE);
#endif
	//setFixedSize(p.width() + 8, p.height() + 8);
    }
};
