#include "compressioninterface.h"
#include "graphicspalettecoloritem.h"
#include "graphicstileitem.h"
#include "mainui.h"
#include "mcpdocsdialog.h"
#include "mcpserver.h"
#include "savestatepaletteselector.h"
#include "txttranslator.h"
#include "ui_mainui.h"

#include <QFile>
#include <QFileDialog>
#include <QDebug>
#include <QtMath>
#include <QDir>
#include <QGraphicsPixmapItem>
#include <QPluginLoader>
#include <QInputDialog>
#include <QShortcut>
#include <QMenuBar>
#include <QMenu>

#include <QRegularExpressionValidator>
#include <QRegularExpression>
#include <QMessageBox>
#include "lowlevelstuff/src/rommapping.h"
#include "lowlevelstuff/src/palette.h"
#include "pngexportimport.h"
#include "injectdialog.h"


MainUI::MainUI(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainUI)
{
    ui->setupUi(this);
    ui->presetOpenPushButton->setIcon(style()->standardPixmap(QStyle::SP_DialogOpenButton));
    ui->presetSavePushButton->setIcon(style()->standardPixmap(QStyle::SP_DialogSaveButton));
    QRegularExpressionValidator* hexValid = new QRegularExpressionValidator(QRegularExpression("[0-F|a-f]{0,8}"));
    QIntValidator* sizeValid = new QIntValidator(0, 1024 * 1024 * 1024);
    romHasHeader = true;

    ui->snesAddrLineEdit->setValidator(hexValid);
    ui->pcAddrLineEdit->setValidator(hexValid);
    ui->palettePCLocationLineEdit->setValidator(hexValid);
    ui->paletteSNESLocationLineEdit->setValidator(hexValid);
    ui->sizeLineEdit->setValidator(sizeValid);
    tilesPerRow = 8;

    tileScene = new GraphicsTilesScene();
    palScene = new QGraphicsScene();
    paletteEditor = new PaletteEditor();
    connect(paletteEditor, &PaletteEditor::paletteChanged, this, [=] {
        mPalette = paletteEditor->palette();
        buildPaletteScene();
        buildTileScene();
    });
    savestatepaletteselector = new SavestatePaletteSelector();
    connect(savestatepaletteselector, &SavestatePaletteSelector::paletteChanged, this, [=]
    {
        mPalette = savestatepaletteselector->currentPalette();
        buildPaletteScene();
        buildTileScene();
    });

    ui->graphicsView->setScene(tileScene);
    ui->paletteGraphicsView->setScene(palScene);
    setGrayscalePalette();
    buildPaletteScene();
    QShortcut* refreshShortcut = new QShortcut(QKeySequence("F5"), this);
    connect(refreshShortcut, &QShortcut::activated, [=]{ this->on_refreshPushButton_clicked(); });
    QShortcut* incShortcut = new QShortcut(QKeySequence("F1"), this);
    connect(incShortcut, &QShortcut::activated, [=]{ this->on_netaButton_clicked(); });


    connect(ui->headerButtonGroup, &QButtonGroup::idClicked, this, &MainUI::on_headerButtonGroup_clicked);
    connect(ui->graphicsView, SIGNAL(tileUnderCursorChanged()), this, SLOT(onTileUnderCursorChanged()));
    compressInfos = ROMDataEngine::compressionInfos();
    unsigned int cpt = 0;
    ui->compressionComboBox->insertItem(cpt, tr("None"));
    ui->compressionComboBox->setItemData(cpt, "None");   // stable id, presets/logic use it
    ui->compressionComboBox->setItemData(cpt, tr("No compression"), Qt::ToolTipRole);
    cpt++;
    foreach(const QString& key, ROMDataEngine::availableCompressions().keys())
    {
            ui->compressionComboBox->insertItem(cpt, key);
            ui->compressionComboBox->setItemData(cpt, key);   // stable id
            ui->compressionComboBox->setItemData(cpt, compressInfos[key].description, Qt::ToolTipRole);
            cpt++;
    }
    QMap<QString, TilesPattern>  patterns = TilesPattern::Patterns();
    cpt = 0;
    foreach (const QString& key, patterns.keys()) {
        ui->tilePatternComboBox->insertItem(cpt, key);
        ui->tilePatternComboBox->setItemData(cpt, patterns[key].description, Qt::ToolTipRole);
        cpt++;
    }
    ui->tilePatternComboBox->setCurrentText("normal");
    if (0) {
        openRom("F:\\Emulation\\Soul Blazer (U) [!]\\Soul Blazer (US).smc");
        loadPreset("F:\\Project\\SNESTilesKitten\\Presets\\Soul Blazer - Hero sprite.stk");
    }
    m_settings = new QSettings("SNESAssetsRipper", "SNESAssetsRipper");
    // One-time migration of settings saved before the fork rename
    QSettings legacySettings("skarsnik.nyo.fr", "SNESTilesKitten");
    if (m_settings->allKeys().isEmpty() && !legacySettings.allKeys().isEmpty())
    {
        const QStringList legacyKeys = legacySettings.allKeys();
        for (const QString &key : legacyKeys)
            m_settings->setValue(key, legacySettings.value(key));
        m_settings->sync();
    }
    if (m_settings->contains("windowGeometry"))
    {
        lastPresetDirectory = m_settings->value("lastPresetDirectory").toString();
        lastROMDirectory = m_settings->value("lastROMDirectory").toString();
        lastPNGDirectory = m_settings->value("lastPNGDirectory").toString();
        restoreGeometry(m_settings->value("windowGeometry").toByteArray());
        restoreState(m_settings->value("windowState").toByteArray());
    }
    setWindowTitle(qApp->applicationName() + " " + qApp->applicationVersion());
    dataEngine.overrideHeaderInfo = true;

    // ---- Options menu / language / MCP server ----
    m_translationsDir = translationsDirectory();
    m_language = m_settings->value("language", defaultLanguage()).toString();
    buildMenus();
    applyLanguage(m_language);
    m_mcpServer = new McpServer(this);
    m_mcpServer->setTarget(this);
    connect(m_mcpServer, &McpServer::logMessage, this, [this](const QString &msg) {
        ui->statusBar->showMessage(msg, 5000);
    });
    // Headless instances (spawned with --mcp-stdio) never touch the TCP
    // server or the saved settings of the GUI instance.
    const bool headlessMode = qApp->property("snesassetsripper/headless").toBool();
    if (!headlessMode && m_settings->value("mcpEnabled", false).toBool())
        m_actionMcp->setChecked(true);   // triggers toggleMcpServer(true)
}

