# Megaprocessor ASM en C (CLI)

Ensamblador del **Megaprocessor** migrado a **C puro (C99)**, orientado a uso por línea de comandos y con build multiplataforma.

- Lenguaje: C99
- Build: CMake
- Salida: Intel HEX (`.hex`) y listado (`.lst`)
- Plataformas objetivo: Linux, Windows, macOS

## Estado del proyecto (listo para producción)

El ensamblador en C está alineado con la lógica del ensamblador C++ de referencia para los casos de verificación incluidos:

- Preprocesado de `INCLUDE` (incluyendo includes recursivos)
- Ensamblado en dos pasadas (resolución de símbolos + generación de código)
- Evaluación de expresiones (`+ - * / << >>`, `()`, `$`, literales y símbolos)
- Directivas (`ORG`, `EQU`, `DB`, `DW`, `DL`, `DM`, `DS`)
- Generación de Intel HEX y listing (`.lst`)

Además, el repositorio incluye un script de verificación end-to-end para validar:

1. C++ vs HEX de referencia
2. C vs HEX de referencia
3. C vs C++

Ver sección **Verificación de paridad (C / C++ / referencia)**.

## Requisitos

- CMake 3.10+
- Compilador C compatible con C99
  - Linux: GCC o Clang
  - macOS: Apple Clang (Xcode Command Line Tools)
  - Windows: MSVC o MinGW-w64

---

## Compilación del ensamblador C

### Linux

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Binario:

```bash
./build/megap-asm
```

### macOS

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Binario:

```bash
./build/megap-asm
```

### Windows (Developer Command Prompt - MSVC)

```bat
cmake -S . -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Binario:

```bat
build\megap-asm.exe
```

### Windows (MSYS2/MinGW)

```bash
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Binario:

```bash
./build/megap-asm.exe
```

---

## Uso (ensamblado)

### Salida `.hex`

```bash
megap-asm programa.asm
```

Por defecto genera `programa.hex`.

### Salida `.hex` + `.lst`

```bash
megap-asm programa.asm --out salida.hex --lst --lst-out salida.lst
```

Opciones principales:

```bash
megap-asm programa.asm --out salida.hex
megap-asm programa.asm --lst
megap-asm programa.asm --lst --lst-out salida.lst
```

---

## Includes (`INCLUDE`) y ensamblado de archivos con dependencias

Si un `.asm` usa `INCLUDE`, **debes tener disponibles también los archivos `.asm` incluidos**.

El CLI resuelve includes en este orden:

1. `Megaprocessor_defs.asm` (si existe)
2. Includes declarados en el `.asm`
3. Búsqueda en:
   - carpeta del `.asm` principal,
   - subcarpeta `includes/`,
   - ruta directa del nombre de include.

### Recomendación de estructura

```text
proyecto/
  main.asm
  Megaprocessor_defs.asm
  includes/
    macros.asm
    constantes.asm
```

### Ejemplo

```bash
./build/megap-asm ./proyecto/main.asm --out ./proyecto/main.hex --lst --lst-out ./proyecto/main.lst
```

---

## Verificación de paridad (C / C++ / referencia)

El repositorio incluye:

- `Nuevo.zip`: ASM + HEX/LST de referencia
- `emsablador.zip`: código/scritps del ensamblador C++ de referencia
- `scripts/verify_migration_parity.sh`: flujo de verificación completo

Ejecuta:

```bash
scripts/verify_migration_parity.sh
```

Opcionalmente puedes pasar rutas explícitas:

```bash
scripts/verify_migration_parity.sh --nuevo-zip ./Nuevo.zip --cpp-zip ./emsablador.zip
```

Este script hace:

1. Auditoría básica anti-hardcode en el ensamblador C (sin rutas acopladas a C++/artefactos temporales).
2. Ejecuta verificación oficial C++ (`verify_hex_equivalence.sh`) contra referencia.
3. Compila el ensamblador C de este repo.
4. Ensambla todos los `.asm` de `Nuevo.zip` con el ensamblador C generando `.hex` y `.lst`.
5. Compila un CLI C++ local desde `emsablador/cpp` y compara `.hex`:
   - C vs referencia
   - C++ vs referencia
   - C vs C++

Si hay una divergencia, termina con error y muestra el caso.

---

## Instalación opcional del CLI

### Linux/macOS

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
sudo cmake --install build --prefix /usr/local
```

### Windows

```bat
cmake -S . -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build --prefix C:\tools\megap-asm
```

Agrega `C:\tools\megap-asm\bin` al `PATH`.

---

## Estructura relevante

```text
include/megap_asm.h                 API pública del ensamblador en C
src/assembler.c                      Core del ensamblador (2-pass + HEX + LST)
src/main.c                           CLI y carga de archivos/includes
scripts/verify_migration_parity.sh   Verificación C vs C++ vs referencia
CMakeLists.txt                       Build e instalación
```

## Licencia

Apache-2.0. Ver `LICENSE`.
