#include "graphicspalettecoloritem.h"
#include "paletteeditor.h"
#include "ui_paletteeditor.h"

#include <QDebug>
#include <QMessageBox>
#include <QFile>
#include <QColorDialog>
#include <QCloseEvent>

PaletteEditor::PaletteEditor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PaletteEditor),
    colorItem(nullptr),
    m_palette(16),
    m_original(16),
    m_selected(-1)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setMaximumSize(640, 680);
    scene = new QGraphicsScene();
    ui->paletteGView->setScene(scene);
    connect(ui->paletteGView, &GraphicsClickableView::itemClicked, [=] {
        GraphicsPaletteColorItem* clicked = static_cast<GraphicsPaletteColorItem*>(ui->paletteGView->getClickedItem());
        if (clicked == nullptr)
            return;
        if (clicked->iColor >= (unsigned)m_palette.colors.size())
            return;
        colorItem = clicked;
        m_selected = (int)clicked->iColor;
        updateSelectionUi();
    });
    connect(ui->lineEditR, &QLineEdit::editingFinished, this, &PaletteEditor::editColor);
    connect(ui->lineEditG, &QLineEdit::editingFinished, this, &PaletteEditor::editColor);
    connect(ui->lineEditB, &QLineEdit::editingFinished, this, &PaletteEditor::editColor);
}


void    PaletteEditor::setPalette(SNESPalette pal)
{
    m_original = pal;
    m_palette = pal;
    m_selected = pal.colors.isEmpty() ? -1 : 0;
    colorItem = nullptr;
    buildScene();
    updateSelectionUi();
    if (pal.size == 16)
        ui->paletteSize16Radio->setChecked(true);
    if (pal.size == 8)
        ui->paletteSize8Radio->setChecked(true);
    if (pal.size == 4)
        ui->paletteSize4Radio->setChecked(true);
    if (pal.size == 2)
        ui->paletteSize2Radio->setChecked(true);
}

int PaletteEditor::selectedIndex() const
{
    if (colorItem != nullptr && colorItem->iColor < (unsigned)m_palette.colors.size())
        return (int)colorItem->iColor;
    if (m_selected >= 0 && m_selected < m_palette.colors.size())
        return m_selected;
    return -1;
}

void PaletteEditor::buildScene()
{
    scene->clear();
    scene->setBackgroundBrush(QColor(0x24, 0x27, 0x2b));
    unsigned int j = 0;
    unsigned int i = 0;
    unsigned colPos = 0;
    foreach(SNESColor col, m_palette.colors)
    {
        GraphicsPaletteColorItem *newItem = new GraphicsPaletteColorItem();
        newItem->color = col.rgb;
        newItem->iColor = colPos;
        newItem->setPos(i * newItem->boundingRect().width() + i, j * newItem->boundingRect().width() + j);
        scene->addItem(newItem);
        i++;
        colPos++;
        if (i % 8 == 0) {
            j++;
            i = 0;
        }
    }
}

void PaletteEditor::updateSelectionUi()
{
    int idx = selectedIndex();
    if (idx < 0)
    {
        ui->lineEditIndex->clear();
        ui->lineEditR->clear();
        ui->lineEditG->clear();
        ui->lineEditB->clear();
        ui->lineEditSnes->clear();
        ui->pickColorButton->setStyleSheet(QString());
        return;
    }
    const SNESColor &col = m_palette.colors[idx];
    ui->lineEditIndex->setText(QString::number(idx));
    ui->lineEditR->setText(QString::number(qRed(col.rgb), 16));
    ui->lineEditG->setText(QString::number(qGreen(col.rgb), 16));
    ui->lineEditB->setText(QString::number(qBlue(col.rgb), 16));
    ui->lineEditSnes->setText(QString::number(col.snes, 16));
    QColor bg(col.rgb);
    int lum = (299 * bg.red() + 587 * bg.green() + 114 * bg.blue()) / 1000;
    QColor fg = (lum > 128) ? QColor(0, 0, 0) : QColor(255, 255, 255);
    ui->pickColorButton->setStyleSheet(
        QString("QPushButton { background-color: %1; color: %2; }").arg(bg.name(), fg.name()));
}

