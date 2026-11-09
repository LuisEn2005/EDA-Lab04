// Pruebas de la Practica 5, parte B: tabla M[A,a] construida, traza y consultas invalidas.
// Usa la tabla REAL (defaultParseTable), construida automaticamente desde FIRST/FOLLOW.
// Ejecutar:  ./build/test_parser_tabla

#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "grammar.h"
#include "lexer.h"
#include "parse_table.h"
#include "parser.h"
#include "trace.h"

namespace {

std::vector<std::string> currentFailures;
int totalTests = 0, failedTests = 0, totalChecks = 0;

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

void scenario(int n, const std::string& title) { std::cout << "\nEscenario " << n << ": " << title << "\n"; }

using namespace ll1;
using T = Terminal;
using N = NonTerminal;

const ParseTable& TABLE = defaultParseTable();

ParseResult parseSql(const std::string& sql) {
    Lexer lexer(sql);
    return parseTokens(lexer.tokenizeAll(), TABLE);
}

bool accepts(const std::string& sql) { return parseSql(sql).accepted; }

bool expectedContains(const ParseResult& r, const std::string& what) {
    if (!r.error) return false;
    for (const auto& e : r.error->expected) if (e == what) return true;
    return false;
}

// Una consulta invalida debe ser rechazada, con error, sin fallar. Devuelve el resultado.
ParseResult rejects(const std::string& sql) {
    ParseResult r = parseSql(sql);
    EXPECT(!r.accepted);
    EXPECT(r.error.has_value());
    return r;
}

} // namespace

