#include "mcpserver.h"
#include "mainui.h"

#include <QJsonDocument>
#include <QJsonValue>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QSocketNotifier>
#include <climits>

static const char *MCP_PROTOCOL_VERSION = "2024-11-05";

McpServer::McpServer(QObject *parent) :
    QObject(parent)
{
}

void McpServer::setTarget(MainUI *main)
{
    m_main = main;
}

bool McpServer::isRunning() const
{
    if (m_server != nullptr && m_server->isListening())
        return true;
    return m_stdioNotifier != nullptr;
}

bool McpServer::isStdio() const
{
    return m_stdioNotifier != nullptr;
}

QString McpServer::endpoint() const
{
    return QString("http://127.0.0.1:%1/mcp").arg(m_port);
}

bool McpServer::start(quint16 port)
{
    stop();
    m_server = new QTcpServer(this);
    QObject::connect(m_server, &QTcpServer::newConnection, this, &McpServer::onNewConnection);
    if (!m_server->listen(QHostAddress::LocalHost, port)) {
        emit logMessage(m_server->errorString());
        delete m_server;
        m_server = nullptr;
        return false;
    }
    m_port = m_server->serverPort();
    emit runningChanged(true);
    return true;
}

void McpServer::stop()
{
    if (m_stdioNotifier != nullptr)
    {
        m_stdioNotifier->setEnabled(false);
        delete m_stdioNotifier;
        m_stdioNotifier = nullptr;
        delete m_stdioIn;
        m_stdioIn = nullptr;
        delete m_stdioOut;
        m_stdioOut = nullptr;
        m_stdioBuffer.clear();
        emit runningChanged(false);
        return;
    }
    if (m_server == nullptr)
        return;
    m_server->close();
    for (QTcpSocket *client : m_clients) {
        client->disconnectFromHost();
        client->deleteLater();
    }
    m_clients.clear();
    m_buffers.clear();
    delete m_server;
    m_server = nullptr;
    m_port = 0;
    emit runningChanged(false);
}

bool McpServer::startStdio()
{
    stop();
    m_stdioIn = new QFile(this);
    if (!m_stdioIn->open(0, QIODevice::ReadOnly | QIODevice::Unbuffered))
    {
        delete m_stdioIn;
        m_stdioIn = nullptr;
        return false;
    }
    m_stdioOut = new QFile(this);
    if (!m_stdioOut->open(1, QIODevice::WriteOnly | QIODevice::Unbuffered))
    {
        delete m_stdioOut;
        m_stdioOut = nullptr;
        delete m_stdioIn;
        m_stdioIn = nullptr;
        return false;
    }
    m_stdioNotifier = new QSocketNotifier(0, QSocketNotifier::Read, this);
    connect(m_stdioNotifier, &QSocketNotifier::activated, this, &McpServer::onStdioData);
    emit runningChanged(true);
    return true;
}

void McpServer::onStdioData()
{
    m_stdioBuffer += m_stdioIn->readAll();
    if (m_stdioBuffer.isEmpty() && m_stdioIn->atEnd())
    {
        // Client closed stdin: clean shutdown
        m_stdioNotifier->setEnabled(false);
        emit stdioClosed();
        return;
    }
    int idx;
    while ((idx = m_stdioBuffer.indexOf('\n')) != -1) {
        QByteArray line = m_stdioBuffer.left(idx);
        m_stdioBuffer.remove(0, idx + 1);
        line = line.trimmed();
        if (line.isEmpty())
            continue;
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            m_stdioOut->write(QJsonDocument(makeError(QJsonValue::Null, -32700,
                                     QString("Parse error: %1").arg(err.errorString()))).toJson(QJsonDocument::Compact) + "\n");
            continue;
        }
        const QByteArray resp = dispatchMessage(doc.object());
        if (!resp.isEmpty())
            m_stdioOut->write(resp + "\n");
    }
    m_stdioOut->flush();
}

void McpServer::onNewConnection()
{
    while (QTcpSocket *client = m_server->nextPendingConnection()) {
        m_clients.append(client);
        m_buffers[client] = QByteArray();
        QObject::connect(client, &QTcpSocket::readyRead, this, &McpServer::onClientData);
        QObject::connect(client, &QTcpSocket::disconnected, this, &McpServer::onClientDisconnected);
        emit logMessage(QString("client %1:%2 connected").arg(client->peerAddress().toString()).arg(client->peerPort()));
    }
}

void McpServer::onClientData()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (client == nullptr)
        return;
    m_buffers[client] += client->readAll();
    handleHttpRequest(client);
}

