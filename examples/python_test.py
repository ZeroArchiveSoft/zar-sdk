"""
libzar SDK - Python ctypes integration example.
Demonstrates compressing, listing contents (without full decompression),
and extracting a .zar archive with AES-256-GCM + Argon2id encryption and
BLAKE3 integrity verification.
"""

import ctypes
import json
import os
import platform
import shutil
import tempfile


def load_libzar(sdk_root: str) -> ctypes.CDLL:
    system = platform.system()
    if system == "Windows":
        lib_name = "libzar.dll"
    elif system == "Darwin":
        lib_name = "libzar.dylib"
    else:
        lib_name = "libzar.so"

    lib_path = os.path.join(sdk_root, "lib", lib_name)
    if not os.path.exists(lib_path):
        raise FileNotFoundError(f"Could not find {lib_path}. Run build_sdk first.")
    return ctypes.CDLL(lib_path)


class ZarOptions(ctypes.Structure):
    _fields_ = [
        ("algorithm", ctypes.c_int32),           # 1 = LZMA2, 2 = Zstd
        ("level", ctypes.c_int32),               # 1..9
        ("dict_size_mb", ctypes.c_uint32),       # 16..1024
        ("solid_mode", ctypes.c_uint8),          # 1 = enabled, 0 = disabled
        ("encrypt_header", ctypes.c_uint8),      # 1 = encrypted central directory
        ("encrypt_payload", ctypes.c_uint8),     # 1 = encrypted data blocks
        ("_reserved", ctypes.c_uint8),           # must be 0
        ("solid_block_size_mb", ctypes.c_uint64),# 0 = single continuous solid stream
        ("password", ctypes.c_char_p),           # NULL if no encryption
    ]


def configure_bindings(lib: ctypes.CDLL) -> None:
    lib.zar_compress.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ZarOptions]
    lib.zar_compress.restype = ctypes.c_int

    lib.zar_extract.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]
    lib.zar_extract.restype = ctypes.c_int

    lib.zar_list_contents.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
    lib.zar_list_contents.restype = ctypes.c_void_p

    lib.zar_free_string.argtypes = [ctypes.c_void_p]
    lib.zar_free_string.restype = None


def main() -> None:
    sdk_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    lib = load_libzar(sdk_root)
    configure_bindings(lib)

    work_dir = tempfile.mkdtemp(prefix="zar_py_example_")
    try:
        src_dir = os.path.join(work_dir, "input")
        os.makedirs(os.path.join(src_dir, "nested"), exist_ok=True)
        with open(os.path.join(src_dir, "hello.txt"), "wb") as f:
            f.write(b"Hello from .zar ZeroArchive!\n" * 200)
        with open(os.path.join(src_dir, "nested", "data.bin"), "wb") as f:
            f.write(b"Repeated solid block pattern 1234567890\n" * 500)

        archive_path = os.path.join(work_dir, "bundle.zar")
        extract_dir = os.path.join(work_dir, "extracted")
        password = b"correct-horse-battery-staple"

        opts = ZarOptions(
            algorithm=2,  # ZAR_ALGO_ZSTD (use 1 for LZMA2)
            level=6,
            dict_size_mb=16,
            solid_mode=1,
            encrypt_header=1,
            encrypt_payload=1,
            _reserved=0,
            solid_block_size_mb=0,
            password=password,
        )

        rc = lib.zar_compress(src_dir.encode("utf-8"), archive_path.encode("utf-8"), opts)
        assert rc == 0, f"zar_compress failed with code {rc}"
        print(f"Compressed archive created: {archive_path} ({os.path.getsize(archive_path)} bytes)")

        ptr = lib.zar_list_contents(archive_path.encode("utf-8"), password)
        assert ptr, "zar_list_contents returned NULL"
        try:
            listing_json = ctypes.cast(ptr, ctypes.c_char_p).value.decode("utf-8")
            listing = json.loads(listing_json)
            print("Archive listing:", json.dumps(listing, indent=2))
        finally:
            lib.zar_free_string(ptr)

        rc = lib.zar_extract(archive_path.encode("utf-8"), extract_dir.encode("utf-8"), password)
        assert rc == 0, f"zar_extract failed with code {rc}"

        with open(os.path.join(extract_dir, "hello.txt"), "rb") as f:
            assert f.read() == b"Hello from .zar ZeroArchive!\n" * 200
        with open(os.path.join(extract_dir, "nested", "data.bin"), "rb") as f:
            assert f.read() == b"Repeated solid block pattern 1234567890\n" * 500

        print("Extraction + BLAKE3 verification succeeded!")
    finally:
        shutil.rmtree(work_dir, ignore_errors=True)


if __name__ == "__main__":
    main()
