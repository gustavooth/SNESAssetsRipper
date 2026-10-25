#ifndef MCPDOCSDIALOG_H
#define MCPDOCSDIALOG_H

#include <QDialog>

class QTextEdit;

// In-program MCP documentation window: explains how to connect an AI client
// (OpenCode) to the tool, lists the available tools and ships a "copy"
// button so the registration command / full doc can be pasted anywhere.
class McpDocsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit McpDocsDialog(QWidget *parent = nullptr);

    // Rebuilds the text (called again when the language changes)
    void    setDocumentation(const QString &language, const QString &tcpEndpoint);

private slots:
    void    copyDocumentation();

private:
    QTextEdit   *m_text;
    QString     m_language;
    QString     m_tcpEndpoint;
};

#endif // MCPDOCSDIALOG_H