int main() {
    // ------------------------------------------------------------------
    scenario(1, "Tabla M[A, a] construida desde FIRST/FOLLOW");
    runTest("la gramatica es LL(1): la tabla no tiene conflictos", [] {
        EXPECT(TABLE.isLL1());
        EXPECT(TABLE.conflicts.empty());
    });
    runTest("la tabla tiene 25 celdas no vacias", [] { EXPECT(TABLE.cells.size() == 25); });
    runTest("celdas de docs/gramatica.md (seccion 7)", [] {
        struct Cell { N a; T t; int prod; };
        const std::vector<Cell> expected = {
            {N::QUERY, T::SELECT, 1},
            {N::SELECT_LIST, T::STAR, 2}, {N::SELECT_LIST, T::ID, 3},
            {N::ID_LIST_P, T::COMMA, 4},  {N::ID_LIST_P, T::FROM, 5},
            {N::WHERE_OPT, T::WHERE, 6},  {N::WHERE_OPT, T::SEMI, 7},
            {N::COND, T::ID, 8},
            {N::COND_P, T::OR, 9},        {N::COND_P, T::SEMI, 10},
            {N::AND_COND, T::ID, 11},
            {N::AND_P, T::AND, 12},       {N::AND_P, T::OR, 13}, {N::AND_P, T::SEMI, 13},
            {N::PRED, T::ID, 14},
            {N::REL_OP, T::EQ, 15}, {N::REL_OP, T::NEQ, 16}, {N::REL_OP, T::LT, 17},
            {N::REL_OP, T::GT, 18}, {N::REL_OP, T::LE, 19},  {N::REL_OP, T::GE, 20},
            {N::VALUE, T::INT, 21}, {N::VALUE, T::FLOAT, 22}, {N::VALUE, T::STRING, 23}, {N::VALUE, T::ID, 24},
        };
        for (const auto& c : expected) EXPECT(TABLE.lookup(c.a, c.t) == c.prod - 1);
    });
    runTest("las celdas vacias devuelven -1", [] {
        EXPECT(TABLE.lookup(N::QUERY, T::FROM) == -1);
        EXPECT(TABLE.lookup(N::SELECT_LIST, T::COMMA) == -1);
        EXPECT(TABLE.lookup(N::VALUE, T::SEMI) == -1);
        EXPECT(TABLE.lookup(N::WHERE_OPT, T::END) == -1);
    });
    runTest("expected() lista los terminales de la fila", [] {
        std::vector<Terminal> e = TABLE.expected(N::SELECT_LIST);
        EXPECT(e.size() == 2);                       // id y *
        EXPECT(TABLE.expected(N::REL_OP).size() == 6);
    });
    runTest("una gramatica ambigua SI produce conflictos (el detector funciona)", [] {
        // Se altera FOLLOW(WhereOpt) para forzar que 'WHERE' tambien dispare la produccion vacia 7.
        FirstFollowSets sets = computeFirstFollow();
        sets.follow[N::WHERE_OPT].insert(T::WHERE);
        ParseTable bad = buildParseTable(sets);
        EXPECT(!bad.isLL1());
        EXPECT(!bad.conflicts.empty());
    });
    runTest("printParseTable imprime la matriz y 'Sin conflictos'", [] {
        std::ostringstream os;
        printParseTable(os, TABLE);
        EXPECT(os.str().find("SelectList") != std::string::npos);
        EXPECT(os.str().find("Sin conflictos") != std::string::npos);
    });

    // ------------------------------------------------------------------
    scenario(2, "Consultas validas con la tabla construida");
    runTest("simples", [] {
        EXPECT(accepts("SELECT * FROM t;"));
        EXPECT(accepts("SELECT a FROM t;"));
        EXPECT(accepts("SELECT a, b, c FROM t;"));
    });
    runTest("complejas", [] {
        EXPECT(accepts("SELECT nombre, edad FROM estudiantes WHERE edad >= 18 AND carrera = 'CC' "
                       "OR promedio <> 10.5 AND codigo != 7;"));
        EXPECT(accepts("select a from t where x = y;"));
    });

    // ------------------------------------------------------------------
    scenario(3, "Traza de la pila: paso, pila, entrada y accion");
    runTest("traza de SELECT a FROM t WHERE x = 1;", [] {
        Lexer lexer("SELECT a FROM t WHERE x = 1;");
        std::ostringstream os;
        ParseResult r = parseTokens(lexer.tokenizeAll(), TABLE, makeTracer(os));
        EXPECT(r.accepted);
        const std::string out = os.str();
        std::istringstream in(out);
        std::string line;
        int lines = 0;
        while (std::getline(in, line)) ++lines;
        EXPECT(lines == 21);                                           // una fila por paso
        EXPECT(out.find("Query -> SELECT SelectList FROM id WhereOpt ;") != std::string::npos);
        EXPECT(out.find("WhereOpt -> WHERE Cond") != std::string::npos);
        EXPECT(out.find("IdListP -> eps") != std::string::npos);
        EXPECT(out.find("ACEPTAR") != std::string::npos);
    });
    runTest("stackToString: tope a la izquierda", [] {
        std::vector<Symbol> st = {Symbol::T(T::END), Symbol::N(N::QUERY)};
        EXPECT(stackToString(st) == "Query $");
    });
    runTest("remainingInput: entrada restante termina en $", [] {
        Lexer lexer("SELECT a FROM t;");
        std::vector<Token> toks = lexer.tokenizeAll();
        EXPECT(remainingInput(toks, 0) == "SELECT a FROM t ; $");
        EXPECT(remainingInput(toks, 2) == "FROM t ; $");
        EXPECT(remainingInput(toks, toks.size()) == "$");
    });
    runTest("la traza de una consulta invalida termina en ERROR", [] {
        Lexer lexer("SELECT FROM t;");
        std::ostringstream os;
        ParseResult r = parseTokens(lexer.tokenizeAll(), TABLE, makeTracer(os));
        EXPECT(!r.accepted);
        EXPECT(os.str().find("ERROR") != std::string::npos);
    });

    // ------------------------------------------------------------------
    scenario(4, "Errores: palabras clave fuera de orden");
    runTest("FROM antes de SELECT", [] {
        ParseResult r = rejects("FROM t SELECT a;");
        EXPECT(r.error->line == 1 && r.error->column == 1);
        EXPECT(r.error->found == "FROM");
        EXPECT(expectedContains(r, "SELECT"));
    });
    runTest("WHERE antes de FROM", [] {
        ParseResult r = rejects("SELECT a WHERE x = 1 FROM t;");
        EXPECT(r.error->found == "WHERE");
        EXPECT(expectedContains(r, "','") && expectedContains(r, "FROM"));
    });
    runTest("sin SELECT", [] { rejects("a FROM t;"); });

    // ------------------------------------------------------------------
    scenario(5, "Errores: omision de tokens obligatorios");
    runTest("falta la tabla despues de FROM", [] {
        ParseResult r = rejects("SELECT a FROM ;");
        EXPECT(r.error->found == ";");
        EXPECT(r.error->column == 15);
        EXPECT(expectedContains(r, "identificador"));
    });
    runTest("falta la tabla y no hay mas entrada", [] {
        ParseResult r = rejects("SELECT a FROM");
        EXPECT(r.error->found == "fin de entrada");
        EXPECT(expectedContains(r, "identificador"));
    });
    runTest("falta ';' al final", [] {
        ParseResult r = rejects("SELECT a FROM t");
        EXPECT(r.error->found == "fin de entrada");
        EXPECT(expectedContains(r, "';'") && expectedContains(r, "WHERE"));
    });
    runTest("falta ';' despues de WHERE", [] {
        ParseResult r = rejects("SELECT a FROM t WHERE x = 1");
        EXPECT(r.error->found == "fin de entrada");
        EXPECT(expectedContains(r, "';'") && expectedContains(r, "AND") && expectedContains(r, "OR"));
    });
    runTest("falta la lista de columnas", [] {
        ParseResult r = rejects("SELECT FROM t;");
        EXPECT(r.error->found == "FROM");
        EXPECT(expectedContains(r, "identificador") && expectedContains(r, "'*'"));
    });
    runTest("falta FROM", [] {
        ParseResult r = rejects("SELECT a t;");
        EXPECT(r.error->found == "t");
        EXPECT(expectedContains(r, "FROM"));
    });
    runTest("coma sobrante antes de FROM", [] {
        ParseResult r = rejects("SELECT a, FROM t;");
        EXPECT(r.error->found == "FROM");
        EXPECT(expectedContains(r, "identificador"));
    });
    runTest("falta coma entre columnas", [] { rejects("SELECT a b FROM t;"); });
    runTest("'*' mezclado con columnas", [] { rejects("SELECT *, a FROM t;"); });

    // ------------------------------------------------------------------
    scenario(6, "Errores: expresiones mal formadas");
    runTest("falta el valor de la comparacion", [] {
        ParseResult r = rejects("SELECT a FROM t WHERE x >= ;");
        EXPECT(r.error->found == ";");
        EXPECT(expectedContains(r, "entero") && expectedContains(r, "cadena"));
    });
    runTest("falta el operador relacional", [] {
        ParseResult r = rejects("SELECT a FROM t WHERE x 1;");
        EXPECT(r.error->found == "1");
        EXPECT(expectedContains(r, "'='") && expectedContains(r, "'<>'"));
    });
    runTest("falta la columna izquierda", [] {
        ParseResult r = rejects("SELECT a FROM t WHERE = 1;");
        EXPECT(r.error->found == "=");
        EXPECT(expectedContains(r, "identificador"));
    });
    runTest("WHERE sin condicion", [] { rejects("SELECT a FROM t WHERE ;"); });
    runTest("AND sin segunda condicion", [] { rejects("SELECT a FROM t WHERE x = 1 AND ;"); });
    runTest("dos operadores logicos seguidos", [] { rejects("SELECT a FROM t WHERE x = 1 AND OR y = 2;"); });
    runTest("operador logico al inicio", [] { rejects("SELECT a FROM t WHERE AND x = 1;"); });
    runTest("dos operadores relacionales seguidos", [] { rejects("SELECT a FROM t WHERE x = = 1;"); });
    runTest("comparacion con '*' como valor", [] { rejects("SELECT a FROM t WHERE x = *;"); });
    runTest("tokens sobrantes despues de ';'", [] { rejects("SELECT a FROM t; SELECT b FROM u;"); });

    // ------------------------------------------------------------------
    scenario(7, "Errores lexicos y robustez (nunca debe haber crash)");
    runTest("el lexer reporta un caracter invalido", [] {
        Lexer lexer("SELECT a FROM t WHERE x @ 1;");
        lexer.tokenizeAll();
        EXPECT(lexer.hasErrors());
    });
    runTest("cadena sin cerrar: el lexer marca error", [] {
        Lexer lexer("SELECT a FROM t WHERE x = 'abc;");
        lexer.tokenizeAll();
        EXPECT(lexer.hasErrors());
    });
    runTest("el parser no falla con los tokens que deja el lexer tras un error", [] {
        Lexer lexer("SELECT a FROM t WHERE x @ 1;");
        ParseResult r = parseTokens(lexer.tokenizeAll(), TABLE);
        EXPECT(!r.accepted);                          // sin '@' queda 'x 1', que es invalido
        EXPECT(r.error.has_value());
    });
    runTest("entradas raras se rechazan sin fallar", [] {
        rejects("");
        rejects("   \n\t ");
        rejects(";");
        rejects("SELECT");
        rejects("'texto'");
        rejects("SELECT a FROM t WHERE x = 'abc");
        rejects("SELECT 5 FROM t;");
    });
    runTest("el mensaje de error trae posicion, token encontrado y esperados", [] {
        ParseResult r = rejects("SELECT a FROM t\nWHERE x > ;");
        EXPECT(r.error->line == 2);
        EXPECT(r.error->column == 11);
        EXPECT(r.error->message.find("2:11") != std::string::npos);
        EXPECT(r.error->message.find("se encontro") != std::string::npos);
        EXPECT(r.error->message.find("se esperaba") != std::string::npos);
    });

    std::cout << "\n=================================\n";
    std::cout << "Pruebas ejecutadas: " << totalTests << " | verificaciones: " << totalChecks
              << " | fallidas: " << failedTests << "\n";
    std::cout << (failedTests == 0 ? "RESULTADO: TODAS LAS PRUEBAS PASARON\n"
                                   : "RESULTADO: HAY PRUEBAS FALLIDAS\n");
    return failedTests == 0 ? 0 : 1;
}