MainUI::~MainUI()
{
    delete ui;
}

void MainUI::onTileUnderCursorChanged()
{
    //qDebug() << "Tile under cursor : " << ui->graphicsView->tileUnderCursor();
    tile8 tile = ui->graphicsView->tileUnderCursor()->rawTile;
    int pcStart;
    if (currentSet.pcTilesLocation > 0)
        pcStart = currentSet.pcTilesLocation;
    else
        pcStart = ROMDataEngine::snesToPC(currentSet.SNESTilesLocation, currentSet.romType);
    int pcAddr = pcStart + tile.id * currentSet.bpp * 8;
    int romAddr = ROMDataEngine::pcToSnes(pcAddr, currentSet.romType);
    ui->tileInfos->setTile(ui->graphicsView->tileUnderCursor()->image, tile.id, pcAddr, romAddr, tile.id * currentSet.bpp * 8);
}

void    show_tile8(tile8 t)
{
    printf("{\n");
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
          printf("%d ", t.data[i + j * 8]);
        printf("\n");
    }
    printf("}\n");
}

bool MainUI::extractTiles()
{
    dataEngine.overridenHeaderInfo = romHasHeader;
    rawTiles = dataEngine.extractTiles(currentSet);
    if (currentSet.compression != "None" && dataEngine.lastUnCompressSize != 0)
    {
        ui->sizeLineEdit->setText(QString::number(dataEngine.lastUnCompressSize));
        ui->statusBar->showMessage(tr("Tiles presetset size get ajusted to the compressed string size"));
    }
    return !rawTiles.isEmpty();
}

bool MainUI::extractPalette()
{
    dataEngine.overridenHeaderInfo = romHasHeader;
    mPalette = dataEngine.extractPalette(currentSet);
    return (mPalette.size != 0);
}


void MainUI::buildTileScene()
{
   tileScene->buildScene(rawTiles, mPalette, currentSet.tilesPattern);
}

void MainUI::buildPaletteScene()
{
    palScene->clear();
    unsigned int j = 0;
    unsigned int i = 0;
    unsigned colPos = 0;
    palScene->setBackgroundBrush(Qt::black);
    foreach(SNESColor col, mPalette.colors)
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
}

void MainUI::setGrayscalePalette(unsigned int paletteSize)
{
    SNESPalette newPal(paletteSize);
    unsigned int psize = paletteSize;
    for (unsigned int i = 0; i < psize; i++)
    {
        QRgb color = qRgb(i * (255 / psize), i * (255 / psize), i * (255 / psize));
        newPal.colors[i].setRgb(color);
    }
    mPalette = newPal;
}

void MainUI::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
    m_settings->setValue("lastROMDirectory", lastROMDirectory);
    m_settings->setValue("lastPresetDirectory", lastPresetDirectory);
    m_settings->setValue("lastPNGDirectory", lastPNGDirectory);
    m_settings->setValue("windowState", saveState());
    m_settings->setValue("windowGeometry", saveGeometry());
}

void MainUI::on_romOpenButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
          tr("Select ROM"), lastROMDirectory, tr("SNES ROM or associated (*.smc *.sfc *.gfx *.bin);;All files (*)"));
    if (!fileName.isNull())
        openRom(fileName);
}


