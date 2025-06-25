#-------------------------------------------------
#
# Project created by QtCreator 2017-03-02T13:33:21
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SNESTilesKitten
TEMPLATE = app
QMAKE_CFLAGS += "-std=c99"

RC_FILE = snestileskitten.rc

INCLUDEPATH += "lowlevelstuff/src/"
INCLUDEPATH += "lowlevelstuff/src/savestate"
INCLUDEPATH += "lowlevelstuff/src/puff/"

SOURCES += main.cpp\
        mainui.cpp \
    tilepreset.cpp \
    lowlevelstuff/src/tile.c \
    lowlevelstuff/src/mapping_hirom.c \
    lowlevelstuff/src/mapping_lorom.c \
    lowlevelstuff/src/rommapping.c \
    lowlevelstuff/src/palette.c \
    lowlevelstuff/src/rominfo.c \
    graphicspalettecoloritem.cpp \
    presetsavedialog.cpp \
    pngexportimport.cpp \
    romdataengine.cpp \
    graphicstileitem.cpp \
    injectdialog.cpp \
    graphicstilesscene.cpp \
    tilespattern.cpp \
    tileinfow.cpp \
    mgraphicsview.cpp \
    myrominfo.cpp \
    paletteeditor.cpp \
    snespalette.cpp \
    emupaletteimporter.cpp \
    lowlevelstuff/src/savestateloader.c \
    lowlevelstuff/src/savestate/savestate.c \
    lowlevelstuff/src/puff/puff.c \
    lowlevelstuff/src/savestate/snes9x_152.c \
    savestatepaletteselector.cpp \
    graphicsclickableview.cpp \
    snessavestate.cpp

HEADERS  += mainui.h \
    tilepreset.h \
    lowlevelstuff/src/tile.h \
    lowlevelstuff/src/rommapping.h \
    lowlevelstuff/src/rominfo.h \
    compressioninterface.h \
    compressioninfo.h \
    lowlevelstuff/src/palette.h \
    graphicspalettecoloritem.h \
    presetsavedialog.h \
    pngexportimport.h \
    romdataengine.h \
    graphicstileitem.h \
    injectdialog.h \
    tilespattern.h \
    tileinfow.h \
    mgraphicsview.h \
    myrominfo.h \
    paletteeditor.h \
    snespalette.h \
    emupaletteimporter.h \
    lowlevelstuff/src/savestateloader.h \
    lowlevelstuff/src/savestate/_savestate.h \
    lowlevelstuff/src/savestate/snes9x_152.h \
    lowlevelstuff/src/puff/puff.h \
    savestatepaletteselector.h \
    graphicsclickableview.h \
    snessavestate.h

FORMS    += mainui.ui \
    presetsavedialog.ui \
    injectdialog.ui \
    tileinfow.ui \
    paletteeditor.ui \
    savestatepaletteselector.ui

DISTFILES += \
    snestileskitten.rc
