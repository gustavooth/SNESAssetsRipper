#ifndef MCPSERVER_H
#define MCPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QIODevice>

class QFile;
class QSocketNotifier;
class MainUI;

// Minimal MCP (Model Context Protocol) server.
//
// Transports, all speaking JSON-RPC 2.0 with MCP tools:
// - Streamable HTTP server on http://127.0.0.1:<port>/mcp (GUI toggle in the
//   Options menu): MCP clients connect straight to the running instance.
// - stdio mode: the binary is started with --mcp-stdio by an MCP client
//   (ex: `opencode mcp add ... -- SNESTilesRipper --mcp-stdio`); no GUI is
//   shown and stdin/stdout carry the protocol.
//
// Tools map to actions a user could do in the UI, so an AI client can fully
// drive the tool (load ROM, set locations, refresh, export/inject PNG,
// savestate palettes, presets, language...).
class McpServer : public QObject
{
    Q_OBJECT
public:
    explicit McpServer(QObject *parent = nullptr);

    void    setTarget(MainUI *main);
    bool    start(quint16 port);
    bool    startStdio();
    void    stop();
    bool    isRunning() const;
    bool    isStdio() const;
    QString endpoint() const;

signals:
    void    runningChanged(bool running);
    void    logMessage(const QString &msg);
    void    stdioClosed();

private slots:
    void    onNewConnection();
    void    onClientData();
    void    onClientDisconnected();
    void    onStdioData();

private:
    // JSON-RPC dispatch shared by every transport: answers requests, ignores
    // notifications (returns empty QByteArray for them).
    QByteArray  dispatchMessage(const QJsonObject &msg);
    QJsonObject makeResult(const QJsonValue &id, const QJsonObject &result);
    QJsonObject makeError(const QJsonValue &id, int code, const QString &message);

    // HTTP transport (Streamable HTTP, stateless)
    void    handleHttpRequest(QTcpSocket *socket);
    void    sendHttpResponse(QTcpSocket *socket, int status, const QString &contentType, const QByteArray &body);

    QJsonArray  buildToolsList() const;
    QJsonObject toolSchema(const QJsonObject &properties, const QStringList &required = QStringList()) const;
    QJsonObject textContent(const QString &text, bool isError = false) const;
    QJsonObject handleToolCall(const QString &name, const QJsonObject &args, bool &isError);

    QTcpServer              *m_server = nullptr;
    QList<QTcpSocket*>      m_clients;
    QHash<QTcpSocket*, QByteArray>  m_buffers;
    MainUI                  *m_main = nullptr;
    quint16                 m_port = 0;

    // stdio transport
    QFile                   *m_stdioIn = nullptr;
    QFile                   *m_stdioOut = nullptr;
    QSocketNotifier         *m_stdioNotifier = nullptr;
    QByteArray              m_stdioBuffer;
};

#endif // MCPSERVER_H