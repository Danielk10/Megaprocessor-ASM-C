#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

NUEVO_ZIP="${ROOT_DIR}/Nuevo.zip"
CPP_ZIP="${ROOT_DIR}/emsablador.zip"

usage() {
  cat <<'USAGE'
Uso:
  scripts/verify_migration_parity.sh [--nuevo-zip <ruta>] [--cpp-zip <ruta>]

Valida preparación de producción de la migración C++ -> C:
  1) Auditoría básica anti-rutas hardcodeadas del ensamblador C.
  2) Verificación oficial C++ vs referencia.
  3) Build del ensamblador C.
  4) Ensamblado C de todos los ASM y comparación HEX:
     - C vs referencia
     - C++ vs referencia
     - C vs C++
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --nuevo-zip)
      NUEVO_ZIP="$2"; shift 2 ;;
    --cpp-zip)
      CPP_ZIP="$2"; shift 2 ;;
    -h|--help)
      usage; exit 0 ;;
    *)
      echo "Argumento no reconocido: $1" >&2
      usage
      exit 2 ;;
  esac
done

if [[ ! -f "${NUEVO_ZIP}" || ! -f "${CPP_ZIP}" ]]; then
  echo "FAIL"
  echo "Se requieren los zips de entrada. nuevo='${NUEVO_ZIP}', cpp='${CPP_ZIP}'" >&2
  exit 1
fi

# 1) Auditoría anti-hardcode en código C del ensamblador.
if rg -n "app/src/main/cpp|emsablador|Nuevo\.zip|emsablador\.zip|/tmp/" "${ROOT_DIR}/src" "${ROOT_DIR}/include" "${ROOT_DIR}/CMakeLists.txt" >/dev/null; then
  echo "FAIL"
  echo "Se detectaron rutas hardcodeadas ajenas al ensamblador C en código de producción." >&2
  exit 1
fi

echo "[1/5] Auditoría anti-hardcode (C puro): PASS"

unzip -q "${CPP_ZIP}" -d "${WORK_DIR}"
unzip -q "${NUEVO_ZIP}" -d "${WORK_DIR}"

mkdir -p "${WORK_DIR}/emsablador/app/src/main"
cp -r "${WORK_DIR}/emsablador/cpp" "${WORK_DIR}/emsablador/app/src/main/cpp"
cp "${WORK_DIR}"/Nuevo/* "${WORK_DIR}/emsablador/"

# 2) Verificación oficial C++ vs referencia.
bash "${WORK_DIR}/emsablador/scripts/verify_hex_equivalence.sh" >/dev/null
echo "[2/5] C++ vs referencia: PASS"

# 3) Build ensamblador C.
cmake -S "${ROOT_DIR}" -B "${ROOT_DIR}/build" -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build "${ROOT_DIR}/build" -j4 >/dev/null
echo "[3/5] Build ensamblador C: PASS"

# 4) Ensamblado C con .hex + .lst para todos los ASM de prueba.
for asm in "${WORK_DIR}"/Nuevo/*.asm; do
  name="$(basename "${asm}" .asm)"
  "${ROOT_DIR}/build/megap-asm" "${asm}" --out "${WORK_DIR}/${name}.c.hex" --lst --lst-out "${WORK_DIR}/${name}.c.lst" >/dev/null
done
echo "[4/5] Ensamblado C (.hex/.lst): PASS"

# 5) Comparación C vs C++ vs referencia.
cat > "${WORK_DIR}/assembler_cli.cpp" <<'CPP'
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include "assembler.h"

static std::string readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("No se pudo abrir: " + path);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

int main(int argc, char** argv) {
    if (argc != 4) return 2;
    Assembler assembler;
    std::map<std::string, std::string> includes;
    includes["MEGAPROCESSOR_DEFS.ASM"] = readFile(argv[2]);
    assembler.setIncludeFiles(includes);

    const std::string result = assembler.assemble(readFile(argv[1]));
    if (result.rfind("ERROR:", 0) == 0) {
        std::cerr << result << "\n";
        return 1;
    }

    std::ofstream out(argv[3], std::ios::binary);
    out << result;
    return 0;
}
CPP

mkdir -p "${WORK_DIR}/android"
cat > "${WORK_DIR}/android/log.h" <<'H'
#pragma once
#define ANDROID_LOG_INFO 4
#define ANDROID_LOG_ERROR 6
inline int __android_log_print(int, const char*, const char*, ...) { return 0; }
H

g++ -std=c++17 -O2 \
  -I"${WORK_DIR}" \
  -I"${WORK_DIR}/emsablador/cpp" \
  "${WORK_DIR}/assembler_cli.cpp" \
  "${WORK_DIR}/emsablador/cpp/assembler.cpp" \
  "${WORK_DIR}/emsablador/cpp/utils.cpp" \
  -o "${WORK_DIR}/assembler_cli_cpp"

for asm in "${WORK_DIR}"/Nuevo/*.asm; do
  name="$(basename "${asm}" .asm)"
  "${WORK_DIR}/assembler_cli_cpp" "${asm}" "${WORK_DIR}/Nuevo/Megaprocessor_defs.asm" "${WORK_DIR}/${name}.cpp.hex"
done

python3 - "${WORK_DIR}" <<'PY'
import glob
import os
import sys

work = sys.argv[1]


def norm_hex(path):
    txt = open(path, encoding='utf-8', errors='ignore').read().replace('\r', '')
    return '\n'.join([ln.rstrip().upper() for ln in txt.split('\n') if ln.strip()])

ok = True
for asm in sorted(glob.glob(os.path.join(work, 'Nuevo', '*.asm'))):
    name = os.path.splitext(os.path.basename(asm))[0]
    ref = os.path.join(work, 'Nuevo', f'{name}.hex')
    c_hex = os.path.join(work, f'{name}.c.hex')
    cpp_hex = os.path.join(work, f'{name}.cpp.hex')

    c_vs_ref = norm_hex(c_hex) == norm_hex(ref)
    cpp_vs_ref = norm_hex(cpp_hex) == norm_hex(ref)
    c_vs_cpp = norm_hex(c_hex) == norm_hex(cpp_hex)

    print(f"{name}: C=REF={c_vs_ref} CPP=REF={cpp_vs_ref} C=CPP={c_vs_cpp}")
    if not (c_vs_ref and cpp_vs_ref and c_vs_cpp):
        ok = False

if not ok:
    raise SystemExit(1)
PY

echo "[5/5] C vs C++ vs referencia (.hex): PASS"
