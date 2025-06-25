#include "graphicspalettecoloritem.h"
#include "savestatepaletteselector.h"
#include "ui_savestatepaletteselector.h"
#include <QDebug>

SavestatePaletteSelector::SavestatePaletteSelector(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SavestatePaletteSelector)
{
    ui->setupUi(this);
    palScene = new QGraphicsScene();
    ui->paletteGraphicsView->setScene(palScene);
    connect(ui->allPaletteGraphicsView, &GraphicsClickableView::itemClicked, this, [=]()
    {
        QGraphicsItem* item = ui->allPaletteGraphicsView->getClickedItem();
        qDebug() << "Item clicked" << item->scenePos();
        setPaletteScene(palettes.at((int)item->y() % 16));
    });
}

SavestatePaletteSelector::~SavestatePaletteSelector()
{
    delete ui;
}

void SavestatePaletteSelector::setPalettesData(const QByteArray data)
{
    const char* cgRAM = data.constData();
    QGraphicsScene* paletteScene = new QGraphicsScene();

    for (unsigned int i = 0; i < 16; i++)
    {
        SNESPalette mPal(QByteArray(cgRAM + i * 16 * 2, 32));
        palettes.append(mPal);
        for (unsigned int j = 0; j < 16; j++)
        {
            paletteScene->addRect(16 * j, i * 16, 16, 16, QPen(Qt::black), QBrush(QColor::fromRgb(mPal.colors[j].rgb)));
        }
    }
    ui->allPaletteGraphicsView->setScene(paletteScene);
    setPaletteScene(palettes.at(0));
}



SNESPalette SavestatePaletteSelector::currentPalette() const
{
    return palette;
}

void SavestatePaletteSelector::setPaletteScene(SNESPalette pal)
{
    unsigned int j = 0;
    unsigned int i = 0;
    unsigned colPos = 0;
    palScene->clear();
    palScene->setBackgroundBrush(Qt::black);
    foreach(SNESColor col, pal.colors)
    {
        GraphicsPaletteColorItem *newItem = new GraphicsPaletteColorItem();
        newItem->color = col.rgb;
        newItem->iColor = colPos;
        newItem->setPos(i * newItem->boundingRect().width() + i, j * newItem->boundingRect().width() + j);
        palScene->addItem(newItem);
        i++;
        colPos++;
        if (i % 8 == 0) {
            j++;
            i = 0;
        }
    }
    palette = pal;
    emit paletteChanged();
}
