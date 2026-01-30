# Megaprocessor ASM - Ensamblador en C

Ensamblador para el Megaprocessor escrito en C puro.

## Compilación en Linux

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Uso

```bash
./megap-asm archivo.asm
```

Esto genera `archivo.asm.bin` con el código máquina.

## Estructura del Proyecto

- `src/` - Código fuente principal
  - `main.c` - Punto de entrada
  - `lexer.c` - Análisis léxico
  - `parser.c` - Análisis sintáctico
  - `codegen.c` - Generación de código
- `include/` - Archivos de cabecera
  - `megap_asm.h` - Definiciones principales
- `CMakeLists.txt` - Configuración de CMake

## Compilador Recomendado

- **Linux**: GCC (gcc)
- **Build system**: CMake 3.10+

## TODO

- [ ] Completar implementación del lexer
- [ ] Implementar parser completo
- [ ] Implementar generador de código para instrucciones del Megaprocessor
- [ ] Añadir manejo de etiquetas y símbolos
- [ ] Implementar primera y segunda pasada
- [ ] Añadir pruebas unitarias
