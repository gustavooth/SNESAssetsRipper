#ifndef MAINUI_H
#define MAINUI_H

#include <QMainWindow>
#include "lowlevelstuff/src/tile.h"
#include "compressioninterface.h"
#include "graphicstilesscene.h"
#include "paletteeditor.h"
#include "romdataengine.h"
#include "snessavestate.h"
#include "savestatepaletteselector.h"
#include "tilepreset.h"
#include <QGraphicsScene>
#include <QSettings>

namespace Ui {
class MainUI;
}

class MainUI : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainUI(QWidget *parent = nullptr);
    ~MainUI();

private slots:
    void    onTileUnderCursorChanged();

    void on_romOpenButton_clicked();

    void on_headerButtonGroup_clicked(int);

    void on_paletteSNESLocationRadioButton_toggled(bool checked);

    void on_palettePCLocationRadioButton_toggled(bool checked);

    void on_refreshPushButton_clicked();

    void on_paletteGrayRadioButton_toggled(bool checked);

    void on_tilesSNESRadioButton_toggled(bool checked);

    void on_tilesPCRadioButton_toggled(bool checked);

    void on_tilesPerRowSpinBox_valueChanged(int arg1);

    void on_tileZoomHorizontalSlider_valueChanged(int value);

    void on_presetOpenPushButton_clicked();

    void on_presetSavePushButton_clicked();

    void on_pngExportPushButton_clicked();

    void on_pngImportpushButton_clicked();

    void on_compressionComboBox_currentIndexChanged(const QString &arg1);

    void on_tilePatternComboBox_currentIndexChanged(const QString &arg1);

    void on_bankComboBox_currentIndexChanged(int index);

    void on_netaButton_clicked();


    void on_editPaletteButton_clicked();

    void on_openStateButton_clicked();

    void on_pickSavestatePaletteButton_clicked();

private:
    Ui::MainUI      *ui;
    TilePreset      currentSet;
    QSettings*      m_settings;
    QString         lastPresetDirectory;
    QString         lastROMDirectory;
    QString         lastPNGDirectory;
    PaletteEditor*  paletteEditor;
    SNESSavestate       snesSaveState;

    QString         romFile;
    QString         savestateFile;
    unsigned int    tilesPerRow;
    bool            romHasHeader;
    GraphicsTilesScene  *tileScene;
    QGraphicsScene  *palScene;
    ROMDataEngine   dataEngine;

    QList<tile8>    rawTiles;
    SNESPalette     mPalette;
    SavestatePaletteSelector* savestatepaletteselector;

    QMap<QString, CompressionInfo>    compressInfos;

    void    openRom(QString rom);
    bool    loadPreset(const QString &presetFile);
    void    updatePresetWithUi();
    void    updateUiWithPreset();

    void    rebuildGTileView();
    bool    extractTiles();
    void    buildTileScene();
    void    buildPaletteScene();
    void    setGrayscalePalette(unsigned int paletteSize = 16);
    bool    extractPalette();

    void    closeEvent(QCloseEvent* event);
    void    openSavestate(QString path);
};

#endif // MAINUI_H
