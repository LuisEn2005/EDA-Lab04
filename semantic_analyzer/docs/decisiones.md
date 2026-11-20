# Decisiones de Diseño — Práctica 6

Cada decisión describe lo que hace el código de `semantic_analyzer/` (no lo que se pensó hacer).

## 1. Integración con las prácticas anteriores

- Si el lexer o el parser LL(1) reportan errores, **no** se ejecuta el análisis semántico.
- No se modifican el lexer ni el parser: el AST se construye a partir de los tokens y de
  `ParseResult::productions` (ver `sdt.md`, sección 4).

## 2. Catálogo (`data/catalog.csv`)

- Formato de 4 columnas con encabezado, una fila por columna de cada tabla:
  `table_name,column_name,data_type,is_primary_key`.
- La fila de encabezado se omite. No se admiten comentarios ni líneas con otro formato: cada línea no vacía se
  interpreta como una columna.
- La tabla se crea al ver su primera columna. El `ordinal` es la posición de declaración, **empezando en 0**.
- `nullable` se deriva de la llave primaria (`nullable = !is_pk`) y el almacenamiento de todas las tablas es el
  identificador lógico `"disk"` (limitaciones del cargador actual).
- Esquema de prueba: 9 tablas (`usuarios`, `productos`, `ordenes`, `estudiantes`, `cursos`, `matriculas`,
  `empleados`, `departamentos`, `proyectos`) con `INT`, `FLOAT`, `VARCHAR` y `BOOLEAN`.
- Los identificadores son **sensibles a mayúsculas**, igual que en el lexer.

## 3. Expansión de `SELECT *`

El `StarNode` de `selectList` se reemplaza por un `ColumnNode` por cada columna de la tabla, en el orden del
catálogo, cada uno con su tipo. Si la tabla del `FROM` no existe, no se expande.

## 4. Columnas duplicadas en la proyección

Política: **rechazar**. Se reporta `DUPLICATE_COLUMN` como error, con la línea y columna de la repetición.

## 5. Compatibilidad y coerción de tipos (`WHERE`)

| Operandos | `=`, `<>`, `<`, `>`, `<=`, `>=` |
|---|---|
| `INT` con `INT`, `FLOAT` con `FLOAT` | permitido |
| `INT` con `FLOAT` (en cualquier orden) | permitido, promoción numérica implícita `INT → FLOAT` |
| `VARCHAR` con `VARCHAR`, `BOOLEAN` con `BOOLEAN` | permitido |
| cualquier otra combinación (`VARCHAR` con `INT`/`FLOAT`, `BOOLEAN` con numérico, ...) | `TYPE_MISMATCH` |

- Toda comparación produce un resultado `BOOLEAN`.
- `AND` y `OR` exigen operandos `BOOLEAN`; si no, `LOGICAL_OPERAND_NOT_BOOLEAN`. Con la gramática actual los
  operandos de `AND`/`OR` siempre son comparaciones, así que este error solo se puede provocar con un AST armado a mano.
- Si la cláusula `WHERE` completa no evalúa a `BOOLEAN`, se reporta `TYPE_MISMATCH`.
- La promoción es implícita: el AST no marca qué operando se promueve; ambos conservan su tipo inferido.

## 6. Manejo de errores

- Se acumulan **todos** los diagnósticos de la consulta (no se detiene en el primero).
- Sin cascada: una subexpresión con error queda en `UNKNOWN` y sus padres no reportan un segundo error por ello.
- Si la tabla del `FROM` no existe, no se validan las columnas (no hay contra qué compararlas).
- Cada diagnóstico lleva tipo, identificador o expresión conflictiva, línea y columna.

| Tipo | Cuándo |
|---|---|
| `TABLE_NOT_FOUND` | la tabla del `FROM` no está en el catálogo |
| `COLUMN_NOT_FOUND` | la columna no pertenece a la tabla del `FROM` |
| `TYPE_MISMATCH` | comparación entre tipos incompatibles |
| `LOGICAL_OPERAND_NOT_BOOLEAN` | `AND`/`OR` con un operando no `BOOLEAN` |
| `DUPLICATE_COLUMN` | columna repetida en la proyección |

## 7. Literales `TRUE` / `FALSE`

El lexer y la gramática no definen `TRUE`/`FALSE`: la palabra `true` se tokeniza como identificador y se
resolvería como una columna llamada `true` (`COLUMN_NOT_FOUND`). Decisión: **no** ampliar el lexer ni la gramática
para no romper las pruebas de las Prácticas 4 y 5. `BOOLEAN` existe en el catálogo y solo puede compararse con otra
columna `BOOLEAN`. Es una limitación documentada.

## 8. Gestión de memoria

`std::unique_ptr` para todo el AST (sin `new`/`delete` manuales) y `std::unordered_map` en el catálogo para
búsquedas de tablas y columnas en O(1) promedio.
