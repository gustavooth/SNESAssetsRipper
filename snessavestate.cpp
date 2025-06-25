#include "snessavestate.h"
#include "savestateloader.h"
#include <QDebug>

#include <QFile>

SNESSavestate::SNESSavestate()
{
    m_empty = true;
}

bool SNESSavestate::open(const QString path)
{
    if (!QFile::exists(path))
        return false;
    savestate_t msavestate;
    if (savestate_loader_init(path.toLatin1().constData(), NULL, 0, &msavestate) == false)
    {
        fprintf(stderr, "Can't init the file\n");
        return false;
    }
    emulator = msavestate.header.emulator_name;
    version = QVersionNumber::fromString(QString(msavestate.header.emulator_version));
    romName = msavestate.header.rom_file;
    qDebug() << msavestate.header.rom_file;
    qDebug() << msavestate.header.emulator_name;
    savestate_section_t* cgram_section = savestate_loader_get(CGRAM, &msavestate);
    qDebug() << (size_t)cgram_section;
    CGDATA = QByteArray::fromRawData(cgram_section->data, cgram_section->size);
    m_empty = false;
    //savestate_loader_clear(msavestate);
    return true;
}

bool SNESSavestate::isEmpty() const
{
    return m_empty;
}

void SNESSavestate::clear()
{
    m_empty = true;
    emulator.clear();
    romName.clear();
    CGDATA.clear();
}


