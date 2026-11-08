# Práctica 5 — Gramática LL(1) para SELECT

> FIRST y FOLLOW también se calculan en código (`grammar.cpp`) y `test_parser` verifica que coinciden con este documento y con la tabla de la sección 7.

## 1. Terminales (salida del Lexer de la Práctica 4)

| Terminal en la gramática | TokenType del lexer |
|---|---|
| SELECT, FROM, WHERE, AND, OR | KEYWORD_SELECT, KEYWORD_FROM, KEYWORD_WHERE, KEYWORD_AND, KEYWORD_OR |
| id | IDENTIFIER |
| int, float, string | INTEGER_LITERAL, FLOAT_LITERAL, STRING_LITERAL |
| =, <>, <, >, <=, >= | EQUAL, NOT_EQUAL (`<>` y `!=`), LESS_THAN, GREATER_THAN, LESS_EQUAL, GREATER_EQUAL |
| `,` y `;` | COMMA, SEMICOLON |
| STAR (`*`) | STAR |
| $ | TOKEN_EOF |

## 2. Gramática en EBNF (versión legible)

```
Query     ::= SELECT SelectList FROM id [ WHERE Cond ] ;
SelectList::= * | id { , id }
Cond      ::= AndCond { OR AndCond }
AndCond   ::= Pred { AND Pred }
Pred      ::= id RelOp Value
RelOp     ::= = | <> | < | > | <= | >=
Value     ::= int | float | string | id
```

AND tiene mayor precedencia que OR (por eso hay dos niveles: Cond / AndCond).

## 3. Gramática BNF LL(1) (sin recursión izquierda, factorizada)

Las repeticiones `{ ... }` se convierten en no terminales con cola (P) y ε:

| N.º | Producción |
|---|---|
| 1 | Query → SELECT SelectList FROM id WhereOpt ; |
| 2 | SelectList → STAR |
| 3 | SelectList → id IdListP |
| 4 | IdListP → , id IdListP |
| 5 | IdListP → ε |
| 6 | WhereOpt → WHERE Cond |
| 7 | WhereOpt → ε |
| 8 | Cond → AndCond CondP |
| 9 | CondP → OR AndCond CondP |
| 10 | CondP → ε |
| 11 | AndCond → Pred AndP |
| 12 | AndP → AND Pred AndP |
| 13 | AndP → ε |
| 14 | Pred → id RelOp Value |
| 15 | RelOp → = |
| 16 | RelOp → <> |
| 17 | RelOp → < |
| 18 | RelOp → > |
| 19 | RelOp → <= |
| 20 | RelOp → >= |
| 21 | Value → int |
| 22 | Value → float |
| 23 | Value → string |
| 24 | Value → id |

Símbolo inicial: **Query**. Se parsea con `$` al final de la entrada.

Por qué cumple las restricciones:
- **Sin recursión izquierda**: las listas y los AND/OR usan colas (IdListP, CondP, AndP) recursivas por la derecha.
- **Factorizada**: SelectList se decide por el primer token (STAR vs id); no hay prefijos comunes.
- **Sin conflictos**: ver conjuntos de selección abajo.

## 4. Conjuntos FIRST

| No terminal | FIRST |
|---|---|
| Query | { SELECT } |
| SelectList | { STAR, id } |
| IdListP | { `,`, ε } |
| WhereOpt | { WHERE, ε } |
| Cond | { id } |
| CondP | { OR, ε } |
| AndCond | { id } |
| AndP | { AND, ε } |
| Pred | { id } |
| RelOp | { =, <>, <, >, <=, >= } |
| Value | { int, float, string, id } |

## 5. Conjuntos FOLLOW

