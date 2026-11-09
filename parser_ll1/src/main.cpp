// Ejecutable de la Practica 5: lexer (Practica 4) + parser LL(1) con traza.
//
// Uso:
//   ./parser_ll1 consulta.sql          analiza el archivo (con traza paso a paso)
//   ./parser_ll1 --quiet consulta.sql  sin traza, solo el resultado
//   ./parser_ll1 --tables              imprime gramatica, FIRST/FOLLOW y la tabla M[A,a]
//   echo "SELECT * FROM t;" | ./parser_ll1     lee de la entrada estandar
//
// Codigos de salida: 0 = aceptada, 1 = error sintactico, 2 = error lexico, 3 = no se pudo leer.

#include <iostream>
#include <string>
#include <vector>

#include "grammar.h"
#include "input.h"
#include "lexer.h"
#include "parse_table.h"
#include "parser.h"
#include "trace.h"

namespace {

void printTables() {
    ll1::printGrammar(std::cout);
    std::cout << "\n";
    ll1::printFirstFollow(std::cout, ll1::computeFirstFollow());
    std::cout << "\n";
    ll1::printParseTable(std::cout, ll1::defaultParseTable());
}

} // namespace

int main(int argc, char* argv[]) {
    bool quiet = false;
    std::string path;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--tables") { printTables(); return 0; }
        if (arg == "--quiet")  { quiet = true; continue; }
        path = arg;
    }

    std::string source;
    if (!path.empty()) {
        std::string err;
        if (!readFile(path, source, err)) { std::cerr << err << "\n"; return 3; }
    } else {
        source = readStandardInput();
    }

    std::cout << "Consulta:\n" << source << "\n";

    // 1) Analisis lexico (Practica 4, sin modificaciones destructivas).
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenizeAll();

    for (const auto& d : lexer.diagnostics()) {
        std::cout << (d.severity == Severity::ERROR ? "[ERROR lexico] " : "[ADVERTENCIA] ")
                  << d.line << ":" << d.column << " " << d.message << "\n";
    }
    // Decision de diseno: una consulta con errores lexicos se rechaza antes de parsear
    // (el lexer omite los caracteres invalidos y el parser podria aceptar una consulta distinta).
    if (lexer.hasErrors()) {
        std::cout << "\nRESULTADO: RECHAZADA (errores lexicos)\n";
        return 2;
    }

    // 2) Analisis sintactico LL(1) con la tabla construida desde FIRST/FOLLOW.
    const ll1::ParseTable& table = ll1::defaultParseTable();
    if (!table.isLL1()) {
        std::cerr << "La gramatica no es LL(1):\n";
        for (const auto& c : table.conflicts) std::cerr << "  " << c << "\n";
        return 3;
    }

    if (!quiet) {
        std::cout << "\nTraza del parseo:\n";
        ll1::printTraceHeader(std::cout);
    }
    ll1::ParseResult result =
        ll1::parseTokens(tokens, table, quiet ? ll1::StepObserver{} : ll1::makeTracer(std::cout));

    if (result.accepted) {
        std::cout << "\nRESULTADO: ACEPTADA (sintacticamente valida)\n";
        std::cout << "Producciones aplicadas:";
        for (int p : result.productions) std::cout << " " << p;
        std::cout << "\n";
        return 0;
    }

    std::cout << "\n" << result.error->message << "\n";
    std::cout << "RESULTADO: RECHAZADA (error sintactico)\n";
    return 1;
}
