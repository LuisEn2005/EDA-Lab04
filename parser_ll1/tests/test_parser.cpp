// Pruebas de la gramatica LL(1) y del motor por pila (Practica 5, parte A).
// Compilar y ejecutar:  cmake --build build && ./build/test_parser
//
// El motor se prueba con una TABLA DE REFERENCIA escrita a mano a partir de docs/gramatica.md,
// de modo que estas pruebas no dependen de la construccion automatica de la tabla.
// TODO (Persona B): agregar aqui las pruebas de la tabla construida, la traza y las consultas invalidas.

#include <functional>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "grammar.h"
#include "lexer.h"
#include "parse_table.h"
#include "parser.h"

namespace {

std::vector<std::string> currentFailures;
int totalTests = 0;
int failedTests = 0;
int totalChecks = 0;

#define EXPECT(cond)                                                          \
    do {                                                                      \
        ++totalChecks;                                                        \
        if (!(cond)) {                                                        \
            std::ostringstream os_;                                           \
            os_ << "linea " << __LINE__ << ": " << #cond;                     \
            currentFailures.push_back(os_.str());                             \
        }                                                                     \
    } while (0)

void runTest(const std::string& name, const std::function<void()>& body) {
    currentFailures.clear();
    ++totalTests;
    body();
    if (currentFailures.empty()) {
        std::cout << "  [PASS] " << name << "\n";
    } else {
        ++failedTests;
        std::cout << "  [FAIL] " << name << "\n";
        for (const auto& f : currentFailures) std::cout << "         -> " << f << "\n";
    }
}

void scenario(int n, const std::string& title) {
    std::cout << "\nEscenario " << n << ": " << title << "\n";
}

using namespace ll1;
using T = Terminal;
using N = NonTerminal;

// Tabla M[A, a] de docs/gramatica.md, seccion 7 (numero de produccion de la seccion 3).
ParseTable referenceTable() {
    struct Cell { NonTerminal a; Terminal t; int prod; };
    const std::vector<Cell> cells = {
        {N::QUERY, T::SELECT, 1},
        {N::SELECT_LIST, T::STAR, 2}, {N::SELECT_LIST, T::ID, 3},
        {N::ID_LIST_P, T::COMMA, 4},  {N::ID_LIST_P, T::FROM, 5},
        {N::WHERE_OPT, T::WHERE, 6},  {N::WHERE_OPT, T::SEMI, 7},
        {N::COND, T::ID, 8},
        {N::COND_P, T::OR, 9},        {N::COND_P, T::SEMI, 10},
        {N::AND_COND, T::ID, 11},
        {N::AND_P, T::AND, 12},       {N::AND_P, T::OR, 13},   {N::AND_P, T::SEMI, 13},
        {N::PRED, T::ID, 14},
        {N::REL_OP, T::EQ, 15}, {N::REL_OP, T::NEQ, 16}, {N::REL_OP, T::LT, 17},
        {N::REL_OP, T::GT, 18}, {N::REL_OP, T::LE, 19},  {N::REL_OP, T::GE, 20},
        {N::VALUE, T::INT, 21}, {N::VALUE, T::FLOAT, 22}, {N::VALUE, T::STRING, 23}, {N::VALUE, T::ID, 24},
    };
    ParseTable table;
    for (const auto& c : cells) table.cells[{c.a, c.t}] = c.prod - 1;
    return table;
}

const ParseTable REF = referenceTable();

// Lexer (Practica 4) + motor, tal como se integran.
ParseResult parseSql(const std::string& sql) {
    Lexer lexer(sql);
    return parseTokens(lexer.tokenizeAll(), REF);
}

bool accepts(const std::string& sql) { return parseSql(sql).accepted; }

std::set<Terminal> S(std::initializer_list<Terminal> l) { return std::set<Terminal>(l); }

bool expectedContains(const ParseResult& r, const std::string& what) {
    if (!r.error) return false;
    for (const auto& e : r.error->expected) if (e == what) return true;
    return false;
}

} // namespace

