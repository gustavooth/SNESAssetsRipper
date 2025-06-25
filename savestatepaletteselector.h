#ifndef SAVESTATEPALETTESELECTOR_H
#define SAVESTATEPALETTESELECTOR_H

#include <QWidget>

namespace Ui {
class SavestatePaletteSelector;
}

class SavestatePaletteSelector : public QWidget
{
    Q_OBJECT

public:
    explicit SavestatePaletteSelector(QWidget *parent = nullptr);
    ~SavestatePaletteSelector();

private:
    Ui::SavestatePaletteSelector *ui;
};

#endif // SAVESTATEPALETTESELECTOR_H
