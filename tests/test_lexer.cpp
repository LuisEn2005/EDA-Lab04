#include "../include/lexer.h"

#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

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

struct Result {
    std::vector<Token> tokens;
    std::vector<Diagnostic> diags;
};

Result lex(const std::string& src) {
    Lexer lexer(src);
    Result r;
    r.tokens = lexer.tokenizeAll();
    r.diags = lexer.diagnostics();
    return r;
}

int countSeverity(const Result& r, Severity s) {
    int n = 0;
    for (const auto& d : r.diags) if (d.severity == s) ++n;
    return n;
}

int errors(const Result& r)   { return countSeverity(r, Severity::ERROR); }
int warnings(const Result& r) { return countSeverity(r, Severity::WARNING); }

std::vector<TokenType> types(const Result& r) {
    std::vector<TokenType> out;
    for (size_t i = 0; i + 1 < r.tokens.size(); ++i) out.push_back(r.tokens[i].type);
    return out;
}

using TT = TokenType;
using Types = std::vector<TokenType>;

bool endsWithEof(const Result& r) {
    return !r.tokens.empty() && r.tokens.back().type == TT::TOKEN_EOF;
}

const Token& first(const Result& r) { return r.tokens.front(); }

} // namespace

static void testScenario1() {
    scenario(1, "Sentencia valida (SELECT, FROM, WHERE)");

    runTest("ejemplo del enunciado (tabla 1)", [] {
        Result r = lex("SELECT nombre, edad\nFROM estudiantes\nWHERE edad >= 18;");
        Types expected = {TT::KEYWORD_SELECT, TT::IDENTIFIER, TT::COMMA, TT::IDENTIFIER,
                          TT::KEYWORD_FROM, TT::IDENTIFIER, TT::KEYWORD_WHERE,
                          TT::IDENTIFIER, TT::GREATER_EQUAL, TT::INTEGER_LITERAL,
                          TT::SEMICOLON};
        EXPECT(types(r) == expected);
        EXPECT(endsWithEof(r));
        EXPECT(r.tokens.size() == expected.size() + 1);
        EXPECT(r.diags.empty());
    });

    runTest("se conserva el orden y los lexemas", [] {
        Result r = lex("SELECT nombre, edad FROM estudiantes WHERE edad >= 18;");
        const char* lexemes[] = {"SELECT", "nombre", ",", "edad", "FROM", "estudiantes",
                                 "WHERE", "edad", ">=", "18", ";"};
        EXPECT(r.tokens.size() == 12);
        for (size_t i = 0; i < 11 && i < r.tokens.size(); ++i) {
            EXPECT(r.tokens[i].lexeme == lexemes[i]);
        }
    });

    runTest("posiciones (linea, columna) en varias lineas", [] {
        Result r = lex("SELECT nombre, edad\nFROM estudiantes\nWHERE edad >= 18;");
        EXPECT(r.tokens[0].line == 1 && r.tokens[0].column == 1);
        EXPECT(r.tokens[1].line == 1 && r.tokens[1].column == 8);
        EXPECT(r.tokens[2].line == 1 && r.tokens[2].column == 14);
        EXPECT(r.tokens[3].line == 1 && r.tokens[3].column == 16);
        EXPECT(r.tokens[4].line == 2 && r.tokens[4].column == 1);
        EXPECT(r.tokens[5].line == 2 && r.tokens[5].column == 6);
        EXPECT(r.tokens[6].line == 3 && r.tokens[6].column == 1);
        EXPECT(r.tokens[8].line == 3 && r.tokens[8].column == 12);
        EXPECT(r.tokens[9].line == 3 && r.tokens[9].column == 15);
        EXPECT(r.tokens[10].line == 3 && r.tokens[10].column == 17);
    });

    runTest("consulta con AND, OR, cadena y decimal", [] {
        Result r = lex("SELECT a FROM t WHERE a = 'x' AND b <> 2.5 OR c < 3;");
        Types expected = {TT::KEYWORD_SELECT, TT::IDENTIFIER, TT::KEYWORD_FROM, TT::IDENTIFIER,
                          TT::KEYWORD_WHERE, TT::IDENTIFIER, TT::EQUAL, TT::STRING_LITERAL,
                          TT::KEYWORD_AND, TT::IDENTIFIER, TT::NOT_EQUAL, TT::FLOAT_LITERAL,
                          TT::KEYWORD_OR, TT::IDENTIFIER, TT::LESS_THAN, TT::INTEGER_LITERAL,
                          TT::SEMICOLON};
        EXPECT(types(r) == expected);
        EXPECT(r.diags.empty());
    });
}

