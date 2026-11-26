<div align="center">
  <img src="assets/zar_logo.svg" alt="ZeroArchive (.zar) Logo" width="136" />

  <h1>ZeroArchive (<code>libzar</code>) Public SDK</h1>

  <p>
    <strong>High-Compression, Solid-Block, Cryptographically Verified Archive Engine</strong><br/>
    Cross-platform C99 Shared Library (<code>libzar.dll</code> · <code>libzar.so</code> · <code>libzar.dylib</code>)
  </p>

  <p>
    <img src="https://img.shields.io/badge/SDK-v0.1.0-0EA5E9?style=for-the-badge" alt="SDK v0.1.0" />
    <img src="https://img.shields.io/badge/ABI-C99%20Compatible-6366F1?style=for-the-badge" alt="C99 ABI" />
    <img src="https://img.shields.io/badge/Platforms-Windows%20%7C%20Linux%20%7C%20macOS-10B981?style=for-the-badge" alt="Platforms" />
    <img src="https://img.shields.io/badge/Crypto-AES--256--GCM%20%2B%20Argon2id%20%2B%20BLAKE3-F59E0B?style=for-the-badge" alt="Cryptography" />
  </p>

  <p>
    <a href="#key-capabilities">Capabilities</a> •
    <a href="#quick-start">Quick Start</a> •
    <a href="#api-reference">API Reference</a> •
    <a href="#benchmark-reference">Benchmarks</a> •
    <a href="#security-integrity--thread-safety">Security</a> •
    <a href="#license">License</a>
  </p>
</div>

---

## Overview

**`libzar`** is the official public C-ABI SDK for creating, inspecting, verifying, and extracting **`.zar` (ZeroArchive)** files.

Built for software installers, game asset pipelines, backup utilities, and enterprise data distribution, `libzar` exposes a minimal, memory-safe **C99 interface** (`include/zar.h`) that integrates cleanly with **C / C++**, **Python (`ctypes`)**, **Node.js (`koffi`)**, **C# (P/Invoke)**, **Go (`cgo`)**, and any language capable of loading a native shared library.

```text
┌─────────────────────────────┐      C99 ABI       ┌──────────────────────────┐
│  Your Application           │ ─────────────────► │  libzar Shared Library   │
│  C/C++ · Python · Node · Go │   include/zar.h    │  .dll / .so / .dylib     │
└─────────────────────────────┘                    └────────────┬─────────────┘
                                                                │
                                                                ▼
                                                   ┌──────────────────────────┐
                                                   │  .zar Archive Container  │
                                                   │  Header · Solid Blocks   │
                                                   │  Encrypted Directory     │
                                                   └──────────────────────────┘
```

---

## Key Capabilities

| Capability | Description |
| :--- | :--- |
| **Dual Compression Engines** | **LZMA2** (configurable `16 MB – 1024 MB` dictionary for maximum density) and **Zstd** (high-speed multi-threaded compression). |
| **Parallel Solid Compression** | Groups files into continuous solid streams with configurable block-size splitting (`solid_block_size_mb`) for multi-core throughput and superior cross-file ratios. |
| **Zero-Decompression Listing** | Reads the archive footer directly so `zar_list_contents()` returns full metadata JSON in milliseconds without decompressing the payload. |
| **Selective File Extraction** | `zar_extract_file()` extracts a single entry by relative path—decompressing only the block that contains it. |
| **Authenticated Encryption** | Optional independent **AES-256-GCM** encryption for both the Central Directory (`encrypt_header`) and data payload (`encrypt_payload`) using **Argon2id** key derivation. |
| **BLAKE3 Integrity & Verify** | Every extracted file is verified against its **BLAKE3** cryptographic digest; `zar_verify()` validates the entire archive in-memory without touching disk. |
| **Live Progress & Cancellation** | Thread-local `zar_set_progress_callback()` reports processed/total bytes and supports clean cancellation (`ZAR_ERR_CANCELLED`). |
| **Hardened Extraction** | Rejects absolute paths, drive prefixes, and `..` traversal sequences; restores file modification timestamps (`mtime`) and Unix permissions. |

