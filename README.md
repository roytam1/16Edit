# 16Edit

A small, fast Win32 hex editor for viewing and editing files and memory
buffers — including very large files (> 4 GB) and both 32-bit and 64-bit
PE files. One window edits one file; dropping more files opens more windows.

Originated from yoda's 16Edit module, extended by slangmgh (1.04+).

## Features

- Hex + ASCII panes with in-place byte editing (hex nibble or text mode, `TAB` switches panes)
- Unlimited undo/redo
- Insert / delete / cut / copy / paste (insert or overwrite mode)
- Search and replace: hex bytes, ASCII, Unicode, case-sensitive or not, up/down, replace-all
- Go to offset (file offset or PE virtual address), select block by start/end/size
- PE-aware: file-offset ↔ virtual-address display and conversion for 32-bit and 64-bit PEs
- 64-bit file offsets: files larger than 4 GB open, scroll, search and save
- Read-only / read-write toggle, size-lock toggle, ANSI / native (DBCS) display toggle
- Always-on-top toggle, minimize-to-tray, shell ("Open with 16Edit") integration
- Drag-and-drop a file onto the window to open it in a new window
- Remembers window position, goto/search/replace history in `16Edit.ini`

## Usage

```
16Edit.exe [file [start [len]]]
```

- With no arguments, an Open dialog is shown.
- `start` / `len` are numeric offsets selecting an initial block.
- `F5` reloads the current file (prompts to save first if dirty).
- `ESC` clears the selection, or minimizes when nothing is selected.

### Keyboard shortcuts

| Keys | Action |
|---|---|
| `Ctrl+A` | Select all / deselect |
| `Ctrl+B` | Select block |
| `Ctrl+C` | Copy |
| `Ctrl+X` | Cut |
| `Ctrl+V` | Paste |
| `Delete` | Delete selected block |
| `Ctrl+Z` / `Ctrl+Y` | Undo / redo |
| `Ctrl+S` | Save |
| `Ctrl+F` / `F3` / `Ctrl+F3` | Search / again down / again up |
| `Ctrl+R` | Replace |
| `Ctrl+G` | Go to offset |
| `Ctrl+T` / `Ctrl+E` / `Ctrl+I` | Always on top / file-offset↔VA / insert↔overwrite |
| `Ctrl+W` / `Ctrl+D` / `Ctrl+L` | Read-only / ANSI-native / size lock |
| `Ctrl+O` | Options (shell menu, working directory mode) |
| `Tab`, arrows, `PgUp`/`PgDn` | Switch pane / move caret |
| `F5` | Reload file |
| `F12` | About |
| `ESC` | Close window |

## Large files

- Offsets and sizes are 64-bit throughout; I/O is chunked so files past 4 GB work.
- 64-bit builds map the whole file when RAM allows.
- 32-bit builds fall back automatically when a file can't be `malloc`'d
  (practically anything approaching 2 GB): a 64MB sliding mapped window
  serves reads, single-byte edits go to a small overlay, and inserts/deletes
  go through a piece table backed by a temp add-store file, so viewing and
  full editing work with flat RAM usage. Saving streams through a temp file
  and atomically replaces the original.
- The offset column shows 8 digits below 4 GB and 16 above, shifting the
  hex/ASCII panes right so nothing overlaps.

## Building

- Visual C++ 6: `16Edit.dsw` / `16Edit.dsp` (`bin\16Edit.exe`).
- Visual Studio 2005: `16Edit.sln` / `16Edit.vcproj` — `Win32`, `x64`,
  `Itanium` (`bin\16Edit.exe`, `bin\16Edit_x64.exe`, `bin\16Edit_IA64.exe`).
- NMAKE makefiles for other targets: `16Edit-I386.mak`,
  `16Edit-MIPS.mak`, `16Edit-PPC.mak`, `16Edit-AXP32.mak`,
  `16Edit-AXP64.mak` (`bin\16Edit_I386.exe`, …).
- Links: `kernel32 user32 gdi32 comdlg32 advapi32 shell32 ole32 oleaut32
  uuid COMCTL32`. Resources: `rsrc.rc` (toolbar, dialogs, icons,
  accelerators).

## Layout

| File | Role |
|---|---|
| `16EditLoader.cpp` | `WinMain`/`main`, argument parsing, startup |
| `HexEditWnd.{h,cpp}` | Editor core: paint, caret, selection, undo/redo, search/replace, paging + piece table |
| `DialogProc.cpp` | Window proc and goto/select/search/replace/options dialogs |
| `File.{h,cpp}` | File I/O plus `CPagedFile` sliding-window reader |
| `Common.{h,cpp}` | Edit-box hooks, hex parsing, PE `file_type`/`get_va`/`get_fo` |
| `list.cpp` | Undo/redo operation list (`HE_OPER`) |
| `OFN.{h,cpp}`, `CPathString.{h,cpp}`, `WideChar.{h,cpp}` | File dialogs, path helpers, Unicode conversion |
| `Macros.h`, `resource.h`, `rsrc/` | Shared macros, IDs, toolbar bitmap and icons |

`16Edit.ini` (next to the exe) stores window position, font (`fh`/`fn`/`fq`),
goto/search/replace history, shell-menu and working-directory settings.

## History

- yoda's original 16Edit module (small/green/robust freeware hex editor).
- slangmgh 1.04+: clipboard text copy, saved goto/search/replace history,
  paste-enabled search/replace fields, `F5` reload, bug fixes.
- Later work: VS2005 x64/Itanium/Alpha projects; full 64-bit offsets and
  chunked I/O; 32-bit file-backed paging with piece-table editing;
  cross-bitness PE virtual-address translation; drag-and-drop to open.
