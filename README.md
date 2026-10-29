
# Práctica de Laboratorio 4: Analizador Léxico SQL

**Curso:** Estructuras de Datos Avanzadas  
**Institución:** Universidad Nacional de San Agustín (UNSA)  
**Escuela Profesional:** Escuela Profesional de Ciencia de la Computación  
**Tema:** Fase 2 - Analizador Léxico para Sentencias SQL `SELECT`  
**Lenguaje:** C++ (Estándar C++17)  
**Herramienta de Construcción:** CMake (versión mínima 3.15)  

---

## 1. Introducción y Contexto General

El presente proyecto constituye la primera etapa de la **Fase 2** en la construcción progresiva de un servidor de base de datos relacional para sentencias simples de tipo `SELECT`. 

El analizador léxico (*lexer* o *scanner*) es el componente inicial del procesamiento de consultas. Su responsabilidad técnica es examinar un flujo o cadena de caracteres proveniente de la entrada estándar o de archivos `.sql`, aislar las unidades léxicas con significado (*lexemas*), clasificarlas según la gramática léxica definida y emitir una secuencia ordenada de tokens con información contextual precisa (tipo, lexema, fila y columna de inicio). 

Este módulo opera de manera desacoplada del almacenamiento y de la semántica de la base de datos: no verifica la existencia de tablas ni columnas ni ejecuta la consulta; su única tarea es garantizar una interfaz de tokens limpia, robusta y con reporte detallado de errores para el posterior analizador sintáctico (*parser*).

---

## 2. Especificación de Tokens y Reglas Léxicas

### 2.1. Catálogo Completo de Tokens

| Categoría | Nombre del Token | Patrón Léxico / Ejemplos | Descripción |
| :--- | :--- | :--- | :--- |
| **Palabras Reservadas** | `KEYWORD_SELECT` | `SELECT`, `select`, `Select` | Inicio de proyección de consulta |
| | `KEYWORD_FROM` | `FROM`, `from` | Cláusula de origen de datos/tabla |
| | `KEYWORD_WHERE` | `WHERE`, `where` | Cláusula de condición/filtrado |
| | `KEYWORD_AND` | `AND`, `and` | Conjunción lógica en filtros |
| | `KEYWORD_OR` | `OR`, `or` | Disyunción lógica en filtros |
| **Identificadores** | `IDENTIFIER` | `[a-zA-Z_][a-zA-Z0-9_]*` | Nombres de columnas, alias y tablas |
| **Literales Numéricos** | `INTEGER_LITERAL` | `[0-9]+` | Números enteros (ej. `18`, `420`, `0`) |
| | `DECIMAL_LITERAL` | `[0-9]+\.[0-9]+` | Números con parte fraccionaria (ej. `3.1416`, `0.5`) |
| **Literales de Cadena** | `STRING_LITERAL` | `'[^']*'` | Cadenas delimitadas por comillas simples (ej. `'Juan'`) |
| **Operadores Relacionales** | `EQUAL` | `=` | Comparación de igualdad |
| | `LESS` | `<` | Menor que |
| | `GREATER` | `>` | Mayor que |
| | `LESS_EQUAL` | `<=` | Menor o igual que (operador compuesto) |
| | `GREATER_EQUAL` | `>=` | Mayor o igual que (operador compuesto) |
| | `NOT_EQUAL` | `<>` o `!=` | Distinto / no igual (operador compuesto) |
| **Puntuación** | `COMMA` | `,` | Separador de atributos en proyección |
| | `SEMICOLON` | `;` | Terminador de sentencia SQL |
| **Control** | `END_OF_FILE` / `EOF` | Fin de flujo | Señal de fin de entrada para el analizador sintáctico |
| | `TOKEN_ERROR` | Carácter/patrón inválido | Representación de lexema mal formado o no reconocido |

---

### 2.2. Reglas Léxicas y Manejo de Casos Especiales

1. **Insensibilidad a Mayúsculas y Minúsculas (*Case-Insensitivity*):**  
   Las palabras clave del subconjunto SQL son reconocidas independientemente de su capitalización (`sElEcT` $\equiv$ `SELECT`). Internamente, el reconocedor normaliza el texto a mayúsculas antes de consultar la tabla de palabras reservadas. Los identificadores y los literales de cadena preservan su valor original.

2. **Precedencia entre Palabras Reservadas e Identificadores:**  
   Todo lexema alfanumérico que cumpla el patrón `[a-zA-Z_][a-zA-Z0-9_]*` es evaluado primero contra la tabla de palabras reservadas (implementada mediante `std::unordered_map`). Si coincide, se etiqueta como palabra reservada; de lo contrario, se clasifica como `IDENTIFIER`.