void MainUI::on_headerButtonGroup_clicked(int)
{
    romHasHeader = ui->headerRadioButton->isChecked();
}

void MainUI::rebuildGTileView()
{
    if (currentSet.SNESPaletteLocation == 0 && currentSet.pcPaletteLocation == 0)
        setGrayscalePalette(qPow(2, currentSet.bpp));
    else
        extractPalette();
    if (!extractTiles()) {
        ui->statusBar->showMessage(tr("Error extracting tiles"));
        return;
    }
    buildTileScene();
    buildPaletteScene();
}

void MainUI::on_paletteSNESLocationRadioButton_toggled(bool checked)
{
    ui->paletteSNESLocationLineEdit->setEnabled(checked);
}

void MainUI::on_palettePCLocationRadioButton_toggled(bool checked)
{
    ui->palettePCLocationLineEdit->setEnabled(checked);
}


void MainUI::on_refreshPushButton_clicked()
{
    updatePresetWithUi();
    qDebug() << "Updated preset";
    qDebug() << currentSet;
    rebuildGTileView();
}

void MainUI::on_paletteGrayRadioButton_toggled(bool checked)
{
    if (checked == true)
    {
        currentSet.SNESPaletteLocation = 0;
        currentSet.pcPaletteLocation = 0;
        ui->paletteNoZeroColorCheckBox->setEnabled(false);
    }
    else
        ui->paletteNoZeroColorCheckBox->setEnabled(true);
}

void MainUI::on_tilesSNESRadioButton_toggled(bool checked)
{
    ui->snesAddrLineEdit->setEnabled(checked);
}

void MainUI::on_tilesPCRadioButton_toggled(bool checked)
{
    ui->pcAddrLineEdit->setEnabled(checked);
}

void MainUI::openRom(QString rom)
{
    dataEngine.setRomFile(rom);
    ROMInfo& romInfo = dataEngine.romInfo;
    romHasHeader = romInfo.hasHeader;
    if (romHasHeader)
        ui->headerRadioButton->toggle();
    else
        ui->noHeaderRadioButton->toggle();
    if (romInfo.romType == "LoROM")
        ui->loRomRadioButton->toggle();
    else
        ui->hiRomRadioButton->toggle();
    currentSet.romType = romInfo.romType;
    ui->romFileLineEdit->setText(rom);
    ui->statusBar->showMessage(tr("%1 loaded - %2 - Rom title is : %3").arg(QFileInfo(rom).baseName(), romInfo.romType, romInfo.romTitle));
    romFile = rom;
    QIntValidator* intVal = (QIntValidator*) ui->sizeLineEdit->validator();
    intVal->setTop(romInfo.size);
    lastROMDirectory = QFileInfo(rom).dir().absolutePath();
    unsigned int numberOfBanks = romInfo.size / 0x10000;
    for (unsigned int i = 0; i < numberOfBanks; i++)
    {
        ui->bankComboBox->addItem(QString("Bank $%1").arg(i, 2, 16, QChar('0')));
    }

}

bool MainUI::loadPreset(const QString& presetFile)
{
    if (currentSet.load(presetFile)) {
        updateUiWithPreset();
        lastPresetDirectory = QFileInfo(presetFile).dir().absolutePath();
        ui->statusBar->showMessage(tr("Preset file %1 loaded - %2").arg(QFileInfo(presetFile).fileName(), currentSet.name));
        if (!romFile.isEmpty())
            rebuildGTileView();
        return true;
    } else {
        qDebug() << "Error loading preset";
        return false;
    }
}

void MainUI::updateUiWithPreset()
{
    ui->headerRadioButton->setChecked(romHasHeader);
    ui->loRomRadioButton->setChecked(currentSet.romType == "LoROM");
    if (currentSet.SNESTilesLocation != 0)
    {
        ui->tilesSNESRadioButton->toggle();
        ui->snesAddrLineEdit->setText(QString::number(currentSet.SNESTilesLocation, 16));
    }
    else
    {
        ui->tilesPCRadioButton->toggle();
        ui->pcAddrLineEdit->setText(QString::number(currentSet.pcTilesLocation, 16));
    }
    ui->sizeLineEdit->setText(QString::number(currentSet.length));
    switch (currentSet.bpp)
    {
        case 1:
        {
            ui->bpp1RadioButton->setChecked(true);
            break;
        }
        case 2:
        {
            ui->bpp2RadioButton->setChecked(true);
            break;
        }
        case 3:
        {
            ui->bpp3RadioButton->setChecked(true);
            break;
        }
        case 4:
            ui->bpp4RadioButton->setChecked(true);
    }
    ui->tilePatternComboBox->setCurrentText(currentSet.tilesPattern.name);
    int compIdx = ui->compressionComboBox->findData(currentSet.compression);
    if (compIdx >= 0)
        ui->compressionComboBox->setCurrentIndex(compIdx);
    else
        ui->compressionComboBox->setCurrentText(currentSet.compression);
    if (currentSet.SNESPaletteLocation != 0)
    {
        ui->paletteSNESLocationRadioButton->toggle();
        ui->paletteSNESLocationLineEdit->setText(QString::number(currentSet.SNESPaletteLocation, 16));
    }
    if (currentSet.pcPaletteLocation != 0)
    {
        ui->palettePCLocationRadioButton->toggle();
        ui->palettePCLocationLineEdit->setText(QString::number(currentSet.pcPaletteLocation, 16));
    }
    ui->paletteNoZeroColorCheckBox->setChecked(currentSet.paletteNoZeroColor);
    if (currentSet.pcPaletteLocation == 0 && currentSet.SNESPaletteLocation == 0)
    {
        ui->palettePCLocationLineEdit->clear();
        ui->paletteSNESLocationLineEdit->clear();
        ui->paletteGrayRadioButton->toggle();
        ui->paletteNoZeroColorCheckBox->setEnabled(false);
    }
}