void PaletteEditor::applyRgbToSelected(int r, int g, int b)
{
    int idx = selectedIndex();
    if (idx < 0)
        return;
    m_palette.colors[idx].setRgb(qRgb(r, g, b));
    buildScene();
    updateSelectionUi();
    scene->update();
    emit paletteChanged();
}

SNESPalette PaletteEditor::palette() const
{
    return m_palette;
}

void PaletteEditor::setRomFile(QString fileName)
{
    rom = fileName;
}

PaletteEditor::~PaletteEditor()
{
    delete ui;
}

void    PaletteEditor::editColor()
{
    bool    ok;
    int idx = selectedIndex();
    if (idx < 0)
        return;
    applyRgbToSelected(ui->lineEditR->text().toUInt(&ok, 16), ui->lineEditG->text().toUInt(&ok, 16), ui->lineEditB->text().toUInt(&ok, 16));
}


void PaletteEditor::on_lineEditSnes_editingFinished()
{
    bool    ok;
    int idx = selectedIndex();
    if (idx < 0)
        return;
    SNESColor& col = m_palette.colors[idx];
    col.setSNES(ui->lineEditSnes->text().toUInt(&ok, 16));
    buildScene();
    updateSelectionUi();
    scene->update();
    emit paletteChanged();
}

void PaletteEditor::on_pickColorButton_clicked()
{
    int idx = selectedIndex();
    if (idx < 0)
        return;
    QColor current(m_palette.colors[idx].rgb);
    QColor picked = QColorDialog::getColor(current, this, tr("Pick a color"));
    if (!picked.isValid())
        return;
    applyRgbToSelected(picked.red(), picked.green(), picked.blue());
}


//318C 0000 004A 0012 0018 001E 010C 0170 02BE 3086 614C 5A90 7356 214E 4256 635E

static QByteArray   sbData()
{
    uchar rawData[32] = {0x8C, 0x31, 0, 0, 0x4A, 0, 0x12, 0, 0x18, 0, 0x1E, 0, 0x0C, 0x01,
                        0x70, 0x01, 0xBE, 0x02, 0x86, 0x30, 0x4C, 0x61, 0x90, 0x54, 0x56,
                         0x73, 0x4E, 0x21, 0x56, 0x42, 0x5E, 0x63};
    return QByteArray((char*)rawData, 16);
}


void PaletteEditor::on_searchButton_clicked()
{
    QFile romQFile(rom);
    romQFile.open(QIODevice::ReadOnly);
    QByteArray romData = romQFile.readAll();
    QByteArray paletteData = m_palette.encode();
    //QByteArray paletteData = sbData();
    qDebug() << paletteData;
    int pos = romData.indexOf(paletteData);
    qDebug() << pos;
    QString text;
    if (pos == -1)
        text = tr("No corresponding data found in the rom");
    else {
        text = QString(tr("Data for palette found at : %1")).arg(pos, 0, 16);
    }
    QMessageBox::information(this, tr("Palette Data "),  text);
}

void PaletteEditor::on_cancelPushButton_clicked()
{
    // Discard every change made since the dialog was opened.
    m_palette = m_original;
    m_selected = m_palette.colors.isEmpty() ? -1 : 0;
    colorItem = nullptr;
    buildScene();
    updateSelectionUi();
    emit paletteChanged();
    hide();
}

void PaletteEditor::on_applyPushButton_clicked()
{
    // Changes were already previewed live; just confirm and close.
    emit paletteChanged();
    hide();
}

void PaletteEditor::closeEvent(QCloseEvent *event)
{
    // Closing the window behaves like Cancel: changes are not applied.
    m_palette = m_original;
    m_selected = m_palette.colors.isEmpty() ? -1 : 0;
    colorItem = nullptr;
    buildScene();
    updateSelectionUi();
    emit paletteChanged();
    event->accept();
}

void PaletteEditor::retranslate()
{
    ui->retranslateUi(this);
}