3. **Reconocimiento de Operadores Compuestos (*Principio de Máximo Acierto / Lookahead*):**  
   Al detectar los caracteres `<`, `>`, `!` o `=`, el algoritmo realiza una lectura anticipada (*peek*) del siguiente carácter antes de consumir la entrada:
   * Si tras `<` se detecta `=` $\rightarrow$ `LESS_EQUAL` (`<=`).
   * Si tras `<` se detecta `>` $\rightarrow$ `NOT_EQUAL` (`<>`).
   * Si tras `>` se detecta `=` $\rightarrow$ `GREATER_EQUAL` (`>=`).
   * Si tras `!` se detecta `=` $\rightarrow$ `NOT_EQUAL` (`!=`).
   * En caso contrario, se emite el operador simple correspondiente sin dividir indebidamente los compuestos.

4. **Tratamiento de Espacios en Blanco:**  
   Espacios (`' '`), tabulaciones horizontales (`'\t'`) y saltos de línea (`'\n'`, `'\r'`) son consumidos sin emitir tokens. Cada salto de línea incrementa el contador de línea y reinicia la columna en 1, garantizando que el reporte de coordenadas sea exacto.

5. **Literales de Cadena y Errores de Cierre:**  
   Las cadenas inician con `'` y consumen caracteres hasta hallar la comilla de cierre. Si se encuentra un salto de línea o el fin del archivo sin cerrar la comilla, se reporta un error léxico de cadena incompleta con la posición de apertura.

6. **Detección de Números Decimales y Validación:**  
   Un número que contiene un punto seguido inmediatamente de dígitos se clasifica como `DECIMAL_LITERAL`. Si un punto no va precedido ni seguido de dígitos válidos o presenta múltiples puntos consecutivos, se clasifica como error léxico.

---

## 3. Arquitectura y Componentes Principales

El proyecto sigue una separación estricta de responsabilidades en tres capas:

```text

EDA-Lab04/
├── .vscode/
│   ├── c_cpp_properties.json
│   └── settings.json
├── CMakeFiles/
│   ├── 4.4.4/
│   ├── 4.5.0-rc1/
│   ├── pkgRedirects/
│   ├── cmake.check_cache
│   └── CMakeConfigureLog.yaml
├── examples/
│   ├── errores.sql
│   ├── espacios.sql
│   ├── limite.sql
│   ├── literales.sql
│   ├── operadores.sql
│   └── valida.sql
├── include/
│   ├── input.h
│   ├── lexer.h
│   └── token.h
├── src/
│   ├── input.cpp
│   ├── lexer.cpp
│   ├── main.cpp
│   └── token.cpp
├── tests/
│   └── test_lexer.cpp
├── .gitignore
├── CMakeCache.txt
├── CMakeLists.txt
├── main.exe
└── README.md
```


### Descripción de Módulos

* **Modelo de Tokens (`token.h`, `token.cpp`):**  
  Define la estructura `Position` (línea, columna) y la estructura `Token` (tipo enumerado `TokenType`, lexema en `std::string` y posición). Provee métodos como `tokenTypeToString()` para imprimir la salida en un formato legible para el analizador sintáctico.
* **Módulo de Lectura (`input.h`, `input.cpp`):**  
  Abstrae la fuente de datos ya sea desde un archivo físico en disco o desde la entrada estándar (`std::cin`), manteniendo el estado del cursor de lectura.
* **Reconocedor Léxico (`lexer.h`, `lexer.cpp`):**  
  Contiene la clase `Lexer`. Implementa los métodos `nextToken()` y `tokenizeAll()`, manteniendo un autómata finito determinista (AFD) implícito para el reconocimiento de patrones y una tabla asociativa estática para palabras reservadas.
* **Manejo de Errores:**  
  Genera tokens de error (`TOKEN_ERROR`) con la descripción del carácter o secuencia anómala sin provocar caídas del sistema (*crash*), permitiendo al analizador continuar reconociendo los componentes subsiguientes.

---

## 4. Requisitos y Compilación

### Requisitos Previos
* **Compilador C++:** Soporte para estándar **C++17** (GCC 9+, Clang 10+ o MSVC 2019+).
* **CMake:** Versión 3.15 o superior.
* **Sistema Operativo:** Linux, macOS o Windows (MinGW/MSVC).

### Instrucciones de Compilación con CMake

1. Clonar el repositorio o ubicarse en la raíz del proyecto:
   ```bash
   cd EDA-Lab04
   ```

2. Crear y acceder al directorio de construcción (*out-of-source build*):
   ```bash
   mkdir build
   cd build
   ```