/* We use only thing that affect the set, rom header is not really important here for example */
/* Tile arrangement is not updated here */

void MainUI::updatePresetWithUi()
{
    currentSet.SNESTilesLocation = 0;
    currentSet.pcTilesLocation = -1;
    bool ok;
    if (ui->tilesSNESRadioButton->isChecked())
        currentSet.SNESTilesLocation = ui->snesAddrLineEdit->text().toUInt(&ok, 16);
    else
        currentSet.pcTilesLocation = ui->pcAddrLineEdit->text().toUInt(&ok, 16);
    currentSet.length = ui->sizeLineEdit->text().toUInt();
    if (ui->bpp1RadioButton->isChecked())
        currentSet.bpp = 1;
    if (ui->bpp2RadioButton->isChecked())
        currentSet.bpp = 2;
    if (ui->bpp3RadioButton->isChecked())
        currentSet.bpp = 3;
    if (ui->bpp4RadioButton->isChecked())
        currentSet.bpp = 4;

    currentSet.compression = ui->compressionComboBox->currentData().isValid() ?
        ui->compressionComboBox->currentData().toString() : ui->compressionComboBox->currentText();

    currentSet.SNESPaletteLocation = 0;
    currentSet.pcPaletteLocation = 0;
    if (ui->paletteSNESLocationRadioButton->isChecked())
        currentSet.SNESPaletteLocation = ui->paletteSNESLocationLineEdit->text().toUInt(&ok, 16);
    if (ui->palettePCLocationRadioButton->isChecked())
        currentSet.pcPaletteLocation = ui->palettePCLocationLineEdit->text().toUInt(&ok, 16);
    if (ui->paletteNoZeroColorCheckBox->isEnabled())
        currentSet.paletteNoZeroColor = ui->paletteNoZeroColorCheckBox->isChecked();
}

void MainUI::on_tilesPerRowSpinBox_valueChanged(int arg1)
{
    tilesPerRow = arg1;
}

void MainUI::on_tileZoomHorizontalSlider_valueChanged(int value)
{
    tileScene->setTilesZoom(value);
}


void MainUI::on_presetOpenPushButton_clicked()
{
    QString presetFile = QFileDialog::getOpenFileName(this,
                                tr("Select preset"), lastPresetDirectory, tr("stk (*.stk)"));
    if (!presetFile.isEmpty())
    {
        loadPreset(presetFile);
    }
}

void MainUI::on_presetSavePushButton_clicked()
{
    bool    ok;
    QString presetName;
    if (!currentSet.name.isEmpty())
        presetName = currentSet.name;
    else
        presetName = dataEngine.romInfo.romTitle;
    presetName = QInputDialog::getText(this, tr("Preset name"), tr("Preset name"), QLineEdit::Normal, presetName, &ok);
    if (ok) {
        QString presetFile = QFileDialog::getSaveFileName(this, tr("Preset file"), lastPresetDirectory + "/" + presetName + ".stk", tr("stk (*.stk)"));
        if (!presetFile.isEmpty()) {
            currentSet.name = presetName;
            savePresetTo(presetFile);
        }
    }
}

bool MainUI::savePresetTo(const QString &presetFile, QString *error)
{
    updatePresetWithUi();
    if (currentSet.name.isEmpty())
        currentSet.name = dataEngine.romInfo.romTitle;
    if (currentSet.save(presetFile))
    {
        lastPresetDirectory = QFileInfo(presetFile).dir().absolutePath();
        ui->statusBar->showMessage(tr("Preset file saved properly"));
        return true;
    }
    if (error)
        *error = tr("Error saving preset");
    ui->statusBar->showMessage(tr("Error saving preset"));
    qDebug() << "Error saving preset";
    return false;
}

bool MainUI::openPresetFrom(const QString &presetFile, QString *error)
{
    if (!loadPreset(presetFile))
    {
        if (error)
            *error = "Error loading preset: " + presetFile;
        return false;
    }
    return true;
}