static void testScenario2() {
    scenario(2, "Operadores compuestos");

    runTest("los seis operadores relacionales y '!='", [] {
        Result r = lex("= <> != < > <= >=");
        Types expected = {TT::EQUAL, TT::NOT_EQUAL, TT::NOT_EQUAL, TT::LESS_THAN,
                          TT::GREATER_THAN, TT::LESS_EQUAL, TT::GREATER_EQUAL};
        EXPECT(types(r) == expected);
        EXPECT(r.diags.empty());
    });

    runTest(">=, <=, <> y != son UN solo token con su lexema completo", [] {
        const char* ops[] = {">=", "<=", "<>", "!="};
        for (const char* op : ops) {
            Result r = lex(op);
            EXPECT(r.tokens.size() == 2);
            EXPECT(first(r).lexeme == op);
            EXPECT(first(r).column == 1);
        }
    });

    runTest("operadores sin espacios alrededor", [] {
        Result r = lex("a>=1");
        Types expected = {TT::IDENTIFIER, TT::GREATER_EQUAL, TT::INTEGER_LITERAL};
        EXPECT(types(r) == expected);
        Result r2 = lex("a<>b");
        Types expected2 = {TT::IDENTIFIER, TT::NOT_EQUAL, TT::IDENTIFIER};
        EXPECT(types(r2) == expected2);
        Result r3 = lex("a!=b");
        EXPECT(types(r3) == expected2);
    });

    runTest("operadores separados por espacio NO se unen", [] {
        Result r = lex("> =");
        Types expected = {TT::GREATER_THAN, TT::EQUAL};
        EXPECT(types(r) == expected);
        Result r2 = lex("< >");
        Types expected2 = {TT::LESS_THAN, TT::GREATER_THAN};
        EXPECT(types(r2) == expected2);
    });

    runTest("operadores consecutivos se dividen de forma maximal", [] {
        Result r = lex("<=>");
        Types expected = {TT::LESS_EQUAL, TT::GREATER_THAN};
        EXPECT(types(r) == expected);
        Result r2 = lex(">==");
        Types expected2 = {TT::GREATER_EQUAL, TT::EQUAL};
        EXPECT(types(r2) == expected2);
    });

    runTest("'<' y '>' solos", [] {
        Result r = lex("a < b > c");
        Types expected = {TT::IDENTIFIER, TT::LESS_THAN, TT::IDENTIFIER, TT::GREATER_THAN,
                          TT::IDENTIFIER};
        EXPECT(types(r) == expected);
    });
}

