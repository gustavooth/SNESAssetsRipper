#ifndef SBM5COMPRESSION_H
#define SBM5COMPRESSION_H

#include <QObject>
#include <../../compressioninterface.h>

class SBM5Compression : public QObject, public CompressionInterface
{
    Q_OBJECT
    Q_INTERFACES(CompressionInterface)
    Q_PLUGIN_METADATA(IID "fr.nyo.SNESTilesRipper.SBM5Compression" FILE "sbm5.json")

public:
    SBM5Compression();
    QList<CompressionInfo>  compressionList();
    char*   compress(QString name, const char *data, const size_t start, const unsigned int lenght, unsigned int *compressedSize);
    char*   unCompress(QString name, const char *data, const size_t start, const unsigned int lenght, unsigned int *uncompressedSize, unsigned int *compressedLength);
    QString compressionError();
    QString decompressionError();

};
#endif // SBM5COMPRESSION_H
