# EDA-Lab04: Analizador Léxico y Sintáctico SQL (SELECT)

**Curso:** Estructuras de Datos Avanzadas — UNSA, Escuela Profesional de Ciencia de la Computación  
**Lenguaje:** C++20 · **Build:** CMake ≥ 3.14

- **Práctica 4 (terminada):** analizador léxico.
- **Práctica 5 (en curso):** analizador sintáctico predictivo LL(1) no recursivo, en `parser_ll1/`. Hecho: gramática, FIRST/FOLLOW y motor por pila. Pendiente: construcción de la tabla (`parse_table.cpp`), traza, pruebas de consultas inválidas, ejecutable e informe.

## Estructura

```
include/ src/ tests/ examples/     Práctica 4: lexer (token, lexer, input) y sus pruebas
parser_ll1/                        Práctica 5
├── include/  grammar.h parser.h parse_table.h
├── src/      grammar.cpp parser.cpp parse_table.cpp (pendiente)
├── tests/    test_parser.cpp
├── examples/ parser_valid_*.sql  (parser_invalid.sql pendiente)
└── docs/     gramatica.md        (gramática, FIRST/FOLLOW y tabla derivados a mano)
```

## Compilar y ejecutar

```bash
mkdir build && cd build
cmake ..
cmake --build .
./test_lexer                         # pruebas del lexer
./test_parser                        # pruebas de la gramática y del motor LL(1)
./sql_lexer ../examples/valida.sql   # analizar un archivo
./sql_lexer --demo                   # ejecutar los ejemplos (desde la raíz del repo)
echo "SELECT * FROM t;" | ./sql_lexer
```

(`--demo` busca `examples/` relativo al directorio actual, así que ejecútalo desde la raíz: `./build/sql_lexer --demo`.)

## Tokens

| Token | Detalle |
|---|---|
| `KEYWORD_SELECT/FROM/WHERE/AND/OR` | insensibles a mayúsculas; el lexema conserva la escritura original |
| `IDENTIFIER` | `[A-Za-z_][A-Za-z0-9_]*`, sensible a mayúsculas |
| `INTEGER_LITERAL`, `FLOAT_LITERAL` | el `-` va pegado al número; `.5` y `5.` se aceptan con advertencia (valor `0.5` / `5.0`) |
| `STRING_LITERAL` | `'texto'`; lexema y valor sin comillas; escapes `\'` y `\\`; escape desconocido = advertencia |
| `EQUAL`, `NOT_EQUAL` (`<>` o `!=`), `LESS_THAN`, `GREATER_THAN`, `LESS_EQUAL`, `GREATER_EQUAL` | máximo acierto (`>=`, `<=`, `<>`, `!=` son un solo token) |
| `COMMA`, `SEMICOLON`, `STAR` (`*`) | puntuación y proyección total |
| `TOKEN_EOF` | siempre es el último token (único) |

## Reglas y errores léxicos

- Posiciones (línea, columna) desde 1; `\r\n` cuenta como un salto.
- Los errores **no son tokens**: se acumulan en `diagnostics()` (`ERROR` / `WARNING`) y el análisis continúa.
- Son error: caracteres no válidos (`@ # $ ...`), `!` solo, `-` suelto, cadena sin cerrar (o con salto de línea real), `1abc`, `1.2.3`, caracteres no ASCII fuera de cadenas.
- `hasErrors()` es verdadero solo si hay `ERROR` (las advertencias no cuentan).

## Interfaz para el parser

```cpp
Lexer lexer(source);
std::vector<Token> tokens = lexer.tokenizeAll();   // termina en TOKEN_EOF
// o bien: Token t = lexer.nextToken();            // repite EOF al final
```
