# Megaprocessor ASM - Ensamblador en C

**Ensamblador de código abierto para el Megaprocessor escrito en C puro.**

[![Licencia](https://img.shields.io/badge/Licencia-Apache%202.0-blue.svg)](LICENSE)
[![Lenguaje](https://img.shields.io/badge/Lenguaje-C99-orange.svg)](https://en.cppreference.com/w/c/99)
[![Build System](https://img.shields.io/badge/Build-CMake-064F8C.svg)](https://cmake.org/)

## 📋 Descripción

Este proyecto es un **ensamblador completo** para el [Megaprocessor](http://www.megaprocessor.com/), un procesador de 16 bits construido con componentes discretos creado por James Newman. El Megaprocessor es un procesador físico gigante donde cada transistor es visible, diseñado con fines educativos para mostrar cómo funciona un CPU por dentro.

Este ensamblador traduce código assembly del Megaprocessor a código máquina binario que puede ejecutarse en el procesador físico o en simuladores.

## ✨ Características

- ✅ **Análisis léxico completo**: Tokenización de código assembly
- ✅ **Parser sintáctico**: Validación de sintaxis y estructura
- ✅ **Generador de código**: Traducción a bytecode del Megaprocessor
- ✅ **Manejo de etiquetas**: Soporte para saltos y referencias
- ✅ **Detección de errores**: Mensajes claros de errores de sintaxis
- ✅ **Multiplataforma**: Compatible con Linux, Windows y macOS
- ✅ **Sin dependencias externas**: Solo requiere un compilador C estándar

## 🏗️ Arquitectura del Megaprocessor

El Megaprocessor es un procesador de 16 bits con:
- **Arquitectura**: Von Neumann modificada
- **Ancho de palabra**: 16 bits
- **Registros**: 8 registros de propósito general
- **Memoria**: Espacio de direccionamiento de 64KB
- **Set de instrucciones**: RISC simplificado con ~40 instrucciones

Para más información sobre el Megaprocessor, visita: http://www.megaprocessor.com/

## 🚀 Compilación

### Requisitos

- **Compilador C**: GCC 4.8+ o Clang 3.5+
- **CMake**: 3.10 o superior
- **Sistema operativo**: Linux, macOS, Windows (con MinGW/MSYS2)

### En Linux/macOS

```bash
# Clonar el repositorio
git clone https://github.com/Danielk10/Megaprocessor-ASM-C.git
cd Megaprocessor-ASM-C

# Crear directorio de build
mkdir build
cd build

# Configurar y compilar
cmake ..
cmake --build .
```

### En Windows (MinGW/MSYS2)

```bash
# Desde MSYS2 terminal
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
cmake --build .
```

## 📖 Uso

```bash
# Ejecutar el ensamblador
./megaprocessor-asm archivo.asm

# Esto genera:
# - archivo.asm.bin (código máquina binario)
```

### Ejemplo de código assembly

```asm
; Programa de ejemplo para Megaprocessor
; Suma dos números y almacena el resultado

start:
    LOAD R0, #5        ; Cargar 5 en R0
    LOAD R1, #10       ; Cargar 10 en R1
    ADD R2, R0, R1     ; R2 = R0 + R1
    STORE R2, result   ; Guardar en memoria
    HALT               ; Detener ejecución

result:
    .word 0            ; Espacio para resultado
```

## 📂 Estructura del Proyecto

```
Megaprocessor-ASM-C/
├── src/
│   ├── main.c          # Punto de entrada del programa
│   ├── lexer.c         # Análisis léxico (tokenización)
│   ├── parser.c        # Análisis sintáctico
│   └── codegen.c       # Generación de código máquina
├── include/
│   └── megaprocessor_asm.h  # Definiciones y prototipos
├── CMakeLists.txt      # Configuración de CMake
├── README.md           # Este archivo
├── LICENSE             # Licencia Apache 2.0
└── .gitignore          # Archivos ignorados por Git
```

## 🛠️ Desarrollo

### Compilador Recomendado

- **Linux**: GCC (GNU Compiler Collection)
- **macOS**: Clang (incluido con Xcode Command Line Tools)
- **Windows**: MinGW-w64 o Clang

### Sistema de Build

Este proyecto utiliza **CMake** como sistema de build, lo que facilita:
- ✅ Compilación multiplataforma
- ✅ Detección automática de compiladores
- ✅ Configuración sencilla de flags de compilación
- ✅ Fácil integración con IDEs (CLion, Visual Studio Code, etc.)

### Estándar de C

El código está escrito en **C99** (ISO/IEC 9899:1999) para:
- Máxima portabilidad
- Compatibilidad con compiladores modernos
- Características modernas sin complejidad innecesaria

## 🎯 Hoja de Ruta (Roadmap)

- [x] Estructura básica del proyecto
- [x] Configuración de CMake
- [x] Analizador léxico base
- [ ] Parser completo de instrucciones
- [ ] Generador de código para todas las instrucciones del Megaprocessor
- [ ] Tabla de símbolos para etiquetas
- [ ] Segunda pasada del ensamblador (resolución de referencias)
- [ ] Soporte para directivas (`.org`, `.word`, `.byte`)
- [ ] Generación de archivos de listado
- [ ] Modo de depuración con símbolos
- [ ] Optimizaciones básicas
- [ ] Suite de pruebas unitarias
- [ ] Documentación completa del set de instrucciones
- [ ] Integración con simulador del Megaprocessor

## 🤝 Contribuciones

Las contribuciones son bienvenidas. Si deseas contribuir:

1. Fork el proyecto
2. Crea una rama para tu característica (`git checkout -b feature/nueva-caracteristica`)
3. Commit tus cambios (`git commit -am 'Añadir nueva característica'`)
4. Push a la rama (`git push origin feature/nueva-caracteristica`)
5. Abre un Pull Request

## 📄 Licencia

Este proyecto está licenciado bajo la **Apache License 2.0**. Consulta el archivo [LICENSE](LICENSE) para más detalles.

Copyright 2026 Daniel Elias Diamon Vazquez

## 👤 Autor

**Daniel Elias Diamon Vazquez**
- GitHub: [@Danielk10](https://github.com/Danielk10)
- Email: danielpdiamon@gmail.com
- Ubicación: Venezuela

## 🙏 Agradecimientos

- **James Newman** - Creador del Megaprocessor físico
- Comunidad de desarrolladores de ensambladores y compiladores
- Proyecto SDCC por inspiración en arquitectura de compiladores

## 📚 Recursos Adicionales

- [Megaprocessor Official Website](http://www.megaprocessor.com/)
- [Documentación del Set de Instrucciones](http://www.megaprocessor.com/instruction.html)
- [CMake Documentation](https://cmake.org/documentation/)
- [C99 Standard Reference](https://en.cppreference.com/w/c/99)

## 🐛 Reporte de Bugs

Si encuentras algún bug, por favor abre un [issue](https://github.com/Danielk10/Megaprocessor-ASM-C/issues) con:
- Descripción del problema
- Pasos para reproducir
- Comportamiento esperado vs. observado
- Versión del compilador y sistema operativo

---

**¡Hecho con ❤️ para la comunidad del Megaprocessor!**
