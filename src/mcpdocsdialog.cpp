#include "mcpdocsdialog.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QCoreApplication>

// Builds the documentation shown in the window (and copied by the button).
static QString buildDocumentation(const QString &language, const QString &tcpEndpoint)
{
    const QString exe = QCoreApplication::applicationFilePath();
    if (language.startsWith("pt"))
    {
        QString txt;
        txt += "SNESASSETSRIPPER - SERVIDOR MCP (CONTROLE POR IA)\n";
        txt += "====================================================\n\n";
        txt += "Esta ferramenta expõe um servidor MCP (Model Context Protocol) que\n";
        txt += "permite uma IA controlar tudo: carregar ROM, definir localização dos\n";
        txt += "tiles/paleta, atualizar a visão, exportar/injetar PNG, usar savestates,\n";
        txt += "presets, padrões de tiles e o idioma da interface.\n\n";
        txt += "COMO CONECTAR NO OPENCODE (servidor HTTP local, recomendado)\n";
        txt += "------------------------------------------------------------\n";
        txt += "1. Deixe esta janela aberta e ligue o servidor no menu Opções\n";
        txt += "   (\"Servidor MCP de controle por IA\").\n";
        txt += "2. No terminal, registre o servidor no OpenCode:\n\n";
        txt += "  opencode mcp add snesassetsripper --global --url " + (tcpEndpoint.isEmpty() ? "http://127.0.0.1:8765/mcp" : tcpEndpoint) + "\n\n";
        txt += "3. Confira com:\n\n  opencode mcp list\n\n";
        txt += "As tools ficam disponíveis como tools.snesassetsripper.<tool>(...) no\n";
        txt += "Code Mode, controlando ESTA janela que você está vendo.\n\n";
        txt += "MODO STDIO ALTERNATIVO (sem janela)\n";
        txt += "-----------------------------------\n";
        txt += "O executável também é um servidor MCP stdio (o cliente o inicia):\n\n";
        txt += "  opencode mcp add snesassetsripper --global -- " + exe + " --mcp-stdio\n\n";
        txt += "Nesse modo o processo roda em segundo plano, sem janela, falando MCP\n";
        txt += "por stdin/stdout (requer ambiente sem tema GTK forçado).\n\n";
        txt += "PROTOCOLO (Streamable HTTP em " + (tcpEndpoint.isEmpty() ? "http://127.0.0.1:8765/mcp" : tcpEndpoint) + ")\n";
        txt += "----------------------------------------\n";
        txt += "POST /mcp com JSON-RPC 2.0 no corpo, uma mensagem por requisição.\n";
        txt += "Métodos: initialize, ping, tools/list, tools/call. Notificações\n";
        txt += "respondem 202 Accepted. Cada tools/call responde\n";
        txt += "{\"content\":[{\"type\":\"text\",\"text\":\"...\"}],\"isError\":bool}, onde text\n";
        txt += "é uma mensagem legível ou um JSON de estado.\n\n";
        txt += "FERRAMENTAS DISPONÍVEIS\n";
        txt += "-----------------------\n";
        txt += "  get_state             Estado completo (ROM, tiles, paleta, pattern,\n";
        txt += "                        zoom, compressões, patterns, idiomas)\n";
        txt += "  load_rom              path: carrega a ROM (detecta header/mapeamento)\n";
        txt += "  set_tiles_location    pc ou snes (hex), size, bpp (1-4), compression\n";
        txt += "  set_palette           pc ou snes (hex), gray, gray_size, no_zero_color\n";
        txt += "  refresh               Re-extrai tiles/paleta e redesenha (botão Atualizar)\n";
        txt += "  set_pattern           name: arranjo dos tiles (normal, 32x32 A/B)\n";
        txt += "  set_zoom              value: nível de zoom\n";
        txt += "  export_png            path: exporta os tiles atuais como PNG\n";
        txt += "  inject_png            path, work_on_copy (padrão true), use_image_palette\n";
        txt += "  screenshot            path: screenshot da janela (para a IA ver a tela)\n";
        txt += "  open_savestate        path: abre savestate do Snes9x\n";
        txt += "  pick_savestate_palette index (0-15): aplica paleta do savestate\n";
        txt += "  save_preset / open_preset   path: presets .stk\n";
        txt += "  set_language          lang: en ou pt_BR\n\n";
        txt += "NOTA: set_tiles_location / set_palette / set_pattern não atualizam a\n";
        txt += "visão sozinhos - chame refresh em seguida (como clicar em Atualizar).\n\n";
        txt += "EXEMPLO DE CHAMADA\n";
        txt += "------------------\n";
        txt += "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/call\",\"params\":{\"name\":\n";
        txt += "\"load_rom\",\"arguments\":{\"path\":\"/roms/jogo.sfc\"}}}\n\n";
        txt += "O estado do servidor fica salvo entre sessões (chave mcpEnabled).\n";
        return txt;
    }
    QString txt;
    txt += "SNESASSETSRIPPER - MCP SERVER (AI CONTROL)\n";
    txt += "=========================================\n\n";
    txt += "This tool exposes an MCP (Model Context Protocol) server that lets an\n";
    txt += "AI drive everything: load ROM, set tile/palette locations, refresh the\n";
    txt += "view, export/inject PNGs, use savestates, presets, tile patterns and\n";
    txt += "the UI language.\n\n";
    txt += "HOW TO CONNECT WITH OPENCODE (local HTTP server, recommended)\n";
    txt += "-------------------------------------------------------------\n";
    txt += "1. Keep this window open and enable the server in the Options menu\n";
    txt += "   (\"MCP AI control server\").\n";
    txt += "2. In a terminal, register the server with OpenCode:\n\n";
    txt += "  opencode mcp add snesassetsripper --global --url " + (tcpEndpoint.isEmpty() ? "http://127.0.0.1:8765/mcp" : tcpEndpoint) + "\n\n";
    txt += "3. Check with:\n\n  opencode mcp list\n\n";
    txt += "Tools become available as tools.snesassetsripper.<tool>(...) in Code\n";
    txt += "Mode, driving THIS window you are looking at.\n\n";
    txt += "ALTERNATE STDIO MODE (headless)\n";
    txt += "-------------------------------\n";
    txt += "The executable is also a stdio MCP server (the client spawns it):\n\n";
    txt += "  opencode mcp add snesassetsripper --global -- " + exe + " --mcp-stdio\n\n";
    txt += "In that mode the process runs in the background, windowless, speaking\n";
    txt += "MCP over stdin/stdout (needs an environment without a forced GTK\n";
    txt += "platform theme).\n\n";
    txt += "PROTOCOL (Streamable HTTP on " + (tcpEndpoint.isEmpty() ? "http://127.0.0.1:8765/mcp" : tcpEndpoint) + ")\n";
    txt += "------------------------------------\n";
    txt += "POST /mcp with JSON-RPC 2.0 in the body, one message per request.\n";
    txt += "Methods: initialize, ping, tools/list, tools/call. Notifications\n";
    txt += "answer 202 Accepted. Every tools/call answers\n";
    txt += "{\"content\":[{\"type\":\"text\",\"text\":\"...\"}],\"isError\":bool} where text is\n";
    txt += "a human-readable message or a JSON state object.\n\n";
    txt += "AVAILABLE TOOLS\n";
    txt += "---------------\n";
    txt += "  get_state             Full state (ROM, tiles, palette, pattern, zoom,\n";
    txt += "                        compressions, patterns, languages)\n";
    txt += "  load_rom              path: load a ROM (auto header/mapping)\n";
    txt += "  set_tiles_location    pc or snes (hex), size, bpp (1-4), compression\n";
    txt += "  set_palette           pc or snes (hex), gray, gray_size, no_zero_color\n";
    txt += "  refresh               Re-extract tiles/palette and rebuild (Refresh)\n";
    txt += "  set_pattern           name: tile arrangement (normal, 32x32 A/B)\n";
    txt += "  set_zoom              value: zoom level\n";
    txt += "  export_png            path: export current tiles as PNG\n";
    txt += "  inject_png            path, work_on_copy (default true), use_image_palette\n";
    txt += "  screenshot            path: window screenshot (lets the AI see the UI)\n";
    txt += "  open_savestate        path: open a Snes9x savestate\n";
    txt += "  pick_savestate_palette index (0-15): apply a savestate palette\n";
    txt += "  save_preset / open_preset   path: .stk presets\n";
    txt += "  set_language          lang: en or pt_BR\n\n";
    txt += "NOTE: set_tiles_location / set_palette / set_pattern do not refresh the\n";
    txt += "view by themselves - call refresh afterwards (like clicking Refresh).\n\n";
    txt += "CALL EXAMPLE\n";
    txt += "------------\n";
    txt += "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/call\",\"params\":{\"name\":\n";
    txt += "\"load_rom\",\"arguments\":{\"path\":\"/roms/game.sfc\"}}}\n\n";
    txt += "The server state is persisted across sessions (mcpEnabled settings key).\n";
    return txt;
}

McpDocsDialog::McpDocsDialog(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle(tr("MCP documentation"));
    resize(760, 640);

    m_text = new QTextEdit(this);
    m_text->setReadOnly(true);
    m_text->setLineWrapMode(QTextEdit::NoWrap);
    QFont mono("monospace");
    mono.setStyleHint(QFont::TypeWriter);
    m_text->setFont(mono);

    QPushButton *copyButton = new QPushButton(tr("Copy documentation"), this);
    copyButton->setToolTip(tr("Copy the documentation text to the clipboard"));
    QPushButton *closeButton = new QPushButton(tr("Close"), this);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addWidget(copyButton);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(closeButton);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_text);
    mainLayout->addLayout(buttonsLayout);

    connect(copyButton, &QPushButton::clicked, this, &McpDocsDialog::copyDocumentation);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
}

void McpDocsDialog::setDocumentation(const QString &language, const QString &tcpEndpoint)
{
    m_language = language;
    m_tcpEndpoint = tcpEndpoint;
    m_text->setPlainText(buildDocumentation(m_language, tcpEndpoint));
    setWindowTitle(tr("MCP documentation"));
}

void McpDocsDialog::copyDocumentation()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(m_text->toPlainText());
}