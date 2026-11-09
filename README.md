# EDA-Lab04: Analizador Léxico y Sintáctico SQL (SELECT)

**Curso:** Estructuras de Datos Avanzadas — UNSA, Escuela Profesional de Ciencia de la Computación  
**Lenguaje:** C++20 · **Build:** CMake ≥ 3.14

- **Práctica 4 (terminada):** analizador léxico.
- **Práctica 5 (implementada; falta el informe PDF):** analizador sintáctico predictivo LL(1) no recursivo, en `parser_ll1/`: gramática, FIRST/FOLLOW, tabla M[A, a] construida en código, motor por pila, errores con posición, traza paso a paso y pruebas.

## Estructura

```
include/ src/ tests/ examples/     Práctica 4: lexer (token, lexer, input) y sus pruebas
parser_ll1/                        Práctica 5
├── include/  grammar.h parse_table.h parser.h trace.h
├── src/      grammar.cpp parse_table.cpp parser.cpp trace.cpp main.cpp
├── tests/    test_parser.cpp          (gramática y motor, con tabla de referencia)
│             test_tabla_errores.cpp   (tabla construida, traza y consultas inválidas)
├── examples/ parser_valid_*.sql  parser_invalid*.sql
└── docs/     gramatica.md  traza_ejemplo.txt
```

## Compilar y ejecutar

```bash
mkdir build && cd build
cmake ..
cmake --build .
./test_lexer                         # pruebas del lexer
./test_parser                        # pruebas de la gramática y del motor LL(1)
./test_parser_tabla                  # pruebas de la tabla, la traza y las consultas inválidas
ctest                                # corre las tres suites
./sql_lexer ../examples/valida.sql   # analizar un archivo
./sql_lexer --demo                   # ejecutar los ejemplos (desde la raíz del repo)
echo "SELECT * FROM t;" | ./sql_lexer
```

(`--demo` busca `examples/` relativo al directorio actual, así que ejecútalo desde la raíz: `./build/sql_lexer --demo`.)

## Analizador sintáctico (Práctica 5)

Ejecutable `parser_ll1` (lexer + parser LL(1)); por defecto muestra la traza paso a paso.

```bash
./parser_ll1 ../parser_ll1/examples/parser_valid_complex.sql          # con traza
./parser_ll1 --quiet ../parser_ll1/examples/parser_valid_simple.sql   # solo el resultado
./parser_ll1 --tables                                                 # gramática, FIRST/FOLLOW y tabla M[A, a]
echo "SELECT a FROM ;" | ./parser_ll1 --quiet                         # lee la entrada estándar
./parser_ll1 --help
```

| Código de salida | Significado |
|---|---|
| `0` | consulta aceptada |
| `1` | error sintáctico (línea, columna, token encontrado y tokens esperados) |
| `2` | error léxico (la consulta se rechaza sin llegar al parser) |
| `3` | no se pudo leer el archivo u opción desconocida |

Modo todo o nada: ante el primer error léxico o sintáctico se detiene y se reporta. Las advertencias del lexer no bloquean el análisis.

Gramática aceptada (una sentencia por entrada; `AND` tiene mayor precedencia que `OR`; sin paréntesis):
```
Query ::= SELECT ( * | id { , id } ) FROM id [ WHERE Cond ] ;
Cond  ::= Pred { AND Pred } { OR Pred { AND Pred } }
Pred  ::= id RelOp ( int | float | string | id )
RelOp ::= = | <> | != | < | > | <= | >=
```
La versión LL(1) (24 producciones), los conjuntos FIRST/FOLLOW y la tabla están en `parser_ll1/docs/gramatica.md`; un ejemplo de traza, en `parser_ll1/docs/traza_ejemplo.txt`.

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