static void testScenario3() {
    scenario(3, "Literales (enteros, decimales y cadenas)");

    runTest("entero", [] {
        Result r = lex("42");
        EXPECT(first(r).type == TT::INTEGER_LITERAL);
        EXPECT(first(r).lexeme == "42");
        EXPECT(first(r).value == "42");
        EXPECT(r.diags.empty());
    });

    runTest("decimal", [] {
        Result r = lex("3.14");
        EXPECT(first(r).type == TT::FLOAT_LITERAL);
        EXPECT(first(r).lexeme == "3.14");
        EXPECT(first(r).value == "3.14");
        EXPECT(r.diags.empty());
    });

    runTest("entero negativo (el '-' va pegado al numero)", [] {
        Result r = lex("-3");
        EXPECT(r.tokens.size() == 2);
        EXPECT(first(r).type == TT::INTEGER_LITERAL);
        EXPECT(first(r).lexeme == "-3");
        EXPECT(first(r).value == "-3");
    });

    runTest("decimal negativo", [] {
        Result r = lex("-2.75");
        EXPECT(first(r).type == TT::FLOAT_LITERAL);
        EXPECT(first(r).value == "-2.75");
        EXPECT(r.diags.empty());
    });

    runTest(".5 se normaliza a 0.5 (con advertencia)", [] {
        Result r = lex(".5");
        EXPECT(first(r).type == TT::FLOAT_LITERAL);
        EXPECT(first(r).lexeme == ".5");
        EXPECT(first(r).value == "0.5");
        EXPECT(errors(r) == 0);
        EXPECT(warnings(r) == 1);
    });

    runTest("5. se normaliza a 5.0 (con advertencia)", [] {
        Result r = lex("5.");
        EXPECT(first(r).type == TT::FLOAT_LITERAL);
        EXPECT(first(r).lexeme == "5.");
        EXPECT(first(r).value == "5.0");
        EXPECT(errors(r) == 0);
        EXPECT(warnings(r) == 1);
    });

    runTest("-.5 se normaliza a -0.5", [] {
        Result r = lex("-.5");
        EXPECT(first(r).type == TT::FLOAT_LITERAL);
        EXPECT(first(r).lexeme == "-.5");
        EXPECT(first(r).value == "-0.5");
        EXPECT(errors(r) == 0);
    });

    runTest("5. antes de ';' no consume el punto y coma", [] {
        Result r = lex("5.;");
        Types expected = {TT::FLOAT_LITERAL, TT::SEMICOLON};
        EXPECT(types(r) == expected);
    });

    runTest("a-3 se lee como identificador y entero negativo", [] {
        Result r = lex("edad-3");
        Types expected = {TT::IDENTIFIER, TT::INTEGER_LITERAL};
        EXPECT(types(r) == expected);
        EXPECT(r.tokens[1].value == "-3");
        EXPECT(r.tokens[1].column == 5);
        EXPECT(r.diags.empty());
    });

    runTest("cadena simple: el lexema no incluye las comillas", [] {
        Result r = lex("'Juan'");
        EXPECT(first(r).type == TT::STRING_LITERAL);
        EXPECT(first(r).lexeme == "Juan");
        EXPECT(first(r).value == "Juan");
        EXPECT(first(r).column == 1);
        EXPECT(r.diags.empty());
    });

    runTest("cadena vacia y cadena con espacios", [] {
        Result r = lex("'' 'hola mundo'");
        EXPECT(r.tokens.size() == 3);
        EXPECT(r.tokens[0].type == TT::STRING_LITERAL && r.tokens[0].lexeme.empty());
        EXPECT(r.tokens[1].type == TT::STRING_LITERAL && r.tokens[1].value == "hola mundo");
        EXPECT(r.tokens[1].column == 4);
    });

    runTest("palabras reservadas y operadores dentro de una cadena son texto", [] {
        Result r = lex("'SELECT >= 5 ; @'");
        EXPECT(r.tokens.size() == 2);
        EXPECT(first(r).type == TT::STRING_LITERAL);
        EXPECT(first(r).value == "SELECT >= 5 ; @");
        EXPECT(r.diags.empty());
    });

    runTest("escape \\' dentro de la cadena", [] {
        Result r = lex("'it\\'s'");
        EXPECT(r.tokens.size() == 2);
        EXPECT(first(r).type == TT::STRING_LITERAL);
        EXPECT(first(r).lexeme == "it\\'s");
        EXPECT(first(r).value == "it's");
        EXPECT(r.diags.empty());
    });

    runTest("escape \\\\ (barra invertida literal)", [] {
        Result r = lex("'a\\\\b'");
        EXPECT(first(r).type == TT::STRING_LITERAL);
        EXPECT(first(r).value == "a\\b");
        EXPECT(r.diags.empty());
    });

    runTest("escape desconocido: advertencia, no error, se conserva el texto", [] {
        Result r = lex("'x\\ny'");
        EXPECT(first(r).type == TT::STRING_LITERAL);
        EXPECT(first(r).value == "x\\ny");
        EXPECT(errors(r) == 0);
        EXPECT(warnings(r) == 1);
        EXPECT(r.diags[0].line == 1 && r.diags[0].column == 3);
    });

    runTest("caracteres no ASCII dentro de una cadena se aceptan", [] {
        Result r = lex("'a\xC3\xB1o'");
        EXPECT(first(r).type == TT::STRING_LITERAL);
        EXPECT(first(r).value == "a\xC3\xB1o");
        EXPECT(r.diags.empty());
    });
}

