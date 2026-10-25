#include "tilespattern.h"

#include <QSettings>
#include <QDebug>
#include <QRegularExpression>

QMap<QString, TilesPattern> TilesPattern::m_Patterns;

TilesPattern::TilesPattern()
{
    tilesPerRow = 16;
    numberOfTiles = 16;
}

bool TilesPattern::load(QString patternFile)
{
    QSettings   pFile(patternFile, QSettings::IniFormat);
    name = pFile.value("pattern/name").toString();
    description = pFile.value("pattern/description").toString();
    numberOfTiles = pFile.value("pattern/number_of_tile").toInt();
    qDebug() << name;
    //unsigned int nbOfTile = pFile.value("_/number_of_tile").toUInt();
    QString patternString = pFile.value("pattern/pattern").toString();
    qDebug() << patternString;

    if (name.isEmpty() || patternString.isEmpty())
        return false;   // Not a valid TilesPattern file

    // Pattern String is a array description

    transformVector.clear();
    QRegularExpression arrayRegExp("(\\[[\\s|0-F|a-f|,]+\\])");
    int pos = 0;
    QRegularExpressionMatch arrayMatch = arrayRegExp.match(patternString, pos);
    while (arrayMatch.hasMatch())
    {
        QString arrayString = arrayMatch.captured(1);
        QVector<int> tmpVect;
        //qDebug() << arrayString;
        unsigned int stringPos = 1;

        while(arrayString[stringPos] != ']')
        {
            while (arrayString[stringPos].isSpace())
                stringPos++;
            QRegularExpression hex("([0-F|a-f]+)");
            bool    ok;
            QRegularExpressionMatch hexMatch = hex.match(arrayString, stringPos);
            if (hexMatch.capturedStart(1) == (int)stringPos)
            {
                tmpVect.append(hexMatch.captured(1).toInt(&ok, 16));
            }
            while (arrayString[stringPos].isSpace())
                stringPos++;
            stringPos++; // should be the comma
        }
        pos += arrayMatch.capturedLength();
        transformVector.append(tmpVect);
        arrayMatch = arrayRegExp.match(patternString, pos);
    }
    qDebug() << transformVector.size() << transformVector;
    return true;
}

bool TilesPattern::loadPatterns(const QDir& patternDirectory)
{
    foreach (QString fileName, patternDirectory.entryList(QDir::Files))
    {
        TilesPattern tp;
        qDebug() << "Loading " << fileName;
        if (!tp.load(patternDirectory.absoluteFilePath(fileName)))
        {
            qWarning() << "Skipping" << fileName << ": not a valid TilesPattern file";
            continue;
        }
        m_Patterns[tp.name] = tp;
    }
    return true;
}

TilesPattern TilesPattern::pattern(QString name)
{
    return m_Patterns[name];
}

QMap<QString, TilesPattern> TilesPattern::Patterns()
{
    return m_Patterns;
}

QVector<QVector<tile8> > TilesPattern::transform(const QList<tile8>& tiles) const
{
    QVector<QVector<int> > tVect = transformVector;
    if (tVect.isEmpty())
    {
        qWarning() << "TilesPattern" << name << "has no transform data, falling back to normal pattern";
        QList<int> identity;
        for (int i = 0; i < 16; i++)
            identity.append(i);
        tVect.append(identity);
    }
    unsigned int repeatOffsetY = 0;
    unsigned int repeatOffsetX = 0;
    unsigned int tVectHeight = tVect.size();
    unsigned int tVectWidth = tVect[0].size();
    unsigned int repeat = 0;
    QVector<QVector<tile8> > toret;
    unsigned int transPerRow = tilesPerRow / tVectWidth;
    unsigned int nbTransform = tiles.size() / numberOfTiles;
    qDebug("Tiles size : %d - nbtransform : %d - pattern number of tiles : %d", tiles.size(), nbTransform, numberOfTiles);
    if (transPerRow > nbTransform)
        toret.resize(tVectHeight);
    else
        toret.resize(((unsigned int)(((double)nbTransform / (double)transPerRow) + 0.5)) * tVectHeight);
    QMutableVectorIterator<QVector<tile8> > it(toret);
    while (it.hasNext())
    {
        it.next().resize(tilesPerRow);
    }
    //qDebug() << toret[0].size() << "x" << toret.size();
    while (repeat != nbTransform)
    {
        //qDebug() << "repeat" << repeat;
        for (unsigned int j = 0; j < tVectHeight; j++)
        {
            for (unsigned int i = 0; i < tVectWidth; i++)
            {
                unsigned int posTile = tVect[j][i] + numberOfTiles * repeat;
                unsigned int posX = i + repeatOffsetX;
                unsigned int posY = j + repeatOffsetY;
                //qDebug("X: %d - Y: %d - posTile : %d", posX, posY, posTile);
                toret[posY][posX] = tiles[posTile];
            }
        }
        if (repeatOffsetX + tVectWidth == tilesPerRow)
        {
            repeatOffsetX = 0;
            repeatOffsetY += tVectHeight;
        }
        else
            repeatOffsetX += tVectWidth;
        repeat++;
    }
    qDebug() << "End of transform";
    return toret;
}

QList<tile8> TilesPattern::reverse(const QList<tile8> &tiles) const
{
    QVector<QVector<int> > tVect = transformVector;
    if (tVect.isEmpty())
    {
        qWarning() << "TilesPattern" << name << "has no transform data, falling back to normal pattern";
        QList<int> identity;
        for (int i = 0; i < 16; i++)
            identity.append(i);
        tVect.append(identity);
    }
    unsigned int repeatOffsetY = 0;
    unsigned int repeatOffsetX = 0;
    unsigned int tVectHeight = tVect.size();
    unsigned int tVectWidth = tVect[0].size();
    unsigned int repeat = 0;
    unsigned int nbTransPerRow = tilesPerRow / tVectWidth;
    unsigned int nbTiles = tiles.size();
    QVector<tile8>    toretVec(tiles.size());

    for (unsigned int i = 0; i < nbTiles; i++)
    {
        unsigned int lineNb = i / tilesPerRow;
        unsigned int lineInTab = lineNb % tVectHeight;
        unsigned int colInTab = i % tVectWidth;
        unsigned int tileNb = tVect[lineInTab][colInTab];

        unsigned int lineBlock = i / (nbTransPerRow * numberOfTiles);
        unsigned int blockNB = (i % (nbTransPerRow * numberOfTiles) % tilesPerRow) / tVectWidth;

        //qDebug() << colInTab << lineInTab << " = " << tileNb;
        //unsigned int pos = tileNb + ((i % tilesPerRow) / nbTransPerRow) * numberOfTiles;
        unsigned int pos = tileNb + (lineBlock + blockNB) * numberOfTiles;
        //qDebug() << i << "Goes to : " << pos;
        toretVec[pos] = tiles[i];
    }
    return toretVec;
}

QVector<QVector<tile8> > TilesPattern::transform(const TilesPattern &pattern, const QList<tile8>& tiles)
{
    return pattern.transform(tiles);
}

QVector<QVector<tile8> > TilesPattern::transform(const QString id, const QList<tile8>& tiles)
{
    return m_Patterns[id].transform(tiles);
}

QList<tile8> TilesPattern::reverse(const TilesPattern &pattern, const QList<tile8> &tiles)
{
    return pattern.reverse(tiles);
}

