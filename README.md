# Analizador Léxico SQL en C++

Este proyecto implementa la Práctica de Laboratorio 4: **Analizador Léxico SQL** para un subconjunto de sentencias `SELECT`.

## Estructura del Proyecto

```
sql_lexer/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── Token.h
│   └── Lexer.h
└── src/
    ├── Lexer.cpp
    └── main.cpp
    └── token.cpp

└── tests/
```

## Compilación y Ejecución

### Opción 1: Usando CMake

```bash
mkdir build
cd build
cmake ..
make
./sql_lexer
```

### Opción 2: Compilación Directa con g++ / clang++

```bash
g++ -std=c++20 -Iinclude src/Lexer.cpp src/main.cpp -o sql_lexer
./sql_lexer
```

## Características Soportadas

- **Palabras Reservadas**: `SELECT`, `FROM`, `WHERE`, `AND`, `OR` (insensible a mayúsculas/minúsculas).
- **Identificadores y Literales**: Cadenas de texto (`'texto'`), enteros, decimales.
- **Operadores Relacionales**: `=`, `<>`, `!=`, `<`, `>`, `<=`, `>=`.
- **Signos de Puntuación**: Coma (`,`), Punto y coma (`;`).
- **Control de Posición**: Registro de número de línea y columna por cada token.
- **Manejo de Errores**: Detección de caracteres inválidos y cadenas sin cerrar.
# EDA-Lab04