// Minimal Streamable HTTP: POST /mcp carries one JSON-RPC message and gets
// one application/json response; GET/DELETE get 405 (no SSE, stateless);
// OPTIONS gets CORS headers. Connection is closed after each response.
void McpServer::handleHttpRequest(QTcpSocket *client)
{
    QByteArray &buffer = m_buffers[client];
    int headerEnd = buffer.indexOf("\r\n\r\n");
    if (headerEnd == -1)
        return;   // wait for the full header block

    const QList<QByteArray> lines = buffer.left(headerEnd).split('\n');
    const QList<QByteArray> parts = lines.first().trimmed().split(' ');
    int contentLength = 0;
    for (int i = 1; i < lines.size(); i++) {
        QByteArray l = lines[i].trimmed();
        if (l.toLower().startsWith("content-length:"))
            contentLength = l.mid(15).trimmed().toInt();
    }
    const int bodyStart = headerEnd + 4;
    if (buffer.size() < bodyStart + contentLength)
        return;   // wait for the full body

    const QString httpMethod = parts.value(0);
    const QByteArray body = buffer.mid(bodyStart, contentLength);
    buffer = QByteArray();

    if (httpMethod == "OPTIONS") {
        sendHttpResponse(client, 204, "text/plain", "");
    } else if (httpMethod == "GET" || httpMethod == "DELETE") {
        // No SSE stream / no session tracking: allowed by the spec
        sendHttpResponse(client, 405, "text/plain", "SSE and sessions not supported, use POST");
    } else if (httpMethod != "POST") {
        sendHttpResponse(client, 404, "text/plain", "not found");
    } else {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(body, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            sendHttpResponse(client, 400, "application/json",
                             QJsonDocument(makeError(QJsonValue::Null, -32700,
                                                     QString("Parse error: %1").arg(err.errorString()))).toJson(QJsonDocument::Compact));
        } else {
            const QByteArray resp = dispatchMessage(doc.object());
            if (resp.isEmpty())
                sendHttpResponse(client, 202, "application/json", "");   // notification
            else
                sendHttpResponse(client, 200, "application/json", resp);
        }
    }
    client->flush();
    client->disconnectFromHost();
}

void McpServer::sendHttpResponse(QTcpSocket *socket, int status, const QString &contentType, const QByteArray &body)
{
    const char *statusText;
    switch (status) {
        case 200: statusText = "OK"; break;
        case 202: statusText = "Accepted"; break;
        case 204: statusText = "No Content"; break;
        case 400: statusText = "Bad Request"; break;
        case 404: statusText = "Not Found"; break;
        case 405: statusText = "Method Not Allowed"; break;
        default:  statusText = "Error"; break;
    }
    QByteArray head;
    head += QString("HTTP/1.1 %1 %2\r\n").arg(status).arg(statusText).toUtf8();
    head += "Content-Type: " + contentType.toUtf8() + "\r\n";
    head += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    head += "Access-Control-Allow-Origin: *\r\n";
    head += "Access-Control-Allow-Methods: POST, GET, DELETE, OPTIONS\r\n";
    head += "Connection: close\r\n\r\n";
    socket->write(head + body);
}

void McpServer::onClientDisconnected()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (client == nullptr)
        return;
    m_clients.removeAll(client);
    m_buffers.remove(client);
    client->deleteLater();
}

QJsonObject McpServer::makeResult(const QJsonValue &id, const QJsonObject &result)
{
    QJsonObject response;
    response["jsonrpc"] = "2.0";
    response["id"] = id;
    response["result"] = result;
    return response;
}

QJsonObject McpServer::makeError(const QJsonValue &id, int code, const QString &message)
{
    QJsonObject response;
    response["jsonrpc"] = "2.0";
    response["id"] = id;
    response["error"] = QJsonObject{ {"code", code}, {"message", message} };
    return response;
}

