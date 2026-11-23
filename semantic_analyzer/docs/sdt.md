# Esquema de Traducción Dirigida por la Sintaxis (SDT)

## 1. Propósito

Conectar las 24 producciones de la gramática LL(1) de la Práctica 5 (`parser_ll1/src/grammar.cpp`) con la
construcción del AST de la Práctica 6 (`semantic_analyzer/include/ast.h`), propagando la posición
(`line`, `column`) de cada token para que los errores semánticos indiquen dónde ocurrieron.

## 2. Atributos

| Atributo | Clase | No terminales | Contenido |
|---|---|---|---|
| `lista` | sintetizado | `SelectList` | vector de nodos de la proyección (`StarNode` o `ColumnNode`) |
| `her` (lista) | heredado | `IdListP` | columnas acumuladas hasta el momento |
| `sin` (lista) | sintetizado | `IdListP` | lista final de columnas |
| `nodo` | sintetizado | `Query`, `WhereOpt`, `Cond`, `AndCond`, `Pred`, `Value` | subárbol construido (`SelectStmtNode` o `ExprNode`) |
| `her` (nodo) | heredado | `CondP`, `AndP` | subárbol izquierdo ya construido |
| `sin` (nodo) | sintetizado | `CondP`, `AndP` | subárbol completo tras consumir la cola |
| `op` | sintetizado | `RelOp` | `BinaryOp` (`EQ`, `NEQ`, `LT`, `GT`, `LE`, `GE`) |
| `pos` | sintetizado | todos los terminales | `(line, column)` tomada del `Token` |

Los atributos heredados de `IdListP`, `CondP` y `AndP` existen porque la gramática eliminó la recursión por la
izquierda: la cola recibe el subárbol izquierdo y devuelve el árbol completo. Con esto `a AND b AND c` queda
`AND(AND(a, b), c)` (asociativo a la izquierda) y la precedencia `AND` sobre `OR` surge de los niveles
`Cond → AndCond` / `AndCond → Pred`.

## 3. Reglas semánticas por producción

Notación: `ID.lexeme`, `ID.pos` vienen del token; `Nodo(...)` es el constructor del AST con `(line, column)` primero.

| # | Producción | Regla semántica |
|---|---|---|
| 1 | `Query → SELECT SelectList FROM id WhereOpt ;` | `s = SelectStmtNode(SELECT.pos)`; `s.selectList = SelectList.lista`; `s.fromTable = TableRefNode(id.pos, id.lexeme)`; `s.whereClause = WhereOpt.nodo`; `Query.nodo = s` |
| 2 | `SelectList → *` | `SelectList.lista = [ StarNode(*.pos) ]` |
| 3 | `SelectList → id IdListP` | `IdListP.her = [ ColumnNode(id.pos, id.lexeme) ]`; `SelectList.lista = IdListP.sin` |
| 4 | `IdListP → , id IdListP₁` | `IdListP₁.her = IdListP.her ++ [ ColumnNode(id.pos, id.lexeme) ]`; `IdListP.sin = IdListP₁.sin` |
| 5 | `IdListP → ε` | `IdListP.sin = IdListP.her` |
| 6 | `WhereOpt → WHERE Cond` | `WhereOpt.nodo = Cond.nodo` |
| 7 | `WhereOpt → ε` | `WhereOpt.nodo = nullptr` |
| 8 | `Cond → AndCond CondP` | `CondP.her = AndCond.nodo`; `Cond.nodo = CondP.sin` |
| 9 | `CondP → OR AndCond CondP₁` | `CondP₁.her = BinaryExprNode(OR.pos, OR, CondP.her, AndCond.nodo)`; `CondP.sin = CondP₁.sin` |
| 10 | `CondP → ε` | `CondP.sin = CondP.her` |
| 11 | `AndCond → Pred AndP` | `AndP.her = Pred.nodo`; `AndCond.nodo = AndP.sin` |
| 12 | `AndP → AND Pred AndP₁` | `AndP₁.her = BinaryExprNode(AND.pos, AND, AndP.her, Pred.nodo)`; `AndP.sin = AndP₁.sin` |
| 13 | `AndP → ε` | `AndP.sin = AndP.her` |
| 14 | `Pred → id RelOp Value` | `Pred.nodo = BinaryExprNode(RelOp.pos, RelOp.op, ColumnNode(id.pos, id.lexeme), Value.nodo)` |
| 15 | `RelOp → =` | `RelOp.op = EQ` |
| 16 | `RelOp → <>` | `RelOp.op = NEQ` (el lexer también emite `NOT_EQUAL` para `!=`) |
| 17 | `RelOp → <` | `RelOp.op = LT` |
| 18 | `RelOp → >` | `RelOp.op = GT` |
| 19 | `RelOp → <=` | `RelOp.op = LE` |
| 20 | `RelOp → >=` | `RelOp.op = GE` |
| 21 | `Value → int` | `Value.nodo = LiteralNode(int.pos, int.value, INT)` |
| 22 | `Value → float` | `Value.nodo = LiteralNode(float.pos, float.value, FLOAT)` |
| 23 | `Value → string` | `Value.nodo = LiteralNode(string.pos, string.value, VARCHAR)` |
| 24 | `Value → id` | `Value.nodo = ColumnNode(id.pos, id.lexeme)` |

Convención de posición: un `BinaryExprNode` toma la posición de su **operador** (`=`, `<`, `AND`, ...), de modo que
un `TYPE_MISMATCH` apunta al operador conflictivo. `ColumnNode`, `TableRefNode` y `LiteralNode` toman la posición
de su propio token.

## 4. Cómo se evalúa sobre el parser existente

El parser LL(1) (`ll1::parseTokens`) ya devuelve `ParseResult::productions`: los números de producción aplicados
en orden (derivación por la izquierda). El constructor del AST recorre esa lista con un método recursivo por no
terminal, que consume la siguiente producción y los tokens que le corresponden. Los atributos heredados son
parámetros de esos métodos y los sintetizados son sus valores de retorno. Así no se modifica el lexer ni el parser,
y las pruebas de las Prácticas 4 y 5 siguen intactas.

## 5. Ejemplo

`SELECT nombre FROM usuarios WHERE edad >= 18 AND saldo > 0.5;` produce:

```
SelectStmtNode
├─ selectList: ColumnNode(nombre)
├─ fromTable:  TableRefNode(usuarios)
└─ whereClause:
   BinaryExprNode(AND)
   ├─ BinaryExprNode(>=)
   │  ├─ ColumnNode(edad)
   │  └─ LiteralNode(18, INT)
   └─ BinaryExprNode(>)
      ├─ ColumnNode(saldo)
      └─ LiteralNode(0.5, FLOAT)
```
