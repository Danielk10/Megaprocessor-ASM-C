# Megaprocessor ASM en C (CLI)

Ensamblador del **Megaprocessor** migrado a **C puro (C99)**, orientado a uso por línea de comandos y con build multiplataforma.

- Lenguaje: C99
- Build: CMake
- Salida: Intel HEX (`.hex`) y listado (`.lst`)
- Plataformas objetivo: Linux, Windows, macOS (y core reutilizable para Android NDK)

## Estado actual

El ensamblador en C implementa el flujo principal del ensamblador C++ de referencia:

- Preprocesado de `INCLUDE` (incluyendo includes recursivos)
- Ensamblado en dos pasadas (resolución de símbolos + generación de código)
- Evaluación de expresiones (`+ - * / << >>`, `()`, `$`, literales y símbolos)
- Directivas (`ORG`, `EQU`, `DB`, `DW`, `DL`, `DM`, `DS`)
- Codificación de instrucciones del set utilizado por los casos de verificación
- Generación de Intel HEX y listing (`.lst`)

## Requisitos

- CMake 3.10+
- Compilador C compatible con C99
  - Linux: GCC o Clang
  - macOS: Apple Clang (Xcode Command Line Tools)
  - Windows: MSVC o MinGW-w64

## Compilar (CLI)

### Linux

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Binario generado:

```bash
./build/megap-asm
```

### macOS

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Binario generado:

```bash
./build/megap-asm
```

### Windows (Developer Command Prompt - MSVC)

```bat
cmake -S . -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Binario generado:

```bat
build\megap-asm.exe
```

### Windows (MSYS2/MinGW)

```bash
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Binario generado:

```bash
./build/megap-asm.exe
```

## Instalación del CLI

Se agregó target de instalación en CMake.

### Linux/macOS

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
sudo cmake --install build --prefix /usr/local
```

Después:

```bash
megap-asm --help
```

> Nota: actualmente el CLI no imprime ayuda extensa; sin argumentos muestra uso.

### Windows

Instalación en carpeta local:

```bat
cmake -S . -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build --prefix C:\tools\megap-asm
```

Agrega `C:\tools\megap-asm\bin` al `PATH` para invocarlo globalmente.

## Uso

```bash
megap-asm programa.asm
```

Genera por defecto:

- `programa.hex`

Opciones:

```bash
megap-asm programa.asm --out salida.hex
megap-asm programa.asm --lst
megap-asm programa.asm --lst --lst-out salida.lst
```

## Includes

El CLI intenta cargar:

1. `Megaprocessor_defs.asm` (si existe)
2. Includes declarados en el `.asm`
3. Resolución por carpeta del asm, subcarpeta `includes/`, y ruta directa del nombre

## Estructura actual relevante

```text
include/megap_asm.h   API pública del ensamblador en C
src/assembler.c       Core del ensamblador (2-pass + HEX + LST)
src/main.c            CLI y carga de archivos/includes
CMakeLists.txt        Build e instalación
```

## Verificación recomendada

1. Compilar el proyecto.
2. Ejecutar ensamblado sobre casos `.asm`.
3. Comparar los `.hex` contra referencia y contra salida del ensamblador C++.

## Licencia

Apache-2.0. Ver `LICENSE`.