---

## SDK Package Layout

```text
libzar-sdk/
├── assets/
│   ├── zar_logo.svg         # Vector .zar brand emblem
│   ├── zar_logo.png         # 512x512 transparent PNG icon
│   └── zar_logo.ico         # Multi-size Windows icon (16..256 px)
├── include/
│   └── zar.h                # Public C99 / C++ compatible header
├── lib/
│   └── libzar.dll           # Stripped native shared library (or libzar.so / libzar.dylib)
├── examples/
│   ├── python_test.py       # Zero-dependency Python ctypes integration example
│   └── node_test.js         # Node.js koffi FFI integration example
├── .gitignore
├── LICENSE                  # SDK binary redistribution license
└── README.md                # This documentation
```

---

## Quick Start

### C / C++

```c
#include <stdio.h>
#include "zar.h"

static int on_progress(uint64_t done, uint64_t total, void* user_data) {
    (void)user_data;
    if (total > 0) {
        printf("\rProgress: %llu%%", (unsigned long long)((done * 100) / total));
        fflush(stdout);
    }
    return 0; /* Return non-zero to cancel */
}

int main(void) {
    zar_set_progress_callback(on_progress, NULL);

    ZarOptions opts = {
        .algorithm           = ZAR_ALGO_ZSTD,
        .level               = 6,
        .dict_size_mb        = 16,
        .solid_mode          = 1,
        .encrypt_header      = 1,
        .encrypt_payload     = 1,
        ._reserved           = 0,
        .solid_block_size_mb = 64,
        .password            = "my-strong-password"
    };

    /* 1. Compress */
    int rc = zar_compress("./my_folder", "./archive.zar", opts);
    if (rc != ZAR_OK) {
        fprintf(stderr, "\nCompress failed (%d): %s\n", rc, zar_last_error());
        return 1;
    }
    printf("\n");

    /* 2. List Contents (without decompressing payload) */
    const char* json = zar_list_contents("./archive.zar", "my-strong-password");
    if (json != NULL) {
        printf("Archive JSON:\n%s\n", json);
        zar_free_string(json);
    }

    /* 3. Verify Integrity */
    rc = zar_verify("./archive.zar", "my-strong-password");
    if (rc != ZAR_OK) {
        fprintf(stderr, "Verify failed (%d): %s\n", rc, zar_last_error());
        return 1;
    }

    /* 4. Extract All (or use zar_extract_file for a single file) */
    rc = zar_extract("./archive.zar", "./restored", "my-strong-password");
    if (rc != ZAR_OK) {
        fprintf(stderr, "Extract failed (%d): %s\n", rc, zar_last_error());
        return 1;
    }

    zar_set_progress_callback(NULL, NULL);
    return 0;
}
```

**Compile & Link:**
- **Linux / macOS:**
  ```bash
  cc main.c -I./include -L./lib -lzar -Wl,-rpath,'$ORIGIN/lib' -o zar_app
  ```
- **Windows (MinGW / Clang):**
  ```powershell
  gcc main.c -I.\include -L.\lib -lzar -o zar_app.exe
  ```
  *(Ensure `lib/libzar.dll` is in `PATH` or alongside `zar_app.exe`.)*

---

### Python (`ctypes`)

No external packages required—uses Python's built-in `ctypes`:

```bash
python examples/python_test.py
```

---

### Node.js (`koffi`)