static void testScenario4() {
    scenario(4, "Palabras reservadas vs identificadores");

    runTest("las cinco palabras reservadas", [] {
        Result r = lex("SELECT FROM WHERE AND OR");
        Types expected = {TT::KEYWORD_SELECT, TT::KEYWORD_FROM, TT::KEYWORD_WHERE,
                          TT::KEYWORD_AND, TT::KEYWORD_OR};
        EXPECT(types(r) == expected);
    });

    runTest("palabras reservadas insensibles a mayusculas/minusculas", [] {
        Result r = lex("select Select sElEcT from FROM where Where and And or oR");
        Types expected = {TT::KEYWORD_SELECT, TT::KEYWORD_SELECT, TT::KEYWORD_SELECT,
                          TT::KEYWORD_FROM, TT::KEYWORD_FROM, TT::KEYWORD_WHERE,
                          TT::KEYWORD_WHERE, TT::KEYWORD_AND, TT::KEYWORD_AND,
                          TT::KEYWORD_OR, TT::KEYWORD_OR};
        EXPECT(types(r) == expected);
    });

    runTest("el lexema conserva la escritura original", [] {
        Result r = lex("SeLeCt");
        EXPECT(first(r).type == TT::KEYWORD_SELECT);
        EXPECT(first(r).lexeme == "SeLeCt");
    });

    runTest("identificadores sensibles a mayusculas/minusculas", [] {
        Result r = lex("Edad edad EDAD");
        EXPECT(r.tokens.size() == 4);
        for (int i = 0; i < 3; ++i) EXPECT(r.tokens[i].type == TT::IDENTIFIER);
        EXPECT(r.tokens[0].lexeme == "Edad");
        EXPECT(r.tokens[1].lexeme == "edad");
        EXPECT(r.tokens[2].lexeme == "EDAD");
        EXPECT(r.tokens[0].lexeme != r.tokens[1].lexeme);
    });

    runTest("un identificador que CONTIENE una palabra reservada no es palabra reservada", [] {
        Result r = lex("selected fromage orders android where_ _select and1 SELECTS");
        EXPECT(r.tokens.size() == 9);
        for (size_t i = 0; i + 1 < r.tokens.size(); ++i) EXPECT(r.tokens[i].type == TT::IDENTIFIER);
        EXPECT(r.diags.empty());
    });

    runTest("identificadores validos: letras, digitos y guion bajo", [] {
        Result r = lex("a _a a1 a_1 _ __x9 Nombre_Completo");
        EXPECT(r.tokens.size() == 8);
        for (size_t i = 0; i + 1 < r.tokens.size(); ++i) EXPECT(r.tokens[i].type == TT::IDENTIFIER);
        EXPECT(r.tokens[6].lexeme == "Nombre_Completo");
        EXPECT(r.diags.empty());
    });

    runTest("una palabra reservada dentro de una sentencia no rompe el resto", [] {
        Result r = lex("select Select from");
        Types expected = {TT::KEYWORD_SELECT, TT::KEYWORD_SELECT, TT::KEYWORD_FROM};
        EXPECT(types(r) == expected);
    });
}

static void testScenario5() {
    scenario(5, "Espacios en blanco (espacios, tabulaciones, saltos de linea)");

    runTest("espacios multiples se ignoran", [] {
        Result r = lex("SELECT      a     FROM   t");
        Types expected = {TT::KEYWORD_SELECT, TT::IDENTIFIER, TT::KEYWORD_FROM, TT::IDENTIFIER};
        EXPECT(types(r) == expected);
        EXPECT(r.tokens[1].column == 13);
        EXPECT(r.tokens[2].column == 19);
    });

    runTest("tabulaciones se ignoran y cuentan como una columna", [] {
        Result r = lex("a\tb\t\tc");
        EXPECT(r.tokens.size() == 4);
        EXPECT(r.tokens[0].column == 1);
        EXPECT(r.tokens[1].column == 3);
        EXPECT(r.tokens[2].column == 6);
        EXPECT(r.diags.empty());
    });

    runTest("saltos de linea incrementan la linea y reinician la columna", [] {
        Result r = lex("a\nb\n\n  c");
        EXPECT(r.tokens.size() == 4);
        EXPECT(r.tokens[0].line == 1 && r.tokens[0].column == 1);
        EXPECT(r.tokens[1].line == 2 && r.tokens[1].column == 1);
        EXPECT(r.tokens[2].line == 4 && r.tokens[2].column == 3);
    });

    runTest("CRLF (\\r\\n) cuenta como un solo salto de linea", [] {
        Result r = lex("a\r\nb\r\nc");
        EXPECT(r.tokens.size() == 4);
        EXPECT(r.tokens[0].line == 1);
        EXPECT(r.tokens[1].line == 2 && r.tokens[1].column == 1);
        EXPECT(r.tokens[2].line == 3 && r.tokens[2].column == 1);
        EXPECT(r.diags.empty());
    });

    runTest("mezcla de espacios, tabs y saltos de linea en una consulta", [] {
        Result r = lex("SELECT\ta,\n\tb\r\n  FROM\t t\n WHERE a\t=\t1 ;");
        Types expected = {TT::KEYWORD_SELECT, TT::IDENTIFIER, TT::COMMA, TT::IDENTIFIER,
                          TT::KEYWORD_FROM, TT::IDENTIFIER, TT::KEYWORD_WHERE, TT::IDENTIFIER,
                          TT::EQUAL, TT::INTEGER_LITERAL, TT::SEMICOLON};
        EXPECT(types(r) == expected);
        EXPECT(r.diags.empty());
    });

    runTest("los espacios separan tokens: 'ab' y 'a b' son distintos", [] {
        EXPECT(lex("ab").tokens.size() == 2);
        EXPECT(lex("a b").tokens.size() == 3);
    });
}