void MainUI::on_pngExportPushButton_clicked()
{
    unsigned int piko = currentSet.pcTilesLocation == 0 ? currentSet.SNESTilesLocation : currentSet.pcTilesLocation;
    QString exportName = dataEngine.romInfo.romTitle + "_" + QString::number(piko, 16);
    if (!currentSet.name.isEmpty())
        exportName = currentSet.name;
    QString pngFile = QFileDialog::getSaveFileName(this, tr("Exported PNG File"), lastPNGDirectory + "/" + exportName + ".png", tr("PNG file, (*.png)"));
    if (!pngFile.isEmpty())
        exportPngTo(pngFile);
}

bool MainUI::exportPngTo(const QString &pngFile, QString *error)
{
    if (rawTiles.isEmpty())
    {
        if (error)
            *error = tr("No tiles extracted yet, use Refresh first");
        return false;
    }
    QImage img = mergeTilesToImage(rawTiles, mPalette, currentSet.tilesPattern);
    if (saveToPNG(img, pngFile))
    {
        lastPNGDirectory = QFileInfo(pngFile).dir().absolutePath();
        ui->statusBar->showMessage(tr("Export to %1 succesfull").arg(pngFile));
        return true;
    }
    ui->statusBar->showMessage(tr("Error while exporting to %1").arg(pngFile));
    if (error)
        *error = tr("Error while exporting to %1").arg(pngFile);
    return false;
}

void MainUI::on_pngImportpushButton_clicked()
{
    QString pngFile = QFileDialog::getOpenFileName(this,
                                tr("Select PNG image to inject"), lastPNGDirectory, tr("png (*.png)"));
    if (!pngFile.isEmpty())
    {
        InjectDialog iDiag(this, pngFile, mPalette);
        QFileInfo fi(romFile);
        iDiag.setCopyRomString(fi.baseName() + "_copy." + fi.completeSuffix());
        if (iDiag.exec())
            injectPngFrom(pngFile, iDiag.copyRom, iDiag.useImagePalette);
    }
}

QString MainUI::injectPngFrom(const QString &pngFile, bool workOnCopy, bool useImagePalette, QString *error)
{
    if (romFile.isEmpty())
    {
        if (error)
            *error = tr("No ROM loaded");
        return QString();
    }
    if (!QFileInfo::exists(pngFile))
    {
        if (error)
            *error = "File not found: " + pngFile;
        return QString();
    }
    updatePresetWithUi();
    QList<tile8>    importedRawTiles = tilesFromPNG(pngFile);
    qDebug("Imported %d tiles", importedRawTiles.size());
    QString info;
    if (workOnCopy)
    {
        QFileInfo fi(romFile);
        QString copy = fi.absolutePath() + "/" + fi.baseName() + "_copy." + fi.completeSuffix();
        if (QFileInfo::exists(copy))
        {
            if (!QFile::remove(copy))
            {
                qCritical() << "Can't remove rom copy " << copy;
                if (error)
                    *error = "Can't remove rom copy " + copy;
                return QString();
            }
        }
        if (QFile::copy(romFile, copy) == false)
        {
            qCritical() << "Can't create a copy of the rom file\n";
            if (error)
                *error = "Can't create a copy of the rom file";
            return QString();
        }
        qDebug() << "Set rom copy";
        dataEngine.setRomFile(copy);
    }
    importedRawTiles = TilesPattern::reverse(currentSet.tilesPattern, importedRawTiles);
    qDebug("Transformed to %d tiles", importedRawTiles.size());
    auto writtenBytes = dataEngine.injectTiles(importedRawTiles, currentSet);
    if (writtenBytes > currentSet.length)
    {
        QMessageBox::information(this, tr("Exessive write of data"),
        tr("The reinjected data exceed the lenght defined in the preset, It mean the new data inject take more space than the original data.\nThat can corrupt the data that come after, making your rom implayable"));
        info = "Warning: reinjected data exceed the preset length";
    }
    if ((currentSet.pcPaletteLocation != 0 || currentSet.SNESPaletteLocation != 0) && useImagePalette)
    {
        SNESPalette importedPalette = paletteFromPNG(pngFile);
        dataEngine.injectPalette(importedPalette, currentSet);
        mPalette = importedPalette;
    }
    if (workOnCopy)
    {
        dataEngine.setRomFile(romFile);
        ui->statusBar->showMessage(tr("Import successfull - Displaying tiles of the copy"));
    }
    else
    {
        ui->statusBar->showMessage(tr("Import successfull"));
    }
    rawTiles = importedRawTiles;
    buildTileScene();
    return info;
}

void MainUI::on_compressionComboBox_currentIndexChanged(int index)
{
    QVariant data = ui->compressionComboBox->itemData(index);
    QString compressionName = data.isValid() ? data.toString() : ui->compressionComboBox->itemText(index);
    if (compressionName == "None" || compressInfos[compressionName].canCompress)
        ui->pngImportpushButton->setEnabled(true);
    else
        ui->pngImportpushButton->setEnabled(false);

}

