#ifndef MGRAPHICSVIEW_H
#define MGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QMouseEvent>
#include <QObject>
#include <QWidget>
#include "lowlevelstuff/src/tile.h"
#include "graphicstileitem.h"

class MGraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    MGraphicsView(QWidget* parent);
    GraphicsTileItem*  tileUnderCursor();

signals:
    void    tileUnderCursorChanged();
    void    zoomRequested(int steps);

protected:
    void    mouseMoveEvent(QMouseEvent* event);
    void    mousePressEvent(QMouseEvent* event);
    void    mouseReleaseEvent(QMouseEvent* event);
    void    wheelEvent(QWheelEvent* event);

private:
    GraphicsTileItem*   m_tileUnderCursor;
    bool                m_panning;
    QPoint              m_panStart;
};

#endif // MGRAPHICSVIEW_H