Uses [`koffi`](https://www.npmjs.com/package/koffi) for zero-compilation native FFI:

```bash
npm install koffi
node examples/node_test.js
```

---

## API Reference

### `ZarOptions` Struct

Defined in [`include/zar.h`](include/zar.h):

```c
typedef struct ZarOptions {
    int32_t     algorithm;            /* ZAR_ALGO_LZMA2 (1) or ZAR_ALGO_ZSTD (2) */
    int32_t     level;                /* Compression level: 1..9 (0 = default 6) */
    uint32_t    dict_size_mb;         /* Dictionary size in MB: 16..1024 (0 = default 16) */
    uint8_t     solid_mode;           /* 1 = enable solid compression, 0 = per-file */
    uint8_t     encrypt_header;       /* 1 = encrypt central directory, 0 = plaintext */
    uint8_t     encrypt_payload;      /* 1 = encrypt data blocks, 0 = plaintext */
    uint8_t     _reserved;            /* Must be 0 */
    uint64_t    solid_block_size_mb;  /* Max MB per solid block (0 = single continuous block) */
    const char* password;             /* UTF-8 password (NULL if encryption disabled) */
} ZarOptions;
```

| Field | Type | Default (`0`) | Description & Tuning |
| :--- | :--- | :---: | :--- |
| `algorithm` | `int32_t` | — | `ZAR_ALGO_LZMA2` (`1`) for highest compression ratio; `ZAR_ALGO_ZSTD` (`2`) for high-speed throughput. |
| `level` | `int32_t` | `6` | `1` (fastest) to `9` (maximum compression). |
| `dict_size_mb` | `uint32_t` | `16` | Dictionary window in MiB (`16` to `1024`). Primarily tunes LZMA2 ratio/memory. |
| `solid_mode` | `uint8_t` | `0` | `1` to concatenate files into solid blocks; `0` to compress each file independently. |
| `encrypt_header` | `uint8_t` | `0` | `1` to encrypt the Central Directory (hides filenames, sizes, and hashes). |
| `encrypt_payload` | `uint8_t` | `0` | `1` to encrypt compressed data blocks with AES-256-GCM. |
| `_reserved` | `uint8_t` | `0` | Reserved ABI alignment byte. Always set to `0`. |
| `solid_block_size_mb` | `uint64_t` | `0` | Max uncompressed MiB per solid block. `0` = single stream (best ratio); `64`–`128` = multi-core parallel speed. |
| `password` | `const char*` | `NULL` | Null-terminated UTF-8 password. Required when `encrypt_header` or `encrypt_payload` is `1`. |

---

### Functions

| Function | Signature | Notes |
| :--- | :--- | :--- |
| **Compress** | `int zar_compress(const char* input_path, const char* output_zar_path, ZarOptions options)` | Recursively archives a file or directory. Preserves empty directories. Returns `ZAR_OK` (`0`) or `ZAR_ERR_*`. |
| **Extract All** | `int zar_extract(const char* input_zar_path, const char* output_dir, const char* password)` | Extracts all files & empty dirs, restores `mtime`/permissions, and verifies **BLAKE3**. |
| **Extract Single** | `int zar_extract_file(const char* input_zar_path, const char* entry_path, const char* output_dir, const char* password)` | Extracts one file by relative path, decompressing only its containing block. |
| **Verify** | `int zar_verify(const char* input_zar_path, const char* password)` | Validates decryption tags and **BLAKE3** digests in-memory without writing to disk. |
| **List Contents** | `const char* zar_list_contents(const char* input_zar_path, const char* password)` | Returns heap-allocated UTF-8 JSON (or `NULL` on error). Free with `zar_free_string()`. |
| **Free String** | `void zar_free_string(const char* ptr)` | Frees strings returned by `zar_list_contents()`. Safe with `NULL`. |
| **Last Error** | `const char* zar_last_error(void)` | Thread-local error string for the last failed call (`NULL` if succeeded). Do **not** free. |
| **Progress Hook** | `void zar_set_progress_callback(ZarProgressCallback cb, void* user_data)` | Registers/clears (`NULL`) a thread-local progress & cancel callback. |

---

### Error Codes

| Code | Constant | Description |
| :---: | :--- | :--- |
| `0` | `ZAR_OK` | Operation completed successfully. |
| `-1` | `ZAR_ERR_INVALID_ARG` | Null pointer, invalid UTF-8 string, missing entry, or out-of-range option. |
| `-2` | `ZAR_ERR_IO` | Filesystem error (missing input path, permission denied, disk full). |
| `-3` | `ZAR_ERR_FORMAT` | Invalid `.zar` magic header, unsupported version, or corrupt directory. |
| `-4` | `ZAR_ERR_COMPRESSION` | Compression or decompression stream failure. |
| `-5` | `ZAR_ERR_CRYPTO` | Missing/empty password, wrong password, or authentication failure. |
| `-6` | `ZAR_ERR_CHECKSUM` | Extracted file failed BLAKE3 integrity verification. |
| `-7` | `ZAR_ERR_PATH_TRAVERSAL` | Archive entry contained an unsafe path (e.g., `..` or absolute root). |
| `-8` | `ZAR_ERR_CANCELLED` | Operation cancelled by caller via progress callback. |
| `-99` | `ZAR_ERR_INTERNAL` | Unexpected internal error caught safely at the ABI boundary. |

---

## JSON Output Schema (`zar_list_contents`)

```json
{
  "version": 1,
  "algorithm": "zstd",
  "level": 6,
  "dict_size_mb": 16,
  "solid_mode": true,
  "encrypted_header": true,
  "encrypted_payload": true,
  "file_count": 2,
  "empty_dirs": [],
  "files": [
    {
      "path": "hello.txt",
      "original_size": 5800,
      "compressed_offset": 32,
      "block_index": 0,
      "blake3": "b1665215f0cb24c21c41c5b5ad5738acafb7ec8929433ea0db5f1dbe292105f6",
      "modified_time": 1795696601,
      "created_time": 1795696601
    }
  ]
}
```

---

## Benchmark Reference

Observed on a real-world mixed binary/application dataset (`987` files, `1,051.20 MiB` uncompressed):

| Profile | Compressed Size | Space Saved | Compress Time | Compress Speed | Verify Time (`zar_verify`) |
| :--- | :---: | :---: | :---: | :---: | :---: |
| **Zstd Level 3** (`solid_block_size_mb = 64`) | `914.14 MiB` | `13.04%` | `8.55 s` | `122.95 MiB/s` | `0.27 s` (`~3.8 GiB/s`) |
| **Zstd Level 6** (`solid_block_size_mb = 0`) | `909.61 MiB` | `13.47%` | `18.37 s` | `57.22 MiB/s` | `0.89 s` (`1.15 GiB/s`) |
| **LZMA2 Level 3, 32 MB Dict** (`block = 64 MB`) | `869.77 MiB` | `17.26%` | `58.89 s` | `17.85 MiB/s` | `1.84 s` (`571 MiB/s`) |
| **LZMA2 Level 3, 32 MB Dict** (`block = 0`) | `853.31 MiB` | `18.82%` | `351.36 s` | `2.99 MiB/s` | `9.94 s` (`105.75 MiB/s`) |

---

## Security, Integrity & Thread Safety

- **Key Derivation & Cipher:** Uses **Argon2id** password-based key derivation and **AES-256-GCM** authenticated encryption to protect both archive headers and data payloads against unauthorized access and tampering.
- **Memory & ABI Safety:** Derived cryptographic key material is cleared from memory after use, and all C-ABI entrypoints return opaque error codes without unwinding across the ABI boundary.
- **Thread Safety:** All functions in `libzar` are thread-safe across distinct archive files. Progress callbacks and `zar_last_error()` are scoped per calling thread. Avoid concurrently writing to and reading from the same `.zar` file path.
- **Path Encoding:** All string arguments (`input_path`, `output_zar_path`, `output_dir`, `entry_path`, `password`) must be valid **UTF-8** null-terminated strings.

---

## Platform Distribution Matrix

| Operating System | Shared Library | Dynamic Loader Configuration |
| :--- | :--- | :--- |
| **Windows (x64)** | `lib/libzar.dll` | Place beside executable or add `lib/` to `PATH` |
| **Linux (x64 / arm64)** | `lib/libzar.so` | Set `LD_LIBRARY_PATH` or link with `-Wl,-rpath,'$ORIGIN/lib'` |
| **macOS (Universal / arm64)** | `lib/libzar.dylib` | Set `DYLD_LIBRARY_PATH` or embed `@rpath` |

---

## License

The compiled `libzar` binary is distributed under the terms of [`LICENSE`](LICENSE) (free to integrate and redistribute; reverse-engineering prohibited). The public header (`include/zar.h`) and examples (`examples/*`) may be freely used in your projects.
