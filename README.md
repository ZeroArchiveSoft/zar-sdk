<div align="center">

<img src="assets/zar_logo.png" alt="ZeroArchive (.zar)" width="136" />

# ZeroArchive SDK (`libzar`)

**High-density, solid-stream, cryptographically verified `.zar` archive engine — delivered as a lightweight C99 shared library.**

[![Release](https://img.shields.io/badge/release-v0.1.0-0284C7?style=for-the-badge)](#)
[![ABI](https://img.shields.io/badge/ABI-C99%20Shared%20Library-4F46E5?style=for-the-badge)](#api-reference)
[![License](https://img.shields.io/badge/license-SDK%20Binary-0F172A?style=for-the-badge)](LICENSE)

[Why `libzar`?](#why-libzar) • [Quick Start](#quick-start) • [API Reference](#api-reference) • [Security](#security)

</div>

---

## Why `libzar`?

- **Maximum Compression or Speed:** Switch seamlessly between **LZMA2** (`16 MB – 1024 MB` dictionary) for archival density and multi-threaded **Zstd** for blazing-fast throughput.
- **Auto-Parallel Solid Streams:** Groups files into multi-threaded solid blocks (auto-balanced 64 MB blocks by default) for linear multi-core scaling while eliminating cross-file redundancy.
- **Instant Metadata Listing:** Inspect archive contents, sizes, timestamps, and hashes in milliseconds via `zar_list_contents` without decompressing the payload.
- **Built-In Security & Integrity:** End-to-end **AES-256-GCM** + **Argon2id** encryption for headers and payloads, **BLAKE3** verification on every extracted file, and path-traversal protection.
- **Zero-Boilerplate C99 ABI:** Drop `libzar.dll` / `libzar.so` / `libzar.dylib` and `include/zar.h` into **C/C++**, **Python**, **Node.js**, **Go**, or **C#**.

---

## Quick Start

### 1. Download / Clone

```bash
git clone https://github.com/ZeroArchiveSoft/zar-sdk.git
cd zar-sdk
```

### 2. Run in 10 Seconds

<table>
<tr>
<th>Python (<code>ctypes</code>) — Zero Dependencies</th>
<th>Node.js (<code>koffi</code>)</th>
</tr>
<tr>
<td>

```bash
python examples/python_test.py
```

</td>
<td>

```bash
npm install koffi
node examples/node_test.js
```

</td>
</tr>
</table>

### C / C++ Integration Example

```c
#include <stdio.h>
#include "zar.h"

int main(void) {
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

    if (zar_compress("./my_folder", "./archive.zar", opts) != ZAR_OK) {
        fprintf(stderr, "Compress failed: %s\n", zar_last_error());
        return 1;
    }

    const char* json = zar_list_contents("./archive.zar", "my-strong-password");
    if (json) {
        printf("%s\n", json);
        zar_free_string(json);
    }

    if (zar_verify("./archive.zar", "my-strong-password") != ZAR_OK) {
        fprintf(stderr, "Verify failed: %s\n", zar_last_error());
        return 1;
    }

    return zar_extract("./archive.zar", "./restored", "my-strong-password");
}
```

**Build (Linux / macOS):**
```bash
cc main.c -I./include -L./lib -lzar -Wl,-rpath,'$ORIGIN/lib' -o zar_app
```

---

## API Reference

All symbols are declared in [`include/zar.h`](include/zar.h).

### Functions at a Glance

| Function | Purpose |
| :--- | :--- |
| `zar_compress(input, output, options)` | Compresses a file or directory recursively into a `.zar` archive. |
| `zar_extract(archive, out_dir, password)` | Extracts all files & empty directories, restoring timestamps and verifying BLAKE3. |
| `zar_extract_file(archive, entry, out_dir, password)` | Extracts a single file by relative path (decompresses only its block). |
| `zar_verify(archive, password)` | Verifies decryption tags & BLAKE3 checksums in-memory (no disk writes). |
| `zar_list_contents(archive, password)` | Returns heap-allocated JSON metadata without decompressing the payload. |
| `zar_free_string(ptr)` | Frees a string returned by `zar_list_contents` (safe with `NULL`). |
| `zar_last_error()` | Returns thread-local error message for the last failed call (do not free). |
| `zar_set_progress_callback(cb, user_data)` | Registers a thread-local progress & cancellation callback. |

### `ZarOptions` Struct

```c
typedef struct ZarOptions {
    int32_t     algorithm;            /* 0 = default (ZSTD), ZAR_ALGO_LZMA2 (1), ZAR_ALGO_ZSTD (2) */
    int32_t     level;                /* Compression level: 1..9 (0 = default 5) */
    uint32_t    dict_size_mb;         /* Dictionary size in MB: 16..1024 (0 = default 16) */
    uint8_t     solid_mode;           /* 1 = solid compression, 0 = per-file */
    uint8_t     encrypt_header;       /* 1 = encrypt central directory, 0 = plaintext */
    uint8_t     encrypt_payload;      /* 1 = encrypt data blocks, 0 = plaintext */
    uint8_t     _reserved;            /* Must be 0 */
    uint64_t    solid_block_size_mb;  /* Max MB per solid block (0 = default/auto: 64MB parallel blocks) */
    const char* password;             /* UTF-8 password (NULL if unencrypted) */
} ZarOptions;
```

### Error Codes (`ZAR_OK` / `ZAR_ERR_*`)

| Code | Constant | Meaning |
| :---: | :--- | :--- |
| `0` | `ZAR_OK` | Success |
| `-1` | `ZAR_ERR_INVALID_ARG` | Null pointer, invalid UTF-8, or out-of-range option |
| `-2` | `ZAR_ERR_IO` | Filesystem read/write failure |
| `-3` | `ZAR_ERR_FORMAT` | Invalid `.zar` magic bytes, version, or corrupt directory |
| `-4` | `ZAR_ERR_COMPRESSION` | Compression or decompression stream failure |
| `-5` | `ZAR_ERR_CRYPTO` | Missing/wrong password or authentication failure |
| `-6` | `ZAR_ERR_CHECKSUM` | BLAKE3 integrity verification failed |
| `-7` | `ZAR_ERR_PATH_TRAVERSAL` | Unsafe path rejected during extraction |
| `-8` | `ZAR_ERR_CANCELLED` | Cancelled by caller via progress callback |
| `-99` | `ZAR_ERR_INTERNAL` | Unexpected internal error |

---

## Security

- **Authenticated Encryption:** **Argon2id** key derivation paired with **AES-256-GCM** protects both headers and data payloads against unauthorized access and tampering.
- **Memory & ABI Safety:** Cryptographic keys are cleared from memory after use, and all C-ABI entrypoints isolate errors cleanly without unwinding across the ABI boundary.
- **Thread Safety & Encoding:** Stateless and thread-safe across distinct archive paths. All path and password arguments must be valid null-terminated **UTF-8**.

---

## Platform Distribution

| OS | Binary | Loader Configuration |
| :--- | :--- | :--- |
| **Windows (x64)** | `lib/libzar.dll` | Place beside executable or add to `PATH` |
| **Linux (x64 / arm64)** | `lib/libzar.so` | `LD_LIBRARY_PATH` or `-Wl,-rpath` |
| **macOS (Universal / arm64)** | `lib/libzar.dylib` | `DYLD_LIBRARY_PATH` or `@rpath` |

---

<div align="center">
  <sub>Licensed under the <a href="LICENSE">libzar Public SDK License</a> · © 2026 ZeroArchive</sub>
</div>