static void testScenario6() {
    scenario(6, "Errores lexicos");

    runTest("caracteres no admitidos (@ # $ % ^ & * ( ) ~ ?)", [] {
        const char chars[] = {'@', '#', '$', '%', '^', '&', '*', '(', ')', '~', '?', '"', '.'};
        for (char c : chars) {
            Result r = lex(std::string(1, c));
            EXPECT(errors(r) == 1);
            EXPECT(r.tokens.size() == 1);
            EXPECT(r.diags[0].line == 1 && r.diags[0].column == 1);
        }
    });

    runTest("el error no se clasifica como token y el analisis continua", [] {
        Result r = lex("SELECT @campo FROM t");
        Types expected = {TT::KEYWORD_SELECT, TT::IDENTIFIER, TT::KEYWORD_FROM, TT::IDENTIFIER};
        EXPECT(types(r) == expected);
        EXPECT(errors(r) == 1);
        EXPECT(r.diags[0].line == 1 && r.diags[0].column == 8);
        EXPECT(r.tokens[1].lexeme == "campo");
        EXPECT(r.tokens[1].column == 9);
    });

    runTest("cadena sin cerrar (fin de entrada)", [] {
        Result r = lex("SELECT 'incompleta");
        EXPECT(types(r) == Types{TT::KEYWORD_SELECT});
        EXPECT(errors(r) == 1);
        EXPECT(r.diags[0].line == 1 && r.diags[0].column == 8);
        EXPECT(endsWithEof(r));
    });

    runTest("cadena con salto de linea real: error y la linea siguiente se analiza", [] {
        Result r = lex("'abc\nSELECT");
        EXPECT(errors(r) >= 1);
        EXPECT(r.diags[0].line == 1 && r.diags[0].column == 1);
        bool sawSelect = false;
        for (const auto& t : r.tokens) if (t.type == TT::KEYWORD_SELECT) sawSelect = true;
        EXPECT(sawSelect);
    });

    runTest("la comilla escapada no cierra la cadena", [] {
        Result r = lex("'abc\\'");
        EXPECT(errors(r) == 1);
        EXPECT(r.tokens.size() == 1);
    });

    runTest("'!' solo es error; '!=' es valido", [] {
        Result r = lex("a ! b");
        Types expected = {TT::IDENTIFIER, TT::IDENTIFIER};
        EXPECT(types(r) == expected);
        EXPECT(errors(r) == 1);
        EXPECT(r.diags[0].column == 3);
        Result r2 = lex("! =");
        EXPECT(errors(r2) == 1);
        EXPECT(types(r2) == Types{TT::EQUAL});
    });

    runTest("'-' suelto es error (solo es valido pegado a un numero)", [] {
        Result r = lex("a - 3");
        Types expected = {TT::IDENTIFIER, TT::INTEGER_LITERAL};
        EXPECT(types(r) == expected);
        EXPECT(errors(r) == 1);
        EXPECT(r.diags[0].column == 3);
        Result r2 = lex("-abc");
        EXPECT(errors(r2) == 1);
        EXPECT(types(r2) == Types{TT::IDENTIFIER});
        Result r3 = lex("-");
        EXPECT(errors(r3) == 1);
        EXPECT(r3.tokens.size() == 1);
    });

    runTest("identificador que empieza por digito: un solo error", [] {
        Result r = lex("1abc");
        EXPECT(r.tokens.size() == 1);
        EXPECT(errors(r) == 1);
        EXPECT(r.diags[0].column == 1);
        Result r2 = lex("x = 2_a;");
        Types expected = {TT::IDENTIFIER, TT::EQUAL, TT::SEMICOLON};
        EXPECT(types(r2) == expected);
        EXPECT(errors(r2) == 1);
    });

    runTest("numero con mas de un punto: un solo error", [] {
        Result r = lex("x = 1.2.3 y");
        Types expected = {TT::IDENTIFIER, TT::EQUAL, TT::IDENTIFIER};
        EXPECT(types(r) == expected);
        EXPECT(errors(r) == 1);
        EXPECT(r.diags[0].line == 1 && r.diags[0].column == 5);
        Result r2 = lex("1.5abc");
        EXPECT(errors(r2) == 1);
        EXPECT(r2.tokens.size() == 1);
    });

    runTest("caracter no ASCII fuera de cadena: un solo error por caracter", [] {
        Result r = lex("a\xC3\xB1" "b");
        Types expected = {TT::IDENTIFIER, TT::IDENTIFIER};
        EXPECT(types(r) == expected);
        EXPECT(errors(r) == 1);
        EXPECT(r.diags[0].column == 2);
        EXPECT(r.tokens[1].column == 3);
    });

    runTest("se acumulan todos los errores, en orden, con su posicion", [] {
        Result r = lex("SELECT @a,\n#b FROM t WHERE x = 1.2.3 AND y = 'sin cerrar");
        EXPECT(errors(r) == 4);
        EXPECT(r.diags.size() == 4);
        EXPECT(r.diags[0].line == 1 && r.diags[0].column == 8);
        EXPECT(r.diags[1].line == 2 && r.diags[1].column == 1);
        EXPECT(r.diags[2].line == 2 && r.diags[2].column == 21);
        EXPECT(r.diags[3].line == 2 && r.diags[3].column == 35);
        EXPECT(endsWithEof(r));
    });

    runTest("los mensajes de error no estan vacios y todos son ERROR", [] {
        Result r = lex("@ # 1abc 'x");
        EXPECT(errors(r) == 4);
        for (const auto& d : r.diags) {
            EXPECT(!d.message.empty());
            EXPECT(d.severity == Severity::ERROR);
        }
    });

    runTest("las advertencias no cuentan como errores", [] {
        Result r = lex("x = .5 AND y = 'a\\qb'");
        EXPECT(errors(r) == 0);
        EXPECT(warnings(r) == 2);
        EXPECT(r.diags[0].severity == Severity::WARNING);
    });

    runTest("hasErrors() refleja el estado del lexer", [] {
        Lexer ok("SELECT a FROM t");
        ok.tokenizeAll();
        EXPECT(!ok.hasErrors());
        Lexer warnOnly("x = .5");
        warnOnly.tokenizeAll();
        EXPECT(!warnOnly.hasErrors());
        Lexer bad("SELECT @");
        bad.tokenizeAll();
        EXPECT(bad.hasErrors());
    });
}