void MainUI::on_tilePatternComboBox_currentIndexChanged(int index)
{
    currentSet.tilesPattern = TilesPattern::pattern(ui->tilePatternComboBox->itemText(index));
    if (!rawTiles.isEmpty())
        tileScene->buildScene(rawTiles, mPalette, currentSet.tilesPattern);
}

void MainUI::on_bankComboBox_currentIndexChanged(int index)
{
    ui->pcAddrLineEdit->setText(QString::number(index * 0x10000, 16));
    ui->sizeLineEdit->setText(QString::number(0x10000));
}

void MainUI::on_netaButton_clicked()
{
    bool ok;
    auto value = ui->palettePCLocationLineEdit->text().toUInt(&ok, 16);
    ui->palettePCLocationLineEdit->setText(QString::number(++value, 16));
}

void MainUI::on_editPaletteButton_clicked()
{
    paletteEditor->show();
    paletteEditor->setPalette(mPalette);
    paletteEditor->setRomFile(romFile);
}

void MainUI::on_openStateButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
          tr("Select a SaveState file"), lastROMDirectory, tr("Savestate files;All files (*)"));
    if (!fileName.isNull())
        openSavestate(fileName);
}

void    MainUI::openSavestate(QString path)
{
    qDebug() << "Opening savestate";
    if (!snesSaveState.open(path))
    {
        ui->statusBar->showMessage(tr("Error loading the savestate"));
        ui->pickSavestatePaletteButton->setEnabled(false);
        return ;
    }
    savestateFile = path;
    ui->statusBar->showMessage(QString(tr("Savestate loaded : %1 version %2 - %3")).arg(snesSaveState.emulator, snesSaveState.version.toString(), snesSaveState.romName));
    savestatepaletteselector->setPalettesData(snesSaveState.CGDATA);
    ui->pickSavestatePaletteButton->setEnabled(true);
}

void MainUI::on_pickSavestatePaletteButton_clicked()
{
    savestatepaletteselector->show();
}

// ==================== Options / language / MCP server ====================

void MainUI::buildMenus()
{
    m_optionsMenu = menuBar()->addMenu(tr("Options"));
    m_languageMenu = m_optionsMenu->addMenu(tr("Language"));
    connect(m_languageMenu, &QMenu::aboutToShow, this, &MainUI::refreshLanguageMenu);
    m_actionMcp = m_optionsMenu->addAction(tr("MCP AI control server"));
    m_actionMcp->setCheckable(true);
    m_actionMcp->setToolTip(tr("Lets an AI control this tool through the MCP protocol (localhost TCP)"));
    m_actionMcp->setStatusTip(m_actionMcp->toolTip());
    connect(m_actionMcp, &QAction::toggled, this, &MainUI::toggleMcpServer);
    m_optionsMenu->addSeparator();
    m_actionMcpDocs = m_optionsMenu->addAction(tr("MCP documentation"));
    m_actionMcpDocs->setToolTip(tr("Shows how to connect an AI (OpenCode) to this tool and lists the available tools"));
    connect(m_actionMcpDocs, &QAction::triggered, this, &MainUI::openMcpDocs);
    refreshLanguageMenu();
}

void MainUI::openMcpDocs()
{
    if (m_mcpDocsDialog == nullptr)
    {
        m_mcpDocsDialog = new McpDocsDialog(this);
    }
    QString endpoint = (m_mcpServer && m_mcpServer->isRunning()) ? m_mcpServer->endpoint() : QString();
    m_mcpDocsDialog->setDocumentation(m_language, endpoint);
    m_mcpDocsDialog->show();
    m_mcpDocsDialog->raise();
    m_mcpDocsDialog->activateWindow();
}

// Rebuilt every time the menu opens, so new .txt files dropped in the
// translations directory show up without restarting.
void MainUI::refreshLanguageMenu()
{
    m_languageMenu->clear();
    QAction *en = m_languageMenu->addAction("English");
    en->setCheckable(true);
    en->setChecked(m_language == "en");
    connect(en, &QAction::triggered, this, [this]() { applyLanguage("en"); });
    m_languageMenu->addSeparator();
    for (const LanguageInfo &li : availableLanguages(m_translationsDir))
    {
        QAction *a = m_languageMenu->addAction(li.name);
        a->setCheckable(true);
        a->setChecked(m_language == li.id);
        connect(a, &QAction::triggered, this, [this, li]() { applyLanguage(li.id); });
    }
}

QString MainUI::defaultLanguage() const
{
    if (QLocale::system().name().startsWith("pt"))
    {
        for (const LanguageInfo &li : availableLanguages(m_translationsDir))
        {
            if (li.locale.startsWith("pt"))
                return li.id;
        }
    }
    return "en";
}

