#include "graphicsclickableview.h"


GraphicsClickableView::GraphicsClickableView(QWidget *parent) : QGraphicsView (parent),
    m_current_item(nullptr)
{

}

QGraphicsItem* GraphicsClickableView::getClickedItem()
{
    return m_current_item;
}

void GraphicsClickableView::mouseReleaseEvent(QMouseEvent *env)
{
    if (scene() == nullptr)
        return;
    QGraphicsItem* item = scene()->itemAt(mapToScene(env->pos()), transform());
    if (item != nullptr)
    {
        if (item != m_current_item)
        {
            m_current_item = item;
            emit itemClicked();
        }
    }
}
