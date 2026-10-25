SNESAssetsRipper - arquivos de tradução / translation files
===========================================================

Cada arquivo .txt desta pasta é um idioma da interface. O programa lista os
arquivos .txt encontrados aqui na menu "Options -> Language" e aplica o
escolhido na hora (o idioma também fica salvo entre sessões).

Formato de um arquivo de tradução:

  # name=Português (Brasil)      <- nome exibido no menu
  # locale=pt_BR                 <- locale (para traduzir textos do próprio Qt)

  [MainUI]                       <- contexto = nome da classe do formulário
  Open Preset = Abrir preset     <- texto original = tradução

Regras:
- Um par "original = tradução" por linha; a divisão é feito no primeiro "=".
- Linhas começando com "#" são comentários/cabeçalhos.
- Linhas vazias são ignoradas.
- Use \n para quebra de linha e \t para tabulação dentro dos textos.
- O contexto é o nome da classe do formulário (.ui): MainUI, InjectDialog,
  PaletteEditor, PresetSaveDialog, SavestatePaletteSelector, TileInfow.
- Para criar um novo idioma, copie um arquivo existente (ex: pt_BR.txt) e
  traduza a coluna da direita. Nenhum recompile é necessário.

---

Each .txt file in this folder is one UI language. The tool lists the .txt
files it finds here in the "Options -> Language" menu and applies the chosen
one live (the choice is persisted).

Format:

  # name=<display name>          shown in the Language menu
  # locale=<locale id>           used to load Qt builtin translations

  [FormClassName]                context = form class name
  Source text = Translated text  one pair per line, split on the first '='

- Lines starting with '#' are comments/headers.
- \n and \t escapes are decoded in both source and translation.
- To add a language: copy an existing file, translate the right-hand side,
  drop it here. No rebuild needed.