static void testScenario7() {
    scenario(7, "Casos limite");

    runTest("entrada vacia: solo EOF en 1:1", [] {
        Result r = lex("");
        EXPECT(r.tokens.size() == 1);
        EXPECT(first(r).type == TT::TOKEN_EOF);
        EXPECT(first(r).line == 1 && first(r).column == 1);
        EXPECT(r.diags.empty());
    });

    runTest("solo espacios y saltos de linea: solo EOF con su posicion", [] {
        Result r = lex("  \t\n   \n  ");
        EXPECT(r.tokens.size() == 1);
        EXPECT(first(r).type == TT::TOKEN_EOF);
        EXPECT(first(r).line == 3 && first(r).column == 3);
    });

    runTest("sentencia incompleta: el lexer no valida la gramatica", [] {
        Result r = lex("SELECT");
        EXPECT(types(r) == Types{TT::KEYWORD_SELECT});
        Result r2 = lex("SELECT nombre FROM");
        Types expected = {TT::KEYWORD_SELECT, TT::IDENTIFIER, TT::KEYWORD_FROM};
        EXPECT(types(r2) == expected);
        Result r3 = lex("WHERE a >=");
        Types expected3 = {TT::KEYWORD_WHERE, TT::IDENTIFIER, TT::GREATER_EQUAL};
        EXPECT(types(r3) == expected3);
        EXPECT(r.diags.empty() && r2.diags.empty() && r3.diags.empty());
    });

    runTest("token al final de la entrada, sin espacio ni salto de linea", [] {
        Result r = lex("SELECT a");
        EXPECT(r.tokens.size() == 3);
        EXPECT(r.tokens[1].lexeme == "a");
        EXPECT(r.tokens[2].type == TT::TOKEN_EOF);
        EXPECT(r.tokens[2].column == 9);
        Result r2 = lex("x=18");
        Types expected2 = {TT::IDENTIFIER, TT::EQUAL, TT::INTEGER_LITERAL};
        EXPECT(types(r2) == expected2);
        EXPECT(r2.tokens[2].lexeme == "18");
        Result r3 = lex("x>='fin'");
        EXPECT(r3.tokens[2].type == TT::STRING_LITERAL);
        EXPECT(r3.tokens[2].lexeme == "fin");
        Result r4 = lex("a;");
        EXPECT(types(r4) == (Types{TT::IDENTIFIER, TT::SEMICOLON}));
        Result r5 = lex("a<");
        EXPECT(types(r5) == (Types{TT::IDENTIFIER, TT::LESS_THAN}));
        Result r6 = lex("a>");
        EXPECT(types(r6) == (Types{TT::IDENTIFIER, TT::GREATER_THAN}));
    });

    runTest("operador de dos caracteres incompleto al final", [] {
        Result r = lex("a !");
        EXPECT(types(r) == Types{TT::IDENTIFIER});
        EXPECT(errors(r) == 1);
        Result r2 = lex("a <");
        EXPECT(types(r2) == (Types{TT::IDENTIFIER, TT::LESS_THAN}));
    });

    runTest("un unico token", [] {
        EXPECT(types(lex(",")) == Types{TT::COMMA});
        EXPECT(types(lex(";")) == Types{TT::SEMICOLON});
        EXPECT(types(lex("=")) == Types{TT::EQUAL});
        EXPECT(types(lex("7")) == Types{TT::INTEGER_LITERAL});
    });

    runTest("siempre termina en un unico EOF", [] {
        const char* inputs[] = {"", "SELECT", "@", "'abc", "1.2.3", "a b c", "\n\n"};
        for (const char* in : inputs) {
            Result r = lex(in);
            EXPECT(endsWithEof(r));
            int eofCount = 0;
            for (const auto& t : r.tokens) if (t.type == TT::TOKEN_EOF) ++eofCount;
            EXPECT(eofCount == 1);
        }
    });

    runTest("nextToken() repetido tras el EOF sigue devolviendo EOF", [] {
        Lexer lexer("a");
        EXPECT(lexer.nextToken().type == TT::IDENTIFIER);
        EXPECT(lexer.nextToken().type == TT::TOKEN_EOF);
        EXPECT(lexer.nextToken().type == TT::TOKEN_EOF);
        EXPECT(lexer.nextToken().type == TT::TOKEN_EOF);
    });

    runTest("nextToken() omite los errores y entrega el siguiente token valido", [] {
        Lexer lexer("@ # a");
        Token t = lexer.nextToken();
        EXPECT(t.type == TT::IDENTIFIER);
        EXPECT(t.lexeme == "a");
        EXPECT(lexer.diagnostics().size() == 2);
    });

    runTest("entrada larga (rendimiento y posiciones)", [] {
        std::string src;
        for (int i = 0; i < 2000; ++i) src += "SELECT a FROM t WHERE a >= 1;\n";
        Result r = lex(src);
        EXPECT(r.tokens.size() == 2000 * 9 + 1);
        EXPECT(r.diags.empty());
        EXPECT(r.tokens.back().line == 2001 && r.tokens.back().column == 1);
    });
}

int main() {
    std::cout << "Pruebas del analizador lexico SQL\n";
    std::cout << "=================================\n";

    testScenario1();
    testScenario2();
    testScenario3();
    testScenario4();
    testScenario5();
    testScenario6();
    testScenario7();

    std::cout << "\n=================================\n";
    std::cout << "Pruebas ejecutadas: " << totalTests
              << " | verificaciones: " << totalChecks
              << " | fallidas: " << failedTests << "\n";
    if (failedTests == 0) {
        std::cout << "RESULTADO: TODAS LAS PRUEBAS PASARON\n";
        return 0;
    }
    std::cout << "RESULTADO: HAY PRUEBAS FALLIDAS\n";
    return 1;
}