QByteArray McpServer::dispatchMessage(const QJsonObject &msg)
{
    const QString method = msg.value("method").toString();
    const QJsonValue id = msg.value("id");
    const bool isNotification = id.isUndefined() || id.isNull();
    const QJsonObject params = msg.value("params").toObject();

    if (method == "initialize") {
        QJsonObject result;
        // Echo the client's requested protocol revision when provided
        const QString requested = params.value("protocolVersion").toString();
        result["protocolVersion"] = requested.isEmpty() ? QString(MCP_PROTOCOL_VERSION) : requested;
        QJsonObject toolsCap;
        toolsCap["listChanged"] = false;
        result["capabilities"] = QJsonObject{ {"tools", toolsCap} };
        result["serverInfo"] = QJsonObject{
            {"name", "SNESTilesRipper"},
            {"version", QCoreApplication::applicationVersion()}
        };
        return QJsonDocument(makeResult(id, result)).toJson(QJsonDocument::Compact);
    }
    if (method.startsWith("notifications/")) {
        // notification, nothing to answer
        return QByteArray();
    }
    if (method == "ping") {
        return QJsonDocument(makeResult(id, QJsonObject())).toJson(QJsonDocument::Compact);
    }
    if (method == "tools/list") {
        QJsonObject result;
        result["tools"] = buildToolsList();
        return QJsonDocument(makeResult(id, result)).toJson(QJsonDocument::Compact);
    }
    if (method == "tools/call") {
        const QString name = params.value("name").toString();
        const QJsonObject args = params.value("arguments").toObject();
        bool isError = false;
        QJsonObject result = handleToolCall(name, args, isError);
        return QJsonDocument(makeResult(id, result)).toJson(QJsonDocument::Compact);
    }
    if (isNotification)
        return QByteArray();
    return QJsonDocument(makeError(id, -32601, QString("Method not found: %1").arg(method))).toJson(QJsonDocument::Compact);
}

QJsonObject McpServer::toolSchema(const QJsonObject &properties, const QStringList &required) const
{
    QJsonObject schema;
    schema["type"] = "object";
    schema["properties"] = properties;
    if (!required.isEmpty()) {
        QJsonArray req;
        for (const QString &r : required)
            req.append(r);
        schema["required"] = req;
    }
    return schema;
}

static QJsonObject stringProp(const QString &desc)
{
    return QJsonObject{ {"type", "string"}, {"description", desc} };
}

static QJsonObject intProp(const QString &desc, int minVal = INT_MIN, int maxVal = INT_MAX)
{
    QJsonObject p{ {"type", "integer"}, {"description", desc} };
    if (minVal != INT_MIN) p["minimum"] = minVal;
    if (maxVal != INT_MAX) p["maximum"] = maxVal;
    return p;
}

static QJsonObject boolProp(const QString &desc)
{
    return QJsonObject{ {"type", "boolean"}, {"description", desc} };
}

QJsonArray McpServer::buildToolsList() const
{
    QJsonArray tools;
    auto add = [&tools](const QString &name, const QString &desc, const QJsonObject &schema) {
        QJsonObject tool;
        tool["name"] = name;
        tool["description"] = desc;
        tool["inputSchema"] = schema;
        tools.append(tool);
    };

    add("get_state", "Get the full current state of the tool: loaded ROM info, tiles location/properties, palette, pattern, zoom, available compressions and patterns.",
        toolSchema(QJsonObject()));
    add("load_rom", "Load a SNES ROM file and auto-detect header/mapping.",
        toolSchema(QJsonObject{ {"path", stringProp("Path of the ROM file (.sfc/.smc)")} }, {"path"}));
    add("set_tiles_location", "Set where the tiles are read from, like typing in the UI. Give pc (hex file offset) or snes (hex SNES bus address), plus optional size/bpp/compression. Call refresh afterwards to update the view.",
        toolSchema(QJsonObject{
            {"pc", stringProp("Hex PC file offset (ex: 80000). Assumes no SMC header")},
            {"snes", stringProp("Hex SNES address (ex: 480000 for HiROM, C80000 also accepted)")},
            {"size", intProp("Number of bytes to decode", 1)},
            {"bpp", intProp("Bits per pixel: 1, 2, 3 or 4", 1, 4)},
            {"compression", stringProp("Compression name: None or one of the loaded plugin compressions")}
        }));
    add("set_palette", "Set the palette source: a PC/SNES location in the ROM, or grayscale. Call refresh afterwards.",
        toolSchema(QJsonObject{
            {"pc", stringProp("Hex PC offset of the palette")},
            {"snes", stringProp("Hex SNES address of the palette")},
            {"gray", boolProp("Use shades of gray instead of a ROM palette")},
            {"gray_size", intProp("Size of the gray palette (colors)", 2, 256)},
            {"no_zero_color", boolProp("Check 'Color 0 is not included'")}
        }));
    add("refresh", "Re-extract tiles/palette with the current settings and rebuild the view (same as clicking Refresh).",
        toolSchema(QJsonObject()));
    add("set_pattern", "Set the tiles arrangement pattern (rebuilds the view if tiles are loaded).",
        toolSchema(QJsonObject{ {"name", stringProp("Pattern name, ex: normal, 32x32 A (2x2*4). See get_state patterns list")} }, {"name"}));
    add("set_zoom", "Set the zoom level of the tiles view.", toolSchema(QJsonObject{ {"value", intProp("Zoom slider value (clamped to the slider range)", 1, 99)} }, {"value"}));
    add("export_png", "Export the current tiles arrangement (with palette) as a PNG file.",
        toolSchema(QJsonObject{ {"path", stringProp("Destination PNG file path")} }, {"path"}));
    add("inject_png", "Inject a PNG back into the ROM (Replace with PNG). The PNG must match the current view dimensions.",
        toolSchema(QJsonObject{
            {"path", stringProp("PNG file path to inject")},
            {"work_on_copy", boolProp("Inject into a copy of the ROM instead of the ROM itself (default true)")},
            {"use_image_palette", boolProp("Also inject the PNG palette into the ROM (default true)")}
        }, {"path"}));
    add("screenshot", "Take a screenshot of the whole tool window and save it as PNG (lets you see what the user sees).",
        toolSchema(QJsonObject{ {"path", stringProp("Destination PNG file path")} }, {"path"}));
    add("open_savestate", "Open an emulator savestate (Snes9x snapshot format) to read its palettes.",
        toolSchema(QJsonObject{ {"path", stringProp("Savestate file path")} }, {"path"}));
    add("pick_savestate_palette", "Use the palette at the given index from the opened savestate (0-15).",
        toolSchema(QJsonObject{ {"index", intProp("Palette index in the savestate CGRAM", 0, 15)} }, {"index"}));
    add("save_preset", "Save the current tile settings (location, size, bpp, compression, palette, pattern) into a .stk preset file.",
        toolSchema(QJsonObject{ {"path", stringProp("Destination .stk file path")} }, {"path"}));
    add("open_preset", "Load a .stk preset file and rebuild the view if a ROM is loaded.",
        toolSchema(QJsonObject{ {"path", stringProp("Preset file path")} }, {"path"}));
    add("set_language", "Switch the UI language.",
        toolSchema(QJsonObject{ {"lang", stringProp(QString("Language id (ex: en, pt_BR). Available ids are listed in get_state 'languages': %1").arg(m_main ? m_main->languageList().join(", ") : "en"))} }, {"lang"}));
    add("set_view_background", "Set the tile view background: a hex color or a checkerboard (checker highlights transparent pixels and assets).",
        toolSchema(QJsonObject{ {"spec", stringProp("Hex color like #202020, or 'checker'")} }, {"spec"}));
    return tools;
}

