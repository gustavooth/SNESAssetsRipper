#ifndef SAVESTATEPALETTESELECTOR_H
#define SAVESTATEPALETTESELECTOR_H

#include "snespalette.h"

#include <QGraphicsScene>
#include <QWidget>
#include "graphicsclickableview.h"

namespace Ui {
class SavestatePaletteSelector;
}

class SavestatePaletteSelector : public QWidget
{
    Q_OBJECT

public:
    explicit SavestatePaletteSelector(QWidget *parent = nullptr);
    ~SavestatePaletteSelector();

    void            setPalettesData(const QByteArray data);
    SNESPalette     currentPalette() const;
    void            retranslate();
    bool            selectPalette(int index);

signals:
    void    paletteChanged();

private:
    Ui::SavestatePaletteSelector *ui;
    QGraphicsScene* palScene;
    SNESPalette     palette;

    QVector<SNESPalette> palettes;

    void            setPaletteScene(SNESPalette pal);
};

#endif // SAVESTATEPALETTESELECTOR_H
