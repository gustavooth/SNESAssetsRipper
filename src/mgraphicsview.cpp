#include "graphicstileitem.h"
#include "mgraphicsview.h"
#include <QScrollBar>
#include <QDebug>

MGraphicsView::MGraphicsView(QWidget *parent) : QGraphicsView (parent)
{
    m_tileUnderCursor = nullptr;
    m_panning = false;
    setMouseTracking(true);
}

GraphicsTileItem *MGraphicsView::tileUnderCursor()
{
    return m_tileUnderCursor;
}

// Ctrl + mouse wheel: zoom the tiles view (steps feed the zoom slider)
void MGraphicsView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier)
    {
        int steps = (event->angleDelta().y() > 0) ? 5 : -5;
        emit zoomRequested(steps);
        event->accept();
        return;
    }
    QGraphicsView::wheelEvent(event);
}

void MGraphicsView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_panning = false;
        m_panStart = event->position().toPoint();
    }
    QGraphicsView::mousePressEvent(event);
}

// Left click + drag pans the view; plain moves update the tile under cursor
void MGraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    if (scene() == nullptr)
        return;

    if (event->buttons() & Qt::LeftButton)
    {
        QPoint delta = event->position().toPoint() - m_panStart;
        if (m_panning || delta.manhattanLength() > 6)
        {
            m_panning = true;
            setCursor(Qt::ClosedHandCursor);
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
            verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
            m_panStart = event->position().toPoint();
            event->accept();
            return;
        }
    }

    QGraphicsItem* item = scene()->itemAt(mapToScene(event->position().toPoint()), transform());
    if (item != nullptr)
    {
        GraphicsTileItem* gItem = (GraphicsTileItem*) item;
        if (gItem != m_tileUnderCursor)
        {
            m_tileUnderCursor = gItem;
            emit tileUnderCursorChanged();
        }
    }
    //event->accept();
}

void MGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_panning = false;
        unsetCursor();
    }
    QGraphicsView::mouseReleaseEvent(event);
}