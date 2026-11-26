/**
 * libzar SDK - Node.js FFI integration example (using koffi).
 *
 * Install dependency:
 *   npm install koffi
 *
 * Run:
 *   node examples/node_test.js
 */

const fs = require("fs");
const os = require("os");
const path = require("path");
const koffi = require("koffi");

function getLibPath() {
  const sdkRoot = path.resolve(__dirname, "..");
  const plat = process.platform;
  const libName =
    plat === "win32"
      ? "libzar.dll"
      : plat === "darwin"
      ? "libzar.dylib"
      : "libzar.so";
  return path.join(sdkRoot, "lib", libName);
}

const ZarOptions = koffi.struct("ZarOptions", {
  algorithm: "int32",
  level: "int32",
  dict_size_mb: "uint32",
  solid_mode: "uint8",
  encrypt_header: "uint8",
  encrypt_payload: "uint8",
  _reserved: "uint8",
  solid_block_size_mb: "uint64",
  password: "str",
});

const lib = koffi.load(getLibPath());

const zar_compress = lib.func("int zar_compress(str input_path, str output_zar_path, ZarOptions options)");
const zar_extract = lib.func("int zar_extract(str input_zar_path, str output_dir, str password)");
const zar_list_contents = lib.func("void* zar_list_contents(str input_zar_path, str password)");
const zar_free_string = lib.func("void zar_free_string(void* ptr)");

function main() {
  const workDir = fs.mkdtempSync(path.join(os.tmpdir(), "zar_node_"));
  try {
    const srcDir = path.join(workDir, "input");
    fs.mkdirSync(srcDir, { recursive: true });
    fs.writeFileSync(path.join(srcDir, "sample.txt"), "Node.js .zar SDK test\n".repeat(100));

    const archivePath = path.join(workDir, "sample.zar");
    const extractDir = path.join(workDir, "out");
    const password = "node-secret-password";

    const rc = zar_compress(srcDir, archivePath, {
      algorithm: 1, // ZAR_ALGO_LZMA2
      level: 6,
      dict_size_mb: 16,
      solid_mode: 1,
      encrypt_header: 1,
      encrypt_payload: 1,
      _reserved: 0,
      solid_block_size_mb: 0,
      password,
    });
    if (rc !== 0) {
      throw new Error(`zar_compress failed with code ${rc}`);
    }

    const ptr = zar_list_contents(archivePath, password);
    if (!ptr) {
      throw new Error("zar_list_contents returned NULL");
    }
    try {
      const jsonStr = koffi.decode(ptr, "char", -1);
      console.log("Listing:", JSON.parse(jsonStr));
    } finally {
      zar_free_string(ptr);
    }

    const extRc = zar_extract(archivePath, extractDir, password);
    if (extRc !== 0) {
      throw new Error(`zar_extract failed with code ${extRc}`);
    }
    console.log("Extracted successfully to:", extractDir);
  } finally {
    fs.rmSync(workDir, { recursive: true, force: true });
  }
}

main();
