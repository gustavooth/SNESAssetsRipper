#ifndef TXTTRANSLATOR_H
#define TXTTRANSLATOR_H

#include <QTranslator>
#include <QHash>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QVector>

// One available UI language, backed by a translation .txt file.
struct LanguageInfo
{
    QString id;      // file stem, ex: "pt_BR" for pt_BR.txt
    QString name;    // display name from the "# name=" header (or the id)
    QString locale;  // "# locale=" header (used for Qt builtin strings)
    QString path;    // full path of the .txt file
};

// Resolve the translations directory: <appdir>/translations, falling back to
// <appdir>/../translations (in-source-tree builds). Empty if not found.
QString translationsDirectory();

// List the available languages: every *.txt file in the directory.
QList<LanguageInfo> availableLanguages(const QString &directory);

// Best-effort load of Qt's own strings for a locale (QFileDialog, etc).
// Returns nullptr when no translation is shipped for it.
QTranslator *loadQtTranslatorFor(const QString &locale);

// A QTranslator that reads a simple, human-editable text file:
//
//   # name=Português (Brasil)        <- display name shown in the menu
//   # locale=pt_BR                   <- used for Qt builtin translations
//
//   [MainUI]                         <- context = form class name
//   Open Preset = Abrir preset       <- source = translation
//
// - One "source = translation" pair per line, split on the FIRST '='.
// - Lines starting with '#' are comments (or headers).
// - "\n" and "\t" escapes are decoded in both source and translation.
class TxtTranslator : public QTranslator
{
public:
    explicit TxtTranslator(const QString &filePath);

    QString translate(const char *context, const char *sourceText,
                      const char *disambiguation = nullptr, int n = -1) const override;

    const LanguageInfo &info() const { return m_info; }

private:
    LanguageInfo    m_info;
    QHash<QPair<QString, QString>, QString> m_table;
};

#endif // TXTTRANSLATOR_H