QStringList MainUI::languageList() const
{
    QStringList ids;
    ids << "en";
    for (const LanguageInfo &li : availableLanguages(m_translationsDir))
        ids << li.id;
    return ids;
}

void MainUI::applyLanguage(const QString &lang)
{
    m_language = lang;
    m_settings->setValue("language", lang);
    if (m_txtTranslator)
    {
        QCoreApplication::removeTranslator(m_txtTranslator);
        delete m_txtTranslator;
        m_txtTranslator = nullptr;
    }
    if (m_qtTranslator)
    {
        QCoreApplication::removeTranslator(m_qtTranslator);
        delete m_qtTranslator;
        m_qtTranslator = nullptr;
    }
    if (lang != "en")
    {
        for (const LanguageInfo &li : availableLanguages(m_translationsDir))
        {
            if (li.id == lang)
            {
                m_txtTranslator = new TxtTranslator(li.path);
                QCoreApplication::installTranslator(m_txtTranslator);
                m_qtTranslator = loadQtTranslatorFor(li.locale);
                if (m_qtTranslator)
                    QCoreApplication::installTranslator(m_qtTranslator);
                break;
            }
        }
    }
    retranslateAll();
}

void MainUI::retranslateAll()
{
    ui->retranslateUi(this);
    if (m_optionsMenu)
    {
        m_optionsMenu->setTitle(tr("Options"));
        m_languageMenu->setTitle(tr("Language"));
        m_actionMcp->setText(tr("MCP AI control server"));
        m_actionMcp->setToolTip(tr("Lets an AI control this tool through the MCP protocol (localhost TCP)"));
        m_actionMcpDocs->setText(tr("MCP documentation"));
        if (m_mcpDocsDialog != nullptr)
            m_mcpDocsDialog->setDocumentation(m_language,
                (m_mcpServer && m_mcpServer->isRunning()) ? m_mcpServer->endpoint() : QString());
    }
    if (ui->compressionComboBox->count() > 0)
    {
        ui->compressionComboBox->setItemText(0, tr("None"));
        ui->compressionComboBox->setItemData(0, tr("No compression"), Qt::ToolTipRole);
    }
    paletteEditor->retranslate();
    savestatepaletteselector->retranslate();
    ui->tileInfos->retranslate();
}

void MainUI::toggleMcpServer(bool enabled)
{
    quint16 port = m_settings->value("mcpPort", 8765).toUInt();
    if (enabled)
    {
        if (m_mcpServer->start(port))
        {
            m_settings->setValue("mcpEnabled", true);
            ui->statusBar->showMessage(tr("MCP server started on %1").arg(m_mcpServer->endpoint()));
        }
        else
        {
            m_settings->setValue("mcpEnabled", false);
            m_actionMcp->blockSignals(true);
            m_actionMcp->setChecked(false);
            m_actionMcp->blockSignals(false);
            ui->statusBar->showMessage(tr("MCP server error : %1").arg("listen failed"));
        }
    }
    else
    {
        m_mcpServer->stop();
        m_settings->setValue("mcpEnabled", false);
        ui->statusBar->showMessage(tr("MCP server stopped"));
    }
}

// ==================== Programmatic control API (MCP server) ====================

static QString normalizeHex(QString value)
{
    value = value.trimmed();
    if (value.startsWith("0x", Qt::CaseInsensitive))
        value.remove(0, 2);
    while (value.startsWith("$"))
        value.remove(0, 1);
    return value;
}

void MainUI::refreshView()
{
    updatePresetWithUi();
    rebuildGTileView();
}

QString MainUI::setTileSettings(const QVariantMap &cfg)
{
    QString hex = normalizeHex(cfg.value("pc_hex").toString());
    if (!hex.isEmpty())
    {
        ui->tilesPCRadioButton->setChecked(true);
        ui->pcAddrLineEdit->setText(hex);
    }
    hex = normalizeHex(cfg.value("snes_hex").toString());
    if (!hex.isEmpty())
    {
        ui->tilesSNESRadioButton->setChecked(true);
        ui->snesAddrLineEdit->setText(hex);
    }
    if (cfg.contains("size"))
        ui->sizeLineEdit->setText(QString::number(cfg.value("size").toUInt()));
    if (cfg.contains("bpp"))
    {
        int bpp = cfg.value("bpp").toInt();
        if (bpp < 1 || bpp > 4)
            return "bpp must be 1, 2, 3 or 4";
        switch (bpp)
        {
            case 1: ui->bpp1RadioButton->setChecked(true); break;
            case 2: ui->bpp2RadioButton->setChecked(true); break;
            case 3: ui->bpp3RadioButton->setChecked(true); break;
            case 4: ui->bpp4RadioButton->setChecked(true); break;
        }
    }
    if (cfg.contains("compression"))
    {
        QString name = cfg.value("compression").toString();
        int idx = ui->compressionComboBox->findData(name);
        if (idx < 0)
            return "Unknown compression: " + name;
        ui->compressionComboBox->setCurrentIndex(idx);
    }
    return QString();
}

