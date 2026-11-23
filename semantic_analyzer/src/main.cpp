// Ejecutable de la Practica 6: lexer + parser LL(1) + AST + analisis semantico.
//
// Uso:
//   ./semantic_analyzer [--catalog ruta.csv] [--quiet] [archivo.sql]   (sin archivo: entrada estandar)
// Codigos de salida: 0 = valida, 1 = error sintactico, 2 = error lexico,
//                    3 = error de uso/lectura/catalogo/AST, 4 = errores semanticos.

#include <iostream>
#include <string>
#include <vector>

#include "ast_builder.h"
#include "ast_printer.h"
#include "catalog.h"
#include "input.h"
#include "lexer.h"
#include "parse_table.h"
#include "parser.h"
#include "semantic_analyzer.h"

int main(int argc, char* argv[]) {
    #ifdef DEFAULT_CATALOG
    std::string catalogPath = DEFAULT_CATALOG;
#else
    std::string catalogPath = "semantic_analyzer/data/catalog.csv";
#endif
    std::string path;
    bool quiet = false;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--help" || a == "-h") {
            std::cout << "Uso: semantic_analyzer [--catalog ruta.csv] [--quiet] [archivo.sql]\n";
            return 0;
        }
        if (a == "--quiet") { quiet = true; continue; }
        if (a == "--catalog") {
            if (i + 1 >= argc) { std::cerr << "--catalog requiere una ruta\n"; return 3; }
            catalogPath = argv[++i];
            continue;
        }
        if (a.size() > 1 && a[0] == '-') { std::cerr << "Opcion desconocida: " << a << "\n"; return 3; }
        path = a;
    }

    std::string source, err;
    if (!path.empty()) {
        if (!readFile(path, source, err)) { std::cerr << err << "\n"; return 3; }
    } else {
        source = readStandardInput();
    }

    sem::CatalogManager catalog;
    if (!catalog.loadFromFile(catalogPath, err)) {
        std::cerr << "No se pudo cargar el catalogo '" << catalogPath << "': " << err << "\n";
        return 3;
    }

    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenizeAll();
    for (const auto& d : lexer.diagnostics())
        std::cout << (d.severity == Severity::ERROR ? "[ERROR lexico] " : "[ADVERTENCIA] ")
                  << d.line << ":" << d.column << " " << d.message << "\n";
    if (lexer.hasErrors()) { std::cout << "RESULTADO: RECHAZADA (errores lexicos)\n"; return 2; }

    ll1::ParseResult pr = ll1::parseTokens(tokens, ll1::defaultParseTable());
    if (!pr.accepted) {
        std::cout << pr.error->message << "\nRESULTADO: RECHAZADA (error sintactico)\n";
        return 1;
    }

    sem::BuildResult br = sem::buildAst(tokens, pr.productions);
    if (!br.ok) { std::cerr << br.error << "\n"; return 3; }

    if (!quiet) std::cout << "AST:\n" << sem::astToString(*br.ast) << "\n";

    sem::SemanticAnalyzer analyzer(catalog);
    analyzer.analyze(*br.ast);
    for (const auto& d : analyzer.diagnostics()) std::cout << sem::format(d) << "\n";

    if (analyzer.hasErrors()) { std::cout << "RESULTADO: RECHAZADA (errores semanticos)\n"; return 4; }
    if (!quiet) std::cout << "AST decorado:\n" << sem::astToString(*br.ast);
    std::cout << "RESULTADO: VALIDA (semanticamente correcta)\n";
    return 0;
}
