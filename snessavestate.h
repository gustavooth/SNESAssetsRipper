#pragma once
#include <QByteArray>
#include <QString>
#include <QVersionNumber>

class SNESSavestate
{
public:
    SNESSavestate();

    QString             romName;
    QString             emulator;
    QVersionNumber      version;

    QByteArray  WRAM;
    QByteArray  SRAM;
    QByteArray  ROM;
    QByteArray  CGDATA;

    bool                open(const QString path);
    bool                isEmpty() const;
    void                clear();
private:
    bool                m_empty;
};

