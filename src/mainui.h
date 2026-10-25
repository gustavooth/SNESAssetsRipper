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

class McpServer;
class McpDocsDialog;
class QMenu;
class QAction;
class QTranslator;
class TxtTranslator;

namespace Ui {
class MainUI;
}

class MainUI : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainUI(QWidget *parent = nullptr);
    ~MainUI();

    // ---- Programmatic control API (used by the MCP server) ----
    void        openRom(QString rom);
    bool        loadPreset(const QString &presetFile);
    void        refreshView();
    QString     setTileSettings(const QVariantMap &cfg);
    QString     setPaletteSettings(const QVariantMap &cfg);
    bool        exportPngTo(const QString &pngFile, QString *error = nullptr);
    QString     injectPngFrom(const QString &pngFile, bool workOnCopy, bool useImagePalette, QString *error = nullptr);
    bool        saveWindowScreenshot(const QString &pngFile, QString *error = nullptr);
    bool        savePresetTo(const QString &presetFile, QString *error = nullptr);
    bool        openPresetFrom(const QString &presetFile, QString *error = nullptr);
    bool        openSavestateFile(const QString &path, QString *error = nullptr);
    bool        selectSavestatePalette(int index, QString *error = nullptr);
    QString     setPattern(const QString &name);
    void        setZoom(int value);
    QString     setViewBackground(const QString &spec);
    void        applyLanguage(const QString &lang);
    QStringList languageList() const;
    QVariantMap collectState() const;

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

    void on_compressionComboBox_currentIndexChanged(int index);

    void on_tilePatternComboBox_currentIndexChanged(int index);

    void on_bankComboBox_currentIndexChanged(int index);

    void on_netaButton_clicked();


    void on_editPaletteButton_clicked();

    void on_openStateButton_clicked();

    void on_pickSavestatePaletteButton_clicked();

private slots:
    void    toggleMcpServer(bool enabled);
    void    openMcpDocs();

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

    // ---- Options menu / language / MCP ----
    QString         m_language;
    QString         m_translationsDir;
    TxtTranslator  *m_txtTranslator = nullptr;
    QTranslator    *m_qtTranslator = nullptr;
    McpServer      *m_mcpServer = nullptr;
    QMenu          *m_optionsMenu = nullptr;
    QMenu          *m_languageMenu = nullptr;
    QAction        *m_actionMcp = nullptr;
    QAction        *m_actionMcpDocs = nullptr;
    QMenu          *m_viewBgMenu = nullptr;
    QAction        *m_actionViewBgColor = nullptr;
    QAction        *m_actionViewBgChecker = nullptr;
    McpDocsDialog  *m_mcpDocsDialog = nullptr;
    QString         m_viewBackgroundSpec;

    void    buildMenus();
    void    refreshLanguageMenu();
    void    retranslateAll();
    void    applyViewBackground();
    QString defaultLanguage() const;

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
