#include "sbm5compression.h"
#include "bm5rle.h"

#include <QDebug>

SBM5Compression::SBM5Compression()
{
}

QList<CompressionInfo> SBM5Compression::compressionList()
{
    QList<CompressionInfo> toret;
    CompressionInfo sbm5rle;
    sbm5rle.name = "SBM5RLE";
    sbm5rle.canCompress = true;
    sbm5rle.shortDescription = "Super Bomberman 5 (Hudson) RLE";
    sbm5rle.description =
        "RLE/back-reference compression used by Super Bomberman 5 (Hudson Soft). "
        "The game's pointer table (HiROM PC 0x05E22C) lists every block with its "
        "type, decompressed size and address; types 2-5 use this scheme. "
        "Set the location size to the compressed block's length, not the "
        "decompressed size — the stream carries no length of its own.";
    toret << sbm5rle;
    return toret;
}

QString SBM5Compression::compressionError()
{
    return QString();
}

QString SBM5Compression::decompressionError()
{
    return QString();
}

char *SBM5Compression::compress(QString name, const char *data, const size_t start, const unsigned int lenght, unsigned int *compressedSize)
{
    if (name != "SBM5RLE")
        return nullptr;
    return bm5rle::compress(data + start, lenght, compressedSize);
}

char *SBM5Compression::unCompress(QString name, const char *data, const size_t start, const unsigned int lenght, unsigned int *uncompressedSize, unsigned int *compressedLength)
{
    if (name != "SBM5RLE")
        return nullptr;
    qDebug() << "SBM5RLE: decoding" << lenght << "bytes at offset" << start;
    return bm5rle::decompress(data + start, lenght, uncompressedSize, compressedLength);
}
