# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

`fuseaw` is a FUSE3 filesystem that exposes the contents of Wwise `.pck` (AKPK) audio archives as a read-only directory tree, so individual sound/bank files can be accessed without unpacking the whole archive. Files in `.pck` archives are addressed by numeric IDs and are otherwise opaque to normal tooling; this project translates them into a navigable hierarchy and serves reads on demand.

## Build

The project is plain C++23 with no build system — invoke `g++` directly.

**Main binary** (the FUSE driver):

```bash
g++ -std=c++23 fuseaw.cpp src/*.cpp -o fuseaw $(pkg-config fuse3 --cflags --libs)
```

Requires `libfuse3` (`fuse3` package) and a C++23 compiler. The compile line is also embedded as a comment at the top of `fuseaw.cpp:22`.

**Example utilities** in `examples/` (one-off CLI tools that exercise the library code without mounting FUSE):

```bash
# Header / sector scanner
g++ -std=c++23 examples/test_wavescan.cpp src/wavescan.cpp -Isrc -o test_wavescan

# Tabular dump of every entry in a .pck
g++ -std=c++23 examples/test_file_list.cpp src/wavescan.cpp -Isrc -o test_file_list

# Dump the language-sector string table
g++ -std=c++23 examples/test_languages.cpp src/wavescan.cpp -Isrc -o test_languages

# Pull a single file out of the archive by ID
g++ -std=c++23 examples/test_file_extract.cpp src/wavescan.cpp -Isrc -o test_file_extract

# Pull a single .wem out of a .bnk by wem ID
g++ -std=c++23 examples/test_bnk_extract.cpp src/*.cpp -Isrc -o test_bnk_extract

# Parse a .bnk and list its embedded wems
g++ -std=c++23 examples/test_parse_bnk.cpp src/*.cpp -Isrc -o test_parse_bnk
```

Each example has its exact compile command in a leading comment — copy from there.

## Run

The mount needs a `.pck` file plus a mount point, then forwards the rest to FUSE:

```bash
./fuseaw path/to/archive.pck /mnt/fuseaw
```

The driver forces `-r` (read-only) regardless of extra args. FUSE's `kernel_cache` is enabled in `fuseaw.cpp:73`. No `fusermount -u` is needed beyond the standard `fusermount3 -u /mnt/fuseaw`.

## Architecture

Single-process read-only FUSE driver. The lifetime of a request is:

1. `main` (`fuseaw.cpp:208`) opens the `.pck`, calls `parse_akpk_file` to extract every entry, then `construct_akpk_filesystem` to build an in-memory tree of `AKPKFilesystemNode`.
2. `clean_empty_directories` prunes language directories that ended up empty (a `.pck` may declare languages it doesn't use).
3. FUSE callbacks (`fuseaw_init`/`getattr`/`readdir`/`read`/`readlink`/`open`) look up nodes in the tree; on `read` they `seekg` to the file's `offset + node->offset` and read `node->size` bytes from the single shared `std::ifstream` (guarded by `file_mutex`).
4. A synthetic `source.pck` symlink at the mount root resolves to the absolute path of the backing file (see `fuseaw.cpp:96` and `:127`).

**Virtual path layout** (from the comment at `fuseaw.cpp:24`):

```
/[sector]/[language]/[file_id].[ext]
```

- `sector` ∈ `banks`, `sounds`, `externals`.
- For `bnk` files, the file is a virtual directory containing one `0x<id>.wem` per embedded wem.
- File IDs and language IDs are printed in hex with `0x` prefix via `std::format("0x{:x}.{}", ...)`.

### Source layout (`src/`)

- `wavescan.{h,cpp}` — raw `.pck` reader. Defines the on-disk structs (`AKPKHeader`, `AKPKEntry`) and the three scan functions: `load_akpk_header`, `get_languages`, `get_sector`. Also handles Wwise bank-version sniffing (`get_sector` reads two bytes at `offset + 0x14` to pick a sound file extension — `wav`/`xma`/`ogg`) and the `alt_mode` (0x18-byte entry) layout used by externals.
- `bnk.{h,cpp}` — Wwise `.bnk` parser. Walks the `BKHD` / `DIDX` / `DATA` chunks and returns one `BNKFileMeta` per wem, with both the in-bank offset and the absolute `global_offset` into the parent `.pck`. Tolerates `HIRC` blocks appearing where `DIDX` was expected (the comment at `akpk.cpp:48` links to a reference implementation).
- `akpk.{h,cpp}` — orchestrator. `parse_akpk_file` chains the three sector scans in the fixed order (header → languages → banks → sounds → externals), then loops over the bank entries to populate `data.bnk_files` (a `map<file_id, vector<BNKFileMeta>>`).
- `filesystem.{h,cpp}` — builds the `AKPKFilesystemNode` tree consumed by FUSE. `construct_sector_filesystem` decides per-entry whether to recurse into a `bnk` (synthesizing a directory) or to materialize a leaf file. `clean_empty_directories` is run once after the full tree is built.
- `helpers.{h,cpp}` — `read_string_from_stream` for language names; auto-detects UTF-8 vs UTF-16LE by peeking the first two bytes, with optional forced encoding. Uses `std::expected` for error reporting.

### Key invariants

- All offsets into the `.pck` are computed lazily by `AKPKEntry::get_real_offset()` (`wavescan.h:37`) as `file_offset * (block_size == 0 ? 1 : block_size)`. The FUSE `read` path adds the per-file `offset` on top.
- `file_mutex` in `FSContext` serializes every read because the `std::ifstream` is shared across all FUSE worker threads. Reads clear eof/fail flags and reseek before each transfer.
- The mount is strictly read-only: `fuseaw_open` rejects any non-`O_RDONLY` flag with `EACCES` (`fuseaw.cpp:168`), and `fuseaw_oper` only wires up the read-side callbacks — there is no `write`, `create`, `mkdir`, `unlink`, or `rename`.

## Testing

There is no automated test harness. The `early_examples/` directory contains standalone CLI programs that exercise each layer end-to-end against a real `.pck` file and print what they find — use them as smoke tests when changing parser code. They are not pytest-style; each one is invoked with a file path and prints a human-readable report.

## VS Code configuration

`.vscode/c_cpp_properties.json` configures clang for IntelliSense with `${workspaceFolder}/**` on the include path. No launch/build tasks are defined — build with the `g++` commands above.
