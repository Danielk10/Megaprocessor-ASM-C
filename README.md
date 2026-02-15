# Megaprocessor ASM en C (CLI)

**Ensamblador de código abierto para el Megaprocessor - Versión C Puro (CLI)**

[![Licencia](https://img.shields.io/badge/Licencia-Apache%202.0-blue.svg)](LICENSE)
[![Lenguaje](https://img.shields.io/badge/Lenguaje-C99-00599C.svg)](https://en.wikipedia.org/wiki/C99)
[![Build System](https://img.shields.io/badge/Build-CMake-064F8C.svg)](https://cmake.org/)

## 📋 Descripción

Este proyecto es un **ensamblador completo** para el [Megaprocessor](http://www.megaprocessor.com/), un procesador de 16 bits construido con componentes discretos creado por James Newman. El Megaprocessor es un procesador físico gigante donde cada transistor es visible, diseñado con fines educativos para mostrar cómo funciona un CPU por dentro.

Esta implementación está escrita en **C puro (C99)** y está diseñada para uso por **línea de comandos (CLI)** con soporte multiplataforma (Linux, Windows, macOS).

## Estado del proyecto (listo para producción)

El ensamblador en C está alineado con la lógica del ensamblador C++ de referencia para los casos de verificación incluidos:

- ✅ Preprocesado de `INCLUDE` (incluyendo includes recursivos)
- ✅ Ensamblado en dos pasadas (resolución de símbolos + generación de código)
- ✅ Evaluación de expresiones (`+ - * / << >>`, `()`, `$`, literales y símbolos)
- ✅ Directivas (`ORG`, `EQU`, `DB`, `DW`, `DL`, `DM`, `DS`)
- ✅ Generación de Intel HEX y listing (`.lst`)

Además, el repositorio incluye un script de verificación end-to-end para validar:

1. C++ vs HEX de referencia
2. C vs HEX de referencia
3. C vs C++

Ver sección **Verificación de paridad (C / C++ / referencia)**.

## ✨ Características

### Características del Ensamblador
- ✅ **Análisis léxico completo**: Tokenización de código assembly
- ✅ **Parser sintáctico**: Validación de sintaxis y estructura en dos pasadas
- ✅ **Generador de código**: Traducción a bytecode del Megaprocessor
- ✅ **Manejo de etiquetas**: Soporte para saltos y referencias
- ✅ **Detección de errores**: Mensajes claros de errores de sintaxis
- ✅ **Generación de archivos**: Produce archivos `.hex` (Intel HEX) y `.lst` (listado)
- ✅ **Soporte de includes**: Preprocesamiento de directivas `INCLUDE` con resolución recursiva
- ✅ **Evaluación de expresiones**: Soporte de operadores aritméticos y lógicos en tiempo de ensamblado
- ✅ **Multiplataforma**: Funciona en Linux, Windows y macOS

### Características Técnicas
- 📝 **Código C99**: Portabilidad máxima y compatibilidad con compiladores estándar
- 🔧 **Build con CMake**: Sistema de compilación moderno y multiplataforma
- ⚡ **Performance nativa**: Código nativo compilado para máxima velocidad
- 🧪 **Suite de verificación**: Scripts automáticos para validar paridad con ensamblador de referencia
- 📚 **Documentación completa**: API pública bien documentada

## 🏗️ Arquitectura del Megaprocessor

El Megaprocessor es un procesador de 16 bits con:
- **Arquitectura**: Procesador de 16-bit Load/Store con bus de datos externo de 8-bit
- **Ancho de palabra**: 16 bits
- **Registros**: 4 registros de propósito general (R0, R1, R2, R3) + SP, PS, PC
- **Memoria**: Espacio de direccionamiento de 64KB (0x0000 - 0xFFFF)
- **Set de instrucciones**: Opcodes de 1 byte con operandos variables (ALU, Branches, Memoria, Stack)

Para más información sobre el Megaprocessor, visita: http://www.megaprocessor.com/

## 📦 Requisitos

- **CMake** 3.10 o superior
- **Compilador C compatible con C99**:
  - Linux: GCC o Clang
  - macOS: Apple Clang (Xcode Command Line Tools)
  - Windows: MSVC o MinGW-w64

---

## 🚀 Compilación del ensamblador C

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

---

## 📖 Uso del ensamblador (línea de comandos)

### Sintaxis básica

```bash
megap-asm <archivo.asm> [opciones]
```

### Opciones disponibles

- `--out <archivo.hex>`: Especifica la ruta de salida para el archivo `.hex`
- `--lst`: Habilita la generación del archivo de listado `.lst`
- `--lst-out <archivo.lst>`: Especifica la ruta de salida para el archivo `.lst` (activa automáticamente `--lst`)

### Ejemplos de uso

#### Generar solo archivo `.hex` (por defecto)

```bash
megap-asm programa.asm
```

Genera: `programa.hex` (mismo nombre que el `.asm`)

#### Especificar nombre de salida para `.hex`

```bash
megap-asm programa.asm --out salida.hex
```

#### Generar `.hex` y `.lst` automáticamente

```bash
megap-asm programa.asm --lst
```

Genera: `programa.hex` y `programa.lst`

#### Especificar rutas personalizadas para `.hex` y `.lst`

```bash
megap-asm programa.asm --out salida.hex --lst-out salida.lst
```

---

## 📁 Includes (`INCLUDE`) y ensamblado de archivos con dependencias

Si un archivo `.asm` usa la directiva `INCLUDE`, **debes tener disponibles también los archivos `.asm` incluidos**.

### Orden de resolución de includes

El CLI resuelve includes en este orden:

1. `Megaprocessor_defs.asm` (si existe en la misma carpeta que el archivo `.asm`)
2. Includes declarados en el archivo `.asm` principal
3. Búsqueda en:
   - Carpeta del archivo `.asm` principal
   - Subcarpeta `includes/` relativa al archivo `.asm`
   - Ruta directa del nombre de include

### Estructura de proyecto recomendada

```text
proyecto/
  main.asm
  Megaprocessor_defs.asm
  includes/
    macros.asm
    constantes.asm
```

### Ejemplo de ensamblado con includes

```bash
./build/megap-asm ./proyecto/main.asm --out ./proyecto/main.hex --lst --lst-out ./proyecto/main.lst
```

### Ejemplo de código assembly con includes

```asm
; Ejemplo funcional para Megaprocessor
include "Megaprocessor_defs.asm"

        org 0x0000
start:
        ld.w  r0, #10      ; Cargar 10 en R0
        ld.w  r1, #20      ; Cargar 20 en R1
        add   r2, r0, r1   ; r2 = r0 + r1
        st.w  result, r2   ; Guardar r2 en 'result'
        jmp   loop         ; Bucle infinito

loop:
        jmp   loop

        org 0x1000
result:
        dw    0x0000       ; Espacio para el resultado
```

---

## 🧪 Verificación de paridad (C / C++ / referencia)

El repositorio incluye:

- `Nuevo.zip`: Archivos `.asm` + `.hex`/`.lst` de referencia
- `emsablador.zip`: Código/scripts del ensamblador C++ de referencia
- `scripts/verify_migration_parity.sh`: Flujo de verificación completo

### Ejecutar verificación completa

```bash
scripts/verify_migration_parity.sh
```

Opcionalmente puedes pasar rutas explícitas:

```bash
scripts/verify_migration_parity.sh --nuevo-zip ./Nuevo.zip --cpp-zip ./emsablador.zip
```

### ¿Qué hace el script de verificación?

Este script realiza:

1. **Auditoría básica anti-hardcode** en el ensamblador C (sin rutas acopladas a C++/artefactos temporales)
2. **Ejecuta verificación oficial C++** (`verify_hex_equivalence.sh`) contra referencia
3. **Compila el ensamblador C** de este repositorio
4. **Ensambla todos los `.asm`** de `Nuevo.zip` con el ensamblador C generando `.hex` y `.lst`
5. **Compila un CLI C++ local** desde `emsablador/cpp` y compara `.hex`:
   - C vs referencia
   - C++ vs referencia
   - C vs C++

Si hay una divergencia, termina con error y muestra el caso fallido.

---

## 💻 Instalación opcional del CLI

### Linux/macOS

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
sudo cmake --install build --prefix /usr/local
```

El binario se instalará en `/usr/local/bin/megap-asm`

### Windows

```bat
cmake -S . -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build --prefix C:\tools\megap-asm
```

Agrega `C:\tools\megap-asm\bin` al `PATH` del sistema.

---

## 📂 Estructura del Proyecto

```
Megaprocessor-ASM-C/
├── include/
│   └── megap_asm.h              # API pública del ensamblador
├── src/
│   ├── assembler.c              # Core del ensamblador (2-pass + HEX + LST)
│   └── main.c                   # CLI y carga de archivos/includes
├── scripts/
│   └── verify_migration_parity.sh  # Verificación C vs C++ vs referencia
├── Nuevo.zip                    # Archivos ASM/HEX/LST de referencia
├── emsablador.zip               # Código del ensamblador C++ de referencia
├── CMakeLists.txt               # Build e instalación
├── LICENSE                      # Licencia Apache 2.0
└── README.md                    # Este archivo
```

---

## 🛠️ Arquitectura Técnica

### Flujo de Datos del Ensamblador

```
┌─────────────────────┐
│   main.c            │ (CLI)
│   - Carga archivos  │
│   - Procesa args    │
│   - Resolve includes│
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│   assembler.c       │ (Core)
│   - Primera pasada  │ (resolución de símbolos)
│   - Segunda pasada  │ (generación de código)
│   - Generador HEX   │ (formato Intel HEX)
│   - Generador LST   │ (listado)
└─────────────────────┘
           │
           ▼
┌─────────────────────┐
│   Archivos salida   │
│   - .hex (bytecode) │
│   - .lst (listado)  │
└─────────────────────┘
```

### Proceso de ensamblado en dos pasadas

**Primera pasada:**
- Construye tabla de símbolos (etiquetas y sus direcciones)
- Calcula direcciones de todas las instrucciones
- Procesa directivas `EQU` y `ORG`
- Valida sintaxis básica

**Segunda pasada:**
- Genera código máquina usando la tabla de símbolos
- Resuelve referencias a etiquetas
- Evalúa expresiones con símbolos definidos
- Genera archivo Intel HEX y listado

---

## 🤝 Contribuciones

Las contribuciones son bienvenidas. Si deseas contribuir:

1. Fork el proyecto
2. Crea una rama para tu característica (`git checkout -b feature/nueva-caracteristica`)
3. Commit tus cambios (`git commit -am 'Añadir nueva característica'`)
4. Push a la rama (`git push origin feature/nueva-caracteristica`)
5. Abre un Pull Request

### Guías de Contribución

- **Código C**: Usar estándar C99, formato consistente
- **Commits**: Mensajes descriptivos en español o inglés
- **Testing**: Añadir tests para nueva funcionalidad
- **Documentación**: Actualizar README si cambias features
- **Verificación**: Ejecutar `scripts/verify_migration_parity.sh` antes de PR

---

## 📄 Licencia

Este proyecto está licenciado bajo la **Apache License 2.0**. Consulta el archivo [LICENSE](LICENSE) para más detalles.

---

## 👤 Autor

**Daniel Elias Diamon Vazquez**
- GitHub: [@Danielk10](https://github.com/Danielk10)
- Email: danielpdiamon@gmail.com
- Website: [todoandroid.42web.io](https://todoandroid.42web.io/)
- Ubicación: Venezuela
- Especialidades: Desarrollo de juegos 2D (libGDX), Android nativo, Microcontroladores PIC, Ensambladores y compiladores

---

## 🙏 Agradecimientos

- **James Newman** - Creador del Megaprocessor físico
- Comunidad de desarrolladores de ensambladores y compiladores
- Comunidad de software libre y código abierto

---

## 📚 Recursos Adicionales

### Sobre el Megaprocessor
- [Megaprocessor Official Website](http://www.megaprocessor.com/)
- [Documentación del Set de Instrucciones](http://www.megaprocessor.com/instruction.html)
- [Megaprocessor en YouTube](https://www.youtube.com/watch?v=lNuPy-r1GuQ)

### Herramientas y Referencias
- [CMake Documentation](https://cmake.org/documentation/)
- [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0)
- [Intel HEX Format](https://en.wikipedia.org/wiki/Intel_HEX)
- [C99 Standard](https://en.wikipedia.org/wiki/C99)

---

## 🐛 Reporte de Bugs

Si encuentras algún bug, por favor abre un [issue](https://github.com/Danielk10/Megaprocessor-ASM-C/issues) con:
- Descripción del problema
- Pasos para reproducir
- Comportamiento esperado vs. observado
- Sistema operativo y versión de compilador
- Archivo `.asm` que causa el problema (si aplica)
- Salida de error completa

---

## 🔗 Proyectos Relacionados

- [Megaprocessor Official](http://www.megaprocessor.com/) - Procesador físico original
- [Megaprocessor-ASM-Android](https://github.com/Danielk10/Megaprocessor-ASM-Android) - Versión Android NDK del ensamblador

---

**¡Hecho con ❤️ para la comunidad del Megaprocessor!**

**Ensambla código para el Megaprocessor desde tu terminal 💻⚡**