QString MainUI::setPaletteSettings(const QVariantMap &cfg)
{
    QString hex = normalizeHex(cfg.value("pc_hex").toString());
    if (!hex.isEmpty())
    {
        ui->palettePCLocationRadioButton->setChecked(true);
        ui->palettePCLocationLineEdit->setText(hex);
    }
    hex = normalizeHex(cfg.value("snes_hex").toString());
    if (!hex.isEmpty())
    {
        ui->paletteSNESLocationRadioButton->setChecked(true);
        ui->paletteSNESLocationLineEdit->setText(hex);
    }
    if (cfg.contains("no_zero_color"))
        ui->paletteNoZeroColorCheckBox->setChecked(cfg.value("no_zero_color").toBool());
    if (cfg.value("gray").toBool())
        ui->paletteGrayRadioButton->setChecked(true);
    if (cfg.contains("gray_size"))
    {
        setGrayscalePalette(cfg.value("gray_size").toUInt());
        buildPaletteScene();
    }
    return QString();
}

QString MainUI::setPattern(const QString &name)
{
    int idx = ui->tilePatternComboBox->findText(name);
    if (idx < 0)
        return "Unknown pattern: " + name;
    ui->tilePatternComboBox->setCurrentIndex(idx);
    return QString();
}

void MainUI::setZoom(int value)
{
    value = qBound(ui->tileZoomHorizontalSlider->minimum(), value, ui->tileZoomHorizontalSlider->maximum());
    ui->tileZoomHorizontalSlider->setValue(value);
}

bool MainUI::saveWindowScreenshot(const QString &pngFile, QString *error)
{
    QImage img = grab().toImage();
    if (img.isNull() || !img.save(pngFile))
    {
        if (error)
            *error = "Failed to save screenshot to " + pngFile;
        return false;
    }
    ui->statusBar->showMessage(tr("Export to %1 succesfull").arg(pngFile));
    return true;
}

bool MainUI::openSavestateFile(const QString &path, QString *error)
{
    openSavestate(path);
    if (snesSaveState.isEmpty())
    {
        if (error)
            *error = tr("Error loading the savestate");
        return false;
    }
    return true;
}

bool MainUI::selectSavestatePalette(int index, QString *error)
{
    if (snesSaveState.isEmpty())
    {
        if (error)
            *error = "No savestate opened";
        return false;
    }
    if (!savestatepaletteselector->selectPalette(index))
    {
        if (error)
            *error = QString("Invalid palette index %1").arg(index);
        return false;
    }
    return true;
}

QVariantMap MainUI::collectState() const
{
    QVariantMap st;
    st["rom_loaded"] = !romFile.isEmpty();
    st["rom_file"] = romFile;
    st["rom_title"] = dataEngine.romInfo.romTitle;
    st["rom_type"] = currentSet.romType;
    st["rom_has_header"] = romHasHeader;
    st["tiles_source"] = ui->tilesSNESRadioButton->isChecked() ? "snes" : "pc";
    st["tiles_snes_address_hex"] = ui->snesAddrLineEdit->text();
    st["tiles_pc_address_hex"] = ui->pcAddrLineEdit->text();
    st["size"] = ui->sizeLineEdit->text().toUInt();
    if (ui->bpp1RadioButton->isChecked()) st["bpp"] = 1;
    if (ui->bpp2RadioButton->isChecked()) st["bpp"] = 2;
    if (ui->bpp3RadioButton->isChecked()) st["bpp"] = 3;
    if (ui->bpp4RadioButton->isChecked()) st["bpp"] = 4;
    st["compression"] = ui->compressionComboBox->currentData().isValid() ?
        ui->compressionComboBox->currentData().toString() : ui->compressionComboBox->currentText();
    st["palette_source"] = ui->paletteGrayRadioButton->isChecked() ? "gray" :
        (ui->paletteSNESLocationRadioButton->isChecked() ? "snes" : "pc");
    st["palette_snes_address_hex"] = ui->paletteSNESLocationLineEdit->text();
    st["palette_pc_address_hex"] = ui->palettePCLocationLineEdit->text();
    st["palette_no_zero_color"] = ui->paletteNoZeroColorCheckBox->isChecked();
    st["tiles_pattern"] = ui->tilePatternComboBox->currentText();
    st["zoom"] = ui->tileZoomHorizontalSlider->value();
    st["tiles_extracted"] = rawTiles.size();
    st["palette_colors"] = mPalette.colors.size();
    st["preset_name"] = currentSet.name;
    st["savestate_file"] = savestateFile;
    st["language"] = m_language;
    st["languages"] = languageList();
    st["compressions"] = compressInfos.keys();
    st["patterns"] = TilesPattern::Patterns().keys();
    st["mcp_server"] = (m_mcpServer && m_mcpServer->isRunning()) ? m_mcpServer->endpoint() : QString();
    return st;
}