3. Configurar el proyecto con CMake:
   ```bash
   cmake ..
   ```

4. Compilar los ejecutables del proyecto:
   ```bash
   cmake --build .
   ```

Esto generará dos ejecutables dentro del directorio `build/`:
* `sql_lexer` (o `sql_lexer.exe` en Windows): Aplicación principal.
* `test_lexer` (o `test_lexer.exe` en Windows): Suite de pruebas unitarias.

---

## 5. Instrucciones de Uso y Formato de Salida

### 5.1. Ejecución con Archivo SQL

Para analizar una consulta almacenada en un archivo:
```bash
./sql_lexer ../examples/valida.sql
```

### 5.2. Ejecución Interactiva (Consola)

Si no se pasa ningún argumento, el programa entra en modo interactivo leyendo de `std::cin`:
```bash
./sql_lexer
```
Ingrese la sentencia SQL y termine la entrada enviando `EOF` con:
* **Linux/macOS:** `Ctrl + D`
* **Windows:** `Ctrl + Z` seguido de `Enter`.

### 5.3. Ejemplo de Salida Formateada

Para la siguiente consulta de prueba (`examples/valida.sql`):
```sql
SELECT nombre, edad
FROM estudiantes
WHERE edad >= 18;
```

La salida producida por el analizador léxico es:
```text
[Línea: 1, Col:  1] KEYWORD_SELECT       : "SELECT"
[Línea: 1, Col:  8] IDENTIFIER           : "nombre"
[Línea: 1, Col: 14] COMMA                : ","
[Línea: 1, Col: 16] IDENTIFIER           : "edad"
[Línea: 2, Col:  1] KEYWORD_FROM         : "FROM"
[Línea: 2, Col:  6] IDENTIFIER           : "estudiantes"
[Línea: 3, Col:  1] KEYWORD_WHERE        : "WHERE"
[Línea: 3, Col:  7] IDENTIFIER           : "edad"
[Línea: 3, Col: 12] GREATER_EQUAL        : ">="
[Línea: 3, Col: 15] INTEGER_LITERAL      : "18"
[Línea: 3, Col: 17] SEMICOLON            : ";"
[Línea: 3, Col: 18] END_OF_FILE          : ""
```

---

## 6. Pruebas y Validación (Casos Obligatorios)

El proyecto cuenta con una batería de pruebas automatizadas en `tests/test_lexer.cpp` y ficheros de prueba en la carpeta `examples/` que cubren exhaustivamente los escenarios solicitados en el laboratorio:

| Escenario | Archivo de Prueba | Aspecto Verificado |
| :--- | :--- | :--- |
| **1. Sentencia válida** | `examples/valida.sql` | Consulta completa combinando `SELECT`, `FROM`, `WHERE`, identificadores y operadores. |
| **2. Operadores compuestos** | `examples/operadores.sql` | Reconocimiento atómico de `>=`, `<=`, `<>`, `!=` sin separarlos en operadores individuales. |
| **3. Literales** | `examples/literales.sql` | Enteros (`100`), decimales (`99.95`) y cadenas (`'Arequipa'`). |
| **4. Palabras e Identificadores**| `tests/test_lexer.cpp` | No confundir nombres como `select_col` o `wherever` con palabras reservadas. |
| **5. Espacios en blanco** | `examples/espacios.sql` | Saltos de línea y tabuladores múltiples que verifican el cómputo correcto de líneas y columnas. |
| **6. Errores léxicos** | `examples/errores.sql` | Detección de caracteres inválidos (`@`, `#`, `$`) y cadenas sin cerrar `'sin fin`. |
| **7. Casos límite** | `examples/limite.sql` | Archivo completamente vacío, consultas compuestas solo por espacios y terminación en `EOF`. |

### Ejecución de Pruebas Unitarias
Desde la carpeta `build/`:
```bash
./test_lexer
```
Todas las aserciones validan que tanto el tipo de token como la secuencia y posición concuerden con el resultado esperado.

---

## 7. Integración con Siguientes Etapas (Fase 2 - Parser)

El analizador léxico está diseñado como un componente autónomo mediante una interfaz limpia:
```cpp
Token Lexer::nextToken();
std::vector<Token> Lexer::tokenizeAll();
```

En las etapas posteriores, el analizador sintáctico consumirá los tokens generados mediante llamadas sucesivas o iterando sobre el vector de tokens, permitiendo construir el árbol de análisis sintáctico abstracto (AST) para validar la gramática de la sentencia `SELECT` sin acoplar la lectura de caracteres con el análisis gramatical.