#ifndef GRAPHICSPALETTEVIEW_H
#define GRAPHICSPALETTEVIEW_H

#include <QGraphicsView>
#include <QMouseEvent>
#include <QObject>
#include <QWidget>

class GraphicsClickableView : public QGraphicsView
{
    Q_OBJECT
public:
    GraphicsClickableView(QWidget* parent);
    QGraphicsItem* getClickedItem();

signals:
    void    itemClicked();
protected:
    void    mouseReleaseEvent(QMouseEvent* env);

private:
    QGraphicsItem* m_current_item;
};

#endif // GRAPHICSPALETTEVIEW_H
