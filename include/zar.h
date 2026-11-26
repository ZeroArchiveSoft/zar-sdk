#ifndef ZAR_H
#define ZAR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Error Codes
 * ========================================================================== */
#define ZAR_OK                  0
#define ZAR_ERR_INVALID_ARG    -1
#define ZAR_ERR_IO             -2
#define ZAR_ERR_FORMAT         -3
#define ZAR_ERR_COMPRESSION    -4
#define ZAR_ERR_CRYPTO         -5
#define ZAR_ERR_CHECKSUM       -6
#define ZAR_ERR_PATH_TRAVERSAL -7
#define ZAR_ERR_CANCELLED      -8
#define ZAR_ERR_INTERNAL       -99

/* ============================================================================
 * Compression Algorithms
 * ========================================================================== */
#define ZAR_ALGO_LZMA2          1
#define ZAR_ALGO_ZSTD           2

/* ============================================================================
 * Archive Compression Options
 * ========================================================================== */
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

/* ============================================================================
 * Progress & Cancellation Callback
 * ========================================================================== */

/**
 * Progress callback invoked during compression, extraction, or verification.
 * Return 0 to continue, or non-zero to cancel (operation returns ZAR_ERR_CANCELLED).
 */
typedef int (*ZarProgressCallback)(uint64_t processed_bytes, uint64_t total_bytes, void* user_data);

/**
 * Registers (or clears with NULL) a thread-local progress callback for the calling thread.
 */
void zar_set_progress_callback(ZarProgressCallback cb, void* user_data);

/* ============================================================================
 * Public C-ABI Functions
 * ========================================================================== */

/**
 * Compresses a file or directory into a .zar archive.
 *
 * @param input_path       UTF-8 path to source file or directory.
 * @param output_zar_path  UTF-8 path to destination .zar file.
 * @param options          Compression & encryption options.
 * @return ZAR_OK (0) on success, or a negative ZAR_ERR_* code on failure.
 */
int zar_compress(const char* input_path, const char* output_zar_path, ZarOptions options);

/**
 * Extracts a .zar archive into output_dir and verifies BLAKE3 integrity.
 *
 * @param input_zar_path   UTF-8 path to source .zar archive.
 * @param output_dir       UTF-8 path to destination directory.
 * @param password         UTF-8 password (NULL if archive is not encrypted).
 * @return ZAR_OK (0) on success, or a negative ZAR_ERR_* code on failure.
 */
int zar_extract(const char* input_zar_path, const char* output_dir, const char* password);

/**
 * Extracts a single file by relative archive path into output_dir.
 *
 * @param input_zar_path   UTF-8 path to source .zar archive.
 * @param entry_path       Relative UTF-8 path inside the archive (e.g. "sub/file.txt").
 * @param output_dir       UTF-8 path to destination directory.
 * @param password         UTF-8 password (NULL if archive is not encrypted).
 * @return ZAR_OK (0) on success, or a negative ZAR_ERR_* code on failure.
 */
int zar_extract_file(
    const char* input_zar_path,
    const char* entry_path,
    const char* output_dir,
    const char* password
);

/**
 * Verifies archive integrity (decryption tags + BLAKE3 checksums) without writing files.
 *
 * @param input_zar_path   UTF-8 path to source .zar archive.
 * @param password         UTF-8 password (NULL if archive is not encrypted).
 * @return ZAR_OK (0) on success, or a negative ZAR_ERR_* code on failure.
 */
int zar_verify(const char* input_zar_path, const char* password);

/**
 * Lists archive contents as a JSON string without decompressing the payload.
 * Caller MUST free the returned pointer using zar_free_string().
 *
 * @param input_zar_path   UTF-8 path to source .zar archive.
 * @param password         UTF-8 password (NULL if header is not encrypted).
 * @return Heap-allocated UTF-8 JSON string, or NULL on error.
 */
const char* zar_list_contents(const char* input_zar_path, const char* password);

/**
 * Frees a string returned by zar_list_contents(). Safe to call with NULL.
 */
void zar_free_string(const char* ptr);

/**
 * Returns a thread-local error message for the last failed call on the current thread,
 * or NULL if the last call succeeded. The returned pointer is valid until the next
 * zar_* call on the same thread; do NOT pass it to zar_free_string().
 */
const char* zar_last_error(void);

#ifdef __cplusplus
}
#endif

#endif /* ZAR_H */