int main() {
    // ---------------------------------------------------------------- 1
    scenario(1, "Gramatica: estructura y simbolos");
    runTest("la gramatica tiene 24 producciones y simbolo inicial Query", [] {
        EXPECT(productions().size() == 24);
        EXPECT(startSymbol() == N::QUERY);
    });
    runTest("sin recursion izquierda directa", [] {
        for (const auto& p : productions()) {
            EXPECT(p.rhs.empty() || p.rhs[0].isTerminal || p.rhs[0].nonTerminal() != p.lhs);
        }
    });
    runTest("producciones con el texto documentado", [] {
        EXPECT(toString(productions()[0])  == "Query -> SELECT SelectList FROM id WhereOpt ;");
        EXPECT(toString(productions()[4])  == "IdListP -> eps");
        EXPECT(toString(productions()[11]) == "AndP -> AND Pred AndP");
        EXPECT(toString(productions()[23]) == "Value -> id");
    });

    // ---------------------------------------------------------------- 2
    scenario(2, "Conjuntos FIRST y FOLLOW (coinciden con docs/gramatica.md)");
    FirstFollowSets fs = computeFirstFollow();

    runTest("FIRST de cada no terminal", [&] {
        EXPECT(fs.first.at(N::QUERY)       == S({T::SELECT}));
        EXPECT(fs.first.at(N::SELECT_LIST) == S({T::STAR, T::ID}));
        EXPECT(fs.first.at(N::ID_LIST_P)   == S({T::COMMA}));
        EXPECT(fs.first.at(N::WHERE_OPT)   == S({T::WHERE}));
        EXPECT(fs.first.at(N::COND)        == S({T::ID}));
        EXPECT(fs.first.at(N::COND_P)      == S({T::OR}));
        EXPECT(fs.first.at(N::AND_COND)    == S({T::ID}));
        EXPECT(fs.first.at(N::AND_P)       == S({T::AND}));
        EXPECT(fs.first.at(N::PRED)        == S({T::ID}));
        EXPECT(fs.first.at(N::REL_OP)      == S({T::EQ, T::NEQ, T::LT, T::GT, T::LE, T::GE}));
        EXPECT(fs.first.at(N::VALUE)       == S({T::INT, T::FLOAT, T::STRING, T::ID}));
    });
    runTest("no terminales anulables (derivan epsilon)", [&] {
        std::set<NonTerminal> expected = {N::ID_LIST_P, N::WHERE_OPT, N::COND_P, N::AND_P};
        EXPECT(fs.nullable == expected);
    });
    runTest("FOLLOW de cada no terminal", [&] {
        EXPECT(fs.follow.at(N::QUERY)       == S({T::END}));
        EXPECT(fs.follow.at(N::SELECT_LIST) == S({T::FROM}));
        EXPECT(fs.follow.at(N::ID_LIST_P)   == S({T::FROM}));
        EXPECT(fs.follow.at(N::WHERE_OPT)   == S({T::SEMI}));
        EXPECT(fs.follow.at(N::COND)        == S({T::SEMI}));
        EXPECT(fs.follow.at(N::COND_P)      == S({T::SEMI}));
        EXPECT(fs.follow.at(N::AND_COND)    == S({T::OR, T::SEMI}));
        EXPECT(fs.follow.at(N::AND_P)       == S({T::OR, T::SEMI}));
        EXPECT(fs.follow.at(N::PRED)        == S({T::AND, T::OR, T::SEMI}));
        EXPECT(fs.follow.at(N::REL_OP)      == S({T::ID, T::INT, T::FLOAT, T::STRING}));
        EXPECT(fs.follow.at(N::VALUE)       == S({T::AND, T::OR, T::SEMI}));
    });
    runTest("conjuntos de seleccion = celdas de la tabla de referencia (LL(1) sin conflictos)", [&] {
        // SELECCION(A -> alpha) = FIRST(alpha), mas FOLLOW(A) si alpha deriva epsilon.
        // Cada produccion debe ocupar exactamente esas celdas, y ninguna otra.
        const auto& prods = productions();
        size_t totalCells = 0;
        for (size_t i = 0; i < prods.size(); ++i) {
            bool nullable = false;
            std::set<Terminal> sel = firstOfSequence(prods[i].rhs, fs, nullable);
            if (nullable) sel.insert(fs.follow.at(prods[i].lhs).begin(), fs.follow.at(prods[i].lhs).end());
            std::set<Terminal> inTable;
            for (int j = 0; j < kNumTerminals; ++j) {
                auto t = static_cast<Terminal>(j);
                if (REF.lookup(prods[i].lhs, t) == static_cast<int>(i)) inTable.insert(t);
            }
            EXPECT(sel == inTable);
            totalCells += sel.size();
        }
        EXPECT(totalCells == REF.cells.size());   // 25: ninguna celda reclamada por dos producciones
    });

    // ---------------------------------------------------------------- 3
    scenario(3, "Motor por pila: consultas validas simples");
    runTest("SELECT * FROM t;", [] { EXPECT(accepts("SELECT * FROM t;")); });
    runTest("SELECT a FROM t;", [] { EXPECT(accepts("SELECT a FROM t;")); });
    runTest("varias columnas", [] { EXPECT(accepts("SELECT a, b, c FROM t;")); });
    runTest("palabras clave en minusculas y mezcla", [] {
        EXPECT(accepts("select a from t;"));
        EXPECT(accepts("SeLeCt * FrOm t Where x = 1;"));
    });
    runTest("saltos de linea y tabuladores", [] {
        EXPECT(accepts("SELECT\n\ta,\n\tb\nFROM\n\tt\n;"));
    });
    runTest("WHERE con una comparacion", [] { EXPECT(accepts("SELECT a FROM t WHERE x = 1;")); });

    scenario(4, "Motor por pila: consultas validas complejas");
    runTest("todos los operadores relacionales", [] {
        for (const char* op : {"=", "<>", "!=", "<", ">", "<=", ">="}) {
            EXPECT(accepts(std::string("SELECT a FROM t WHERE x ") + op + " 1;"));
        }
    });
    runTest("todos los tipos de valor", [] {
        EXPECT(accepts("SELECT a FROM t WHERE x = 10;"));
        EXPECT(accepts("SELECT a FROM t WHERE x = 10.5;"));
        EXPECT(accepts("SELECT a FROM t WHERE x = -3;"));
        EXPECT(accepts("SELECT a FROM t WHERE x = 'texto';"));
        EXPECT(accepts("SELECT a FROM t WHERE x = y;"));
    });
    runTest("AND, OR y combinaciones", [] {
        EXPECT(accepts("SELECT a FROM t WHERE x = 1 AND y = 2;"));
        EXPECT(accepts("SELECT a FROM t WHERE x = 1 OR y = 2;"));
        EXPECT(accepts("SELECT a FROM t WHERE x = 1 AND y = 2 OR z = 3 AND w = 4 OR v = 5;"));
    });
    runTest("proyeccion multiple + WHERE compuesto (archivo de ejemplo)", [] {
        EXPECT(accepts("SELECT nombre, edad, carrera\n"
                       "FROM estudiantes\n"
                       "WHERE edad >= 18 AND carrera = 'CC' OR promedio <> 10.5 AND codigo != 7;"));
    });
    runTest("entrada larga: 500 columnas y 200 condiciones", [] {
        std::string sql = "SELECT c0";
        for (int i = 1; i < 500; ++i) sql += ", c" + std::to_string(i);
        sql += " FROM t WHERE x0 = 0";
        for (int i = 1; i < 200; ++i) sql += " AND x" + std::to_string(i) + " = " + std::to_string(i);
        sql += ";";
        EXPECT(accepts(sql));
    });

    // ---------------------------------------------------------------- 5
    scenario(5, "Motor por pila: comportamiento del algoritmo");
    runTest("producciones aplicadas en orden (derivacion por la izquierda)", [] {
        ParseResult r = parseSql("SELECT a FROM t;");
        std::vector<int> expected = {1, 3, 5, 7};   // Query, SelectList->id IdListP, IdListP->eps, WhereOpt->eps
        EXPECT(r.accepted);
        EXPECT(r.productions == expected);
    });
    runTest("el observador recibe un evento por paso (10 pasos) y el ultimo es ACEPTAR", [] {
        Lexer lexer("SELECT a FROM t;");
        std::vector<std::string> actions;
        std::vector<size_t> stackSizes;
        ParseResult r = parseTokens(lexer.tokenizeAll(), REF, [&](const StepEvent& e) {
            EXPECT(e.step == static_cast<int>(actions.size()) + 1);
            actions.push_back(e.action);
            stackSizes.push_back(e.stack.size());
        });
        EXPECT(r.accepted);
        EXPECT(actions.size() == 10);
        EXPECT(actions.front() == "Query -> SELECT SelectList FROM id WhereOpt ;");
        EXPECT(actions[1] == "match SELECT");
        EXPECT(actions.back() == "ACEPTAR");
        EXPECT(stackSizes.front() == 2);   // Query $
        EXPECT(stackSizes.back() == 1);    // solo '$'
    });
    runTest("la pila termina vacia: solo '$' en el fondo al aceptar", [] {
        Lexer lexer("SELECT * FROM t WHERE x = 1;");
        size_t last = 99;
        parseTokens(lexer.tokenizeAll(), REF, [&](const StepEvent& e) { last = e.stack.size(); });
        EXPECT(last == 1);
    });

    // ---------------------------------------------------------------- 6
    scenario(6, "Motor por pila: reporte basico de error (todo o nada)");
    runTest("se detiene en el primer error: linea, columna y token encontrado", [] {
        ParseResult r = parseSql("SELECT a FROM ;");
        EXPECT(!r.accepted);
        EXPECT(r.error.has_value());
        EXPECT(r.error->line == 1 && r.error->column == 15);
        EXPECT(r.error->found == ";");
        EXPECT(expectedContains(r, "identificador"));
    });
    runTest("mensaje completo con posicion, token y esperados", [] {
        ParseResult r = parseSql("SELECT a FROM ;");
        EXPECT(r.error->message ==
               "Error sintactico en 1:15: se encontro ';' pero se esperaba identificador");
    });
    runTest("terminal esperado distinto del token (FROM antes de SELECT)", [] {
        ParseResult r = parseSql("FROM t SELECT a;");
        EXPECT(!r.accepted);
        EXPECT(r.error->line == 1 && r.error->column == 1);
        EXPECT(r.error->expected.size() == 1 && r.error->expected[0] == "SELECT");
    });
    runTest("fin de entrada inesperado (falta ';') informa los tokens esperados", [] {
        ParseResult r = parseSql("SELECT a FROM t");
        EXPECT(!r.accepted);
        EXPECT(r.error->found == "fin de entrada");
        EXPECT(expectedContains(r, "WHERE"));
        EXPECT(expectedContains(r, "';'"));
    });
    runTest("posicion correcta en entrada multilinea", [] {
        ParseResult r = parseSql("SELECT a\nFROM t\nWHERE x >;");
        EXPECT(!r.accepted);
        EXPECT(r.error->line == 3 && r.error->column == 10);
    });
    runTest("tokens despues del ';' se rechazan", [] {
        EXPECT(!accepts("SELECT a FROM t; SELECT b FROM u;"));
    });

    // ---------------------------------------------------------------- 7
    scenario(7, "Motor por pila: robustez e integracion con el lexer");
    runTest("entrada vacia se rechaza sin fallar", [] {
        ParseResult r = parseSql("");
        EXPECT(!r.accepted);
        EXPECT(r.error->found == "fin de entrada");
        EXPECT(expectedContains(r, "SELECT"));
    });
    runTest("solo espacios, y un unico token", [] {
        EXPECT(!accepts("  \n\t \n"));
        EXPECT(!accepts("SELECT"));
        EXPECT(!accepts(";"));
        EXPECT(!accepts("*"));
    });
    runTest("vector de tokens vacio (sin EOF) no se sale de rango", [] {
        ParseResult r = parseTokens({}, REF);
        EXPECT(!r.accepted);
        EXPECT(r.error.has_value());
    });
    runTest("vector sin TOKEN_EOF final se trata como fin de entrada", [] {
        Lexer lexer("SELECT * FROM t;");
        auto tokens = lexer.tokenizeAll();
        tokens.pop_back();                                  // quitar EOF
        EXPECT(parseTokens(tokens, REF).accepted);
        tokens.pop_back();                                  // quitar tambien ';'
        EXPECT(!parseTokens(tokens, REF).accepted);
    });
    runTest("correspondencia TokenType -> Terminal", [] {
        EXPECT(terminalFromToken(TokenType::KEYWORD_SELECT) == T::SELECT);
        EXPECT(terminalFromToken(TokenType::IDENTIFIER) == T::ID);
        EXPECT(terminalFromToken(TokenType::INTEGER_LITERAL) == T::INT);
        EXPECT(terminalFromToken(TokenType::FLOAT_LITERAL) == T::FLOAT);
        EXPECT(terminalFromToken(TokenType::STRING_LITERAL) == T::STRING);
        EXPECT(terminalFromToken(TokenType::NOT_EQUAL) == T::NEQ);
        EXPECT(terminalFromToken(TokenType::STAR) == T::STAR);
        EXPECT(terminalFromToken(TokenType::TOKEN_EOF) == T::END);
    });
    runTest("el lexer no se modifica: != y <> son el mismo terminal", [] {
        EXPECT(accepts("SELECT a FROM t WHERE x != 1 AND y <> 2;"));
    });
    runTest("decimales normalizados por el lexer (.5 y 5.) son validos", [] {
        EXPECT(accepts("SELECT a FROM t WHERE x = .5 AND y = 5.;"));
    });
    runTest("palabras reservadas no pueden usarse como identificador", [] {
        EXPECT(!accepts("SELECT select FROM t;"));
        EXPECT(!accepts("SELECT a FROM where;"));
    });
    runTest("cadena con escapes cuenta como un solo valor", [] {
        EXPECT(accepts("SELECT a FROM t WHERE n = 'it\\'s';"));
    });

    std::cout << "\n=================================\n";
    std::cout << "Pruebas ejecutadas: " << totalTests << " | verificaciones: " << totalChecks
              << " | fallidas: " << failedTests << "\n";
    std::cout << (failedTests == 0 ? "RESULTADO: TODAS LAS PRUEBAS PASARON\n"
                                   : "RESULTADO: HAY PRUEBAS FALLIDAS\n");
    return failedTests == 0 ? 0 : 1;
}