QJsonObject McpServer::textContent(const QString &text, bool isError) const
{
    QJsonObject content;
    content["type"] = "text";
    content["text"] = text;
    QJsonObject result;
    result["content"] = QJsonArray{ content };
    result["isError"] = isError;
    return result;
}

QJsonObject McpServer::handleToolCall(const QString &name, const QJsonObject &args, bool &isError)
{
    isError = false;
    if (m_main == nullptr) {
        isError = true;
        return textContent("No MainUI target", true);
    }

    // helpers to convert arguments
    auto hexArg = [&](const char *key, bool *present) -> unsigned int {
        *present = args.contains(key);
        if (!*present) return 0;
        QString v = args.value(key).toString();
        if (v.startsWith("0x", Qt::CaseInsensitive)) v.remove(0, 2);
        v.remove('$');
        bool ok = false;
        unsigned int val = v.toUInt(&ok, 16);
        if (!ok) { *present = false; }
        return val;
    };

    if (name == "get_state") {
        return textContent(QJsonDocument(QJsonObject::fromVariantMap(m_main->collectState())).toJson(QJsonDocument::Indented).constData());
    }
    if (name == "load_rom") {
        QString path = args.value("path").toString();
        if (!QFileInfo::exists(path)) {
            isError = true;
            return textContent(QString("File not found: %1").arg(path), true);
        }
        m_main->openRom(path);
        QVariantMap st = m_main->collectState();
        return textContent(QString("ROM loaded: %1 (%2, header=%3)")
                           .arg(st.value("rom_file").toString(), st.value("rom_type").toString())
                           .arg(st.value("rom_has_header").toBool() ? "yes" : "no"));
    }
    if (name == "set_tiles_location") {
        bool present;
        QVariantMap cfg;
        hexArg("pc", &present);  if (present) cfg["pc_hex"] = args.value("pc").toString();
        hexArg("snes", &present); if (present) cfg["snes_hex"] = args.value("snes").toString();
        if (args.contains("size")) cfg["size"] = args.value("size").toInt();
        if (args.contains("bpp")) cfg["bpp"] = args.value("bpp").toInt();
        if (args.contains("compression")) cfg["compression"] = args.value("compression").toString();
        QString err = m_main->setTileSettings(cfg);
        if (!err.isEmpty()) { isError = true; return textContent(err, true); }
        return textContent("Tiles location/properties updated (call refresh to extract)");
    }
    if (name == "set_palette") {
        QVariantMap cfg;
        if (args.contains("pc")) cfg["pc_hex"] = args.value("pc").toString();
        if (args.contains("snes")) cfg["snes_hex"] = args.value("snes").toString();
        if (args.contains("gray")) cfg["gray"] = args.value("gray").toBool();
        if (args.contains("gray_size")) cfg["gray_size"] = args.value("gray_size").toInt();
        if (args.contains("no_zero_color")) cfg["no_zero_color"] = args.value("no_zero_color").toBool();
        QString err = m_main->setPaletteSettings(cfg);
        if (!err.isEmpty()) { isError = true; return textContent(err, true); }
        return textContent("Palette settings updated (call refresh to extract)");
    }
    if (name == "refresh") {
        m_main->refreshView();
        QVariantMap st = m_main->collectState();
        return textContent(QString("View refreshed, %1 tiles extracted").arg(st.value("tiles_extracted").toInt()));
    }
    if (name == "set_pattern") {
        QString err = m_main->setPattern(args.value("name").toString());
        if (!err.isEmpty()) { isError = true; return textContent(err, true); }
        return textContent(QString("Pattern set to %1").arg(args.value("name").toString()));
    }
    if (name == "set_zoom") {
        m_main->setZoom(args.value("value").toInt());
        return textContent(QString("Zoom set to %1").arg(args.value("value").toInt()));
    }
    if (name == "export_png") {
        QString err;
        if (!m_main->exportPngTo(args.value("path").toString(), &err)) {
            isError = true;
            return textContent(err, true);
        }
        return textContent(QString("PNG exported to %1").arg(args.value("path").toString()));
    }
    if (name == "inject_png") {
        QString err;
        bool workOnCopy = args.contains("work_on_copy") ? args.value("work_on_copy").toBool() : true;
        bool useImagePal = args.contains("use_image_palette") ? args.value("use_image_palette").toBool() : true;
        QString res = m_main->injectPngFrom(args.value("path").toString(), workOnCopy, useImagePal, &err);
        if (!err.isEmpty()) { isError = true; return textContent(err, true); }
        return textContent(res.isEmpty() ? "Injection done" : res);
    }
    if (name == "screenshot") {
        QString err;
        if (!m_main->saveWindowScreenshot(args.value("path").toString(), &err)) {
            isError = true;
            return textContent(err, true);
        }
        return textContent(QString("Screenshot saved to %1").arg(args.value("path").toString()));
    }
    if (name == "open_savestate") {
        QString err;
        if (!m_main->openSavestateFile(args.value("path").toString(), &err)) {
            isError = true;
            return textContent(err, true);
        }
        return textContent(QString("Savestate opened: %1").arg(args.value("path").toString()));
    }
    if (name == "pick_savestate_palette") {
        QString err;
        if (!m_main->selectSavestatePalette(args.value("index").toInt(), &err)) {
            isError = true;
            return textContent(err, true);
        }
        return textContent(QString("Palette %1 from savestate applied").arg(args.value("index").toInt()));
    }
    if (name == "save_preset") {
        QString err;
        if (!m_main->savePresetTo(args.value("path").toString(), &err)) {
            isError = true;
            return textContent(err, true);
        }
        return textContent(QString("Preset saved to %1").arg(args.value("path").toString()));
    }
    if (name == "open_preset") {
        QString err;
        if (!m_main->openPresetFrom(args.value("path").toString(), &err)) {
            isError = true;
            return textContent(err, true);
        }
        return textContent(QString("Preset loaded from %1").arg(args.value("path").toString()));
    }
    if (name == "set_language") {
        QString lang = args.value("lang").toString();
        QStringList langs = m_main->languageList();
        if (!langs.contains(lang)) {
            isError = true;
            return textContent(QString("Unknown language '%1'. Available: %2").arg(lang, langs.join(", ")), true);
        }
        m_main->applyLanguage(lang);
        return textContent(QString("Language set to %1").arg(lang));
    }
    if (name == "set_view_background") {
        QString err = m_main->setViewBackground(args.value("spec").toString());
        if (!err.isEmpty()) {
            isError = true;
            return textContent(err, true);
        }
        return textContent(QString("View background set to %1").arg(m_main->collectState().value("view_background").toString()));
    }

    isError = true;
    return textContent(QString("Unknown tool: %1").arg(name), true);
}