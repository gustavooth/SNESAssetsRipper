#include "txttranslator.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QRegularExpression>
#include <QTextStream>

static QString unescape(QString str)
{
    str.replace("\\n", "\n");
    str.replace("\\t", "\t");
    return str;
}

// Parse a translation .txt file. Returns false when the file can't be read.
static bool parseTranslationFile(const QString &path, LanguageInfo &info,
                                 QHash<QPair<QString, QString>, QString> *table)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    info.path = path;
    info.id = QFileInfo(path).completeBaseName();
    info.name = info.id;
    info.locale = info.id;

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QString context;
    while (!in.atEnd())
    {
        QString line = in.readLine();
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty())
            continue;

        if (trimmed.startsWith("#"))
        {
            // header metadata: "# name=..." / "# locale=..."
            if (trimmed.startsWith("# name=", Qt::CaseInsensitive))
                info.name = unescape(trimmed.mid(7).trimmed());
            else if (trimmed.startsWith("# locale=", Qt::CaseInsensitive))
                info.locale = unescape(trimmed.mid(9).trimmed());
            continue;
        }

        if (trimmed.startsWith("[") && trimmed.endsWith("]"))
        {
            context = trimmed.mid(1, trimmed.size() - 2);
            continue;
        }

        if (context.isEmpty() || !trimmed.contains('='))
            continue;

        int eq = trimmed.indexOf('=');
        QString source = unescape(trimmed.left(eq).trimmed());
        QString translated = unescape(trimmed.mid(eq + 1).trimmed());
        if (source.isEmpty() || table == nullptr)
            continue;
        table->insert(QPair<QString, QString>(context, source), translated);
    }
    return true;
}

QString translationsDirectory()
{
    QDir appDir(QCoreApplication::applicationDirPath());
    if (appDir.cd("translations") && !appDir.entryList({"*.txt"}, QDir::Files).isEmpty())
        return appDir.absolutePath();
    QDir parent = appDir;
    if (parent.cdUp() && parent.cd("translations") && !parent.entryList({"*.txt"}, QDir::Files).isEmpty())
        return parent.absolutePath();
    return QString();
}

QList<LanguageInfo> availableLanguages(const QString &directory)
{
    QList<LanguageInfo> langs;
    QDir dir(directory);
    for (const QString &fileName : dir.entryList({"*.txt"}, QDir::Files, QDir::Name))
    {
        if (fileName.compare("readme.txt", Qt::CaseInsensitive) == 0)
            continue;   // documentation file, not a language
        LanguageInfo info;
        if (parseTranslationFile(dir.absoluteFilePath(fileName), info, nullptr))
            langs.append(info);
    }
    return langs;
}

QTranslator *loadQtTranslatorFor(const QString &locale)
{
    if (locale.isEmpty() || locale == "en")
        return nullptr;
    const QString dir = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
    QTranslator *translator = new QTranslator(QCoreApplication::instance());
    if (translator->load("qtbase_" + locale, dir) || translator->load("qt_" + locale, dir))
        return translator;
    delete translator;
    return nullptr;
}

TxtTranslator::TxtTranslator(const QString &filePath)
{
    parseTranslationFile(filePath, m_info, &m_table);
}

QString TxtTranslator::translate(const char *context, const char *sourceText,
                                 const char *disambiguation, int n) const
{
    Q_UNUSED(disambiguation);
    Q_UNUSED(n);
    return m_table.value(QPair<QString, QString>(QString::fromLatin1(context),
                                                 QString::fromUtf8(sourceText)));
}