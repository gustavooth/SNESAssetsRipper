#include "savestatepaletteselector.h"
#include "ui_savestatepaletteselector.h"

SavestatePaletteSelector::SavestatePaletteSelector(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SavestatePaletteSelector)
{
    ui->setupUi(this);
}

SavestatePaletteSelector::~SavestatePaletteSelector()
{
    delete ui;
}
