# MCP Server (AI control)

SNESTilesRipper can expose a **MCP (Model Context Protocol) server** so an AI
agent can fully control the tool: load ROMs, set tile locations, extract,
export/inject PNGs, pick savestate palettes, manage presets and switch the UI
language. The AI drives the **same window you are looking at**.

## Local HTTP server (recommended)

- Menu **Options -> MCP AI control server** (check to enable, uncheck to stop).
  The state is persisted: if enabled when you close, it restarts on launch.
- Endpoint: `http://127.0.0.1:8765/mcp` (port via the `mcpPort` settings key,
  localhost only).
- Register it with OpenCode:

  ```sh
  opencode mcp add snestilesripper --global --url http://127.0.0.1:8765/mcp
  opencode mcp list        # expect: ✓ snestilesripper connected
  ```

- Tools become available as `tools.snestilesripper.<tool>(...)` (Code Mode) or
  `snestilesripper_<tool>` depending on the client.
- Use **Options -> MCP documentation** inside the app to see this documentation
  in a window with a **copy** button.

## Headless stdio mode (alternative)

The executable itself is also a stdio MCP server, for clients that spawn the
process (no window, no GUI needed):

```sh
opencode mcp add snestilesripper --global -- /path/to/SNESTilesRipper --mcp-stdio
```

Notes: the process forces `QT_QPA_PLATFORM=offscreen` and drops a forced GTK
platform theme automatically, so it works even when the spawning environment
has no display. It never touches the GUI instance settings or its TCP server.

## Protocol

Both transports speak the same JSON-RPC 2.0 MCP messages:

- HTTP (Streamable HTTP, stateless): `POST /mcp` with one JSON-RPC message;
  requests get `200 application/json`, notifications `202 Accepted`;
  `GET`/`DELETE` answer `405` (no SSE, no session tracking).
- stdio: newline-delimited JSON-RPC on stdin/stdout.

Methods: `initialize` (echoes the client protocol version), `ping`,
`tools/list`, `tools/call`. Every `tools/call` result is
`{"content": [{"type": "text", "text": "..."}], "isError": bool}` where `text`
is a plain message or a JSON object (documented per tool below).

## Tools

| Tool | Arguments | Description |
|---|---|---|
| `get_state` | – | Full state: rom info, tiles location/properties, palette, pattern, zoom, compressions, patterns, languages |
| `load_rom` | `path` | Load a SNES ROM, auto-detect header/mapping |
| `set_tiles_location` | `pc` or `snes` (hex), `size`, `bpp` (1-4), `compression` | Set tile source like typing in the UI |
| `set_palette` | `pc` or `snes` (hex), `gray`, `gray_size`, `no_zero_color` | Set palette source |
| `refresh` | – | Re-extract tiles/palette with the current settings and rebuild the view (same as clicking Refresh) |
| `set_pattern` | `name` | Tiles arrangement (`normal`, `32x32 A (2x2*4)`, ...) |
| `set_zoom` | `value` | Zoom slider value |
| `export_png` | `path` | Export the current tiles view as a PNG file |
| `inject_png` | `path`, `work_on_copy` (default true), `use_image_palette` (default true) | Replace with PNG |
| `screenshot` | `path` | Screenshot of the whole tool window (lets you see what the user sees) |
| `open_savestate` | `path` | Open an emulator savestate (Snes9x format) to read its palettes |
| `pick_savestate_palette` | `index` (0-15) | Use the palette at the given index from the opened savestate |
| `save_preset` | `path` | Save the current tile settings as a preset (.stk) |
| `open_preset` | `path` | Load a preset (.stk) file |
| `set_language` | `lang` | Switch the UI language (ids in `get_state` → `languages`) |
| `set_view_background` | `spec` | Tile view background: hex color (`#RRGGBB`) or `checker` (checkerboard — highlights transparent pixels and assets) |

`set_tiles_location` / `set_palette` / `set_pattern` do not refresh
automatically — call `refresh` afterwards, like a user clicking Refresh.

## Example session

With the app open and the server enabled:

```python
import json, urllib.request

URL = "http://127.0.0.1:8765/mcp"

def post(payload):
    req = urllib.request.Request(URL, data=json.dumps(payload).encode(),
                                 headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=15) as r:
        body = r.read()
        return json.loads(body) if body else None

post({"jsonrpc": "2.0", "method": "initialize",
      "params": {"protocolVersion": "2025-06-18", "capabilities": {}}})
post({"jsonrpc": "2.0", "method": "notifications/initialized"})
post({"jsonrpc": "2.0", "id": 3, "method": "tools/call",
      "params": {"name": "load_rom", "arguments": {"path": "/roms/game.sfc"}}})
post({"jsonrpc": "2.0", "id": 4, "method": "tools/call",
      "params": {"name": "set_tiles_location",
                 "arguments": {"pc": "80000", "size": 65536, "bpp": 4,
                               "compression": "None"}}})
post({"jsonrpc": "2.0", "id": 5, "method": "tools/call",
      "params": {"name": "set_palette", "arguments": {"pc": "19940"}}})
post({"jsonrpc": "2.0", "id": 6, "method": "tools/call", "params": {"name": "refresh"}})
post({"jsonrpc": "2.0", "id": 7, "method": "tools/call",
      "params": {"name": "screenshot", "arguments": {"path": "/tmp/out.png"}}})
```

Quick check with curl:

```sh
curl -s -X POST http://127.0.0.1:8765/mcp \
  -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":1,"method":"tools/list"}'
```

---

# Servidor MCP (controle por IA) — resumo em português

- Ligue em **Opções -> Servidor MCP de controle por IA** (fica salvo).
- Endpoint HTTP local: `http://127.0.0.1:8765/mcp`.
- Conecte o OpenCode:
  `opencode mcp add snestilesripper --global --url http://127.0.0.1:8765/mcp`
- **Opções -> Documentação MCP** mostra esta documentação dentro do programa,
  com botão para copiar (já inclui o comando de registro pronto).
- 15 tools deixam uma IA controlar a janela que você está vendo: carregar ROM,
  definir localização dos tiles/paleta, atualizar visão, exportar/injetar PNG,
  savestates, presets, idioma e screenshot da janela.
- `set_tiles_location`/`set_palette`/`set_pattern` não atualizam a visão
  sozinhos — chame `refresh` em seguida.
- Modo stdio alternativo (sem janela): o executável com `--mcp-stdio`.