| No terminal | FOLLOW | Razón |
|---|---|---|
| Query | { $ } | símbolo inicial |
| SelectList | { FROM } | prod. 1 |
| IdListP | { FROM } | hereda FOLLOW(SelectList) (prod. 3) |
| WhereOpt | { ; } | prod. 1 |
| Cond | { ; } | hereda FOLLOW(WhereOpt) (prod. 6) |
| CondP | { ; } | hereda FOLLOW(Cond) (prod. 8) |
| AndCond | { OR, ; } | FIRST(CondP) − ε, y FOLLOW(Cond)/FOLLOW(CondP) por ε |
| AndP | { OR, ; } | hereda FOLLOW(AndCond) (prod. 11) |
| Pred | { AND, OR, ; } | FIRST(AndP) − ε, y FOLLOW(AndCond) por ε |
| RelOp | { int, float, string, id } | FIRST(Value) (prod. 14) |
| Value | { AND, OR, ; } | hereda FOLLOW(Pred) (prod. 14) |

## 6. Verificación LL(1) (conjuntos de selección)

Solo los no terminales con alternativas pueden tener conflicto:

| No terminal | Alternativas | Selección | ¿Disjuntos? |
|---|---|---|---|
| SelectList | 2 / 3 | {STAR} / {id} | Sí |
| IdListP | 4 / 5 | {`,`} / FOLLOW={FROM} | Sí |
| WhereOpt | 6 / 7 | {WHERE} / FOLLOW={;} | Sí |
| CondP | 9 / 10 | {OR} / {;} | Sí |
| AndP | 12 / 13 | {AND} / {OR, ;} | Sí |
| RelOp | 15–20 | un operador distinto cada una | Sí |
| Value | 21–24 | {int}, {float}, {string}, {id} | Sí |

## 7. Tabla de parseo M[A, a]

Cada celda es el N.º de producción de la sección 3. Celda vacía = error sintáctico.

| | SELECT | FROM | WHERE | AND | OR | id | int | float | string | = | <> | < | > | <= | >= | `,` | `;` | STAR | $ |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| **Query** | 1 | | | | | | | | | | | | | | | | | | |
| **SelectList** | | | | | | 3 | | | | | | | | | | | | 2 | |
| **IdListP** | | 5 | | | | | | | | | | | | | | 4 | | | |
| **WhereOpt** | | | 6 | | | | | | | | | | | | | | 7 | | |
| **Cond** | | | | | | 8 | | | | | | | | | | | | | |
| **CondP** | | | | | 9 | | | | | | | | | | | | 10 | | |
| **AndCond** | | | | | | 11 | | | | | | | | | | | | | |
| **AndP** | | | | 12 | 13 | | | | | | | | | | | | 13 | | |
| **Pred** | | | | | | 14 | | | | | | | | | | | | | |
| **RelOp** | | | | | | | | | | 15 | 16 | 17 | 18 | 19 | 20 | | | | |
| **Value** | | | | | | 24 | 21 | 22 | 23 | | | | | | | | | | |

## 8. Ejemplo de traza esperada (para el informe)

Entrada: `SELECT a FROM t WHERE x = 1;`

| Paso | Pila (tope a la izquierda) | Entrada | Acción |
|---|---|---|---|
| 1 | Query $ | SELECT a FROM t WHERE x = 1 ; $ | Query → SELECT SelectList FROM id WhereOpt ; |
| 2 | SELECT SelectList FROM id WhereOpt ; $ | SELECT a ... | match SELECT |
| 3 | SelectList FROM id WhereOpt ; $ | a FROM ... | SelectList → id IdListP |
| 4 | id IdListP FROM id WhereOpt ; $ | a FROM ... | match id |
| 5 | IdListP FROM id WhereOpt ; $ | FROM t ... | IdListP → ε |
| 6 | FROM id WhereOpt ; $ | FROM t ... | match FROM |
| 7 | id WhereOpt ; $ | t WHERE ... | match id |
| 8 | WhereOpt ; $ | WHERE x = 1 ; $ | WhereOpt → WHERE Cond |
| ... | ... | ... | (el motor genera el resto; verificar con el código) |
