# SNESAssetsRipper

![SNESAssetsRipper — exploring Super Bomberman 5 tiles](docs/screenshot.png)

> **🤖 AI/MCP ready** — the tool exposes an MCP server (local HTTP, plus a headless stdio mode) so an AI agent can fully control it: load ROMs, locate and extract tiles, change palettes/patterns, export and inject PNGs, manage presets and more. The screenshot above was taken by an AI through it. See [docs/MCP_SERVER.md](docs/MCP_SERVER.md).

SNESAssetsRipper is a fork of [SNESTilesKitten](https://github.com/Skarsnik/SNESTilesKitten), a tool to view/extract/inject graphic tiles in a SNES Rom file. It allows you to use SNES address (support HiROM/LoROM) so you can use it to discover tile location directly from ASM trace.

If you just want to use the original software look at the [SNESTilesKitten webpage](https://skarsnik.github.io/SNESTilesKitten/)

This readme is written toward developpers.

# Checking out the project

Since the project uses an external project as a git submodule, use:

`git clone --recurse-submodules https://github.com/Skarsnik/SNESTilesKitten.git`

or, on an already cloned directory:

`git submodule update --init --recursive`

# Building the project

Requirements (Arch Linux package names):

| Package | Purpose |
|---|---|
| `qt6-base` | Qt 6 (Core/Gui/Widgets/Network) + `moc`/`uic` |
| `qt6-wayland` | Native Wayland GUI (optional, for Wayland sessions) |
| `cmake` + `make` or `ninja` | Build system |
| `gcc` / `pkgconf` / `git` | Compiler, Qt discovery, submodules |

Build (all binaries and compression plugins are output to `bin/`):

```sh
cmake -B build
cmake --build build -j$(nproc)
```

Run:

```sh
./bin/SNESAssetsRipper        # GUI
./bin/SNESAssetsRipperCLI     # command line tool
```

Runtime data (`TilesPatterns/`, `translations/`, `Presets/`) is resolved from
the repository root, one level above `bin/`. Compression plugins are loaded
from `bin/plugins/` (built automatically).  

# Project structure

```
src/                  all sources
  *.cpp *.h *.ui      application sources (GUI)
  tinymain.cpp        SNESAssetsRipperCLI command line tool
  lowlevelstuff/      git submodule: SNES low-level C code
  compressions/       compression plugins (Nintendo, Konami, Quintet, BandaiNamco)
TilesPatterns/        tile arrangement presets loaded at runtime
translations/         UI language files (*.txt), see translations/README.txt
Presets/              example .stk presets
docs/                 MCP server documentation
bin/                  build output (gitignored): GUI, CLI and bin/plugins/*.so
```

# Working on the code

You have to be carefull when you work on the code. All the low level stuff (mainly SNES specific stuff) are located in C files in the `lowlevelstuff` directory. You should probably work on the [base project](https://github.com/Skarsnik/sneshacking/) if you need to change/correct something in there.

# Compression

If you want write a new compression plugin, just copy/paste an already existing one and adapt the code. Qt plugins can be annoying to write from scratch.

# Translations / Languages

The UI language is selected in `Options -> Language`. The tool scans the `translations/` directory and lists every `*.txt` file found there as an available language (re-scanned each time the menu opens, so adding a language does not require rebuilding).

Format: `# name=` / `# locale=` headers, then one `source = translation` pair per line under a `[FormClassName]` section. See `translations/README.txt` for details. `pt_BR.txt` ships as an example.

# MCP server (AI control)

The `Options -> MCP AI control server` toggle starts a local MCP HTTP server (`http://127.0.0.1:8765/mcp`) exposing 15 tools that let an AI agent fully drive the tool: load ROM, set tile/palette locations, refresh, export/inject PNG, savestate palettes, presets, language switching and window screenshots. Connect OpenCode with `opencode mcp add snesassetsripper --global --url http://127.0.0.1:8765/mcp`. `Options -> MCP documentation` shows the in-app documentation window (with a copy button). The executable also works as a headless stdio MCP server (`SNESAssetsRipper --mcp-stdio`). Protocol and examples: [docs/MCP_SERVER.md](docs/MCP_SERVER.md).

# Licencing

All the code is under the GPL3 Licence