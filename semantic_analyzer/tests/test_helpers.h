#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

// Utilidades compartidas por test_errores.cpp y test_ast_catalog.cpp (Practica 6).
// Reproduce el flujo completo: texto SQL -> Lexer -> Parser LL(1) -> AST -> analizador semantico.

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "ast_builder.h"
#include "ast_printer.h"
#include "catalog.h"
#include "lexer.h"
#include "parse_table.h"
#include "parser.h"
#include "semantic_analyzer.h"

namespace th {
inline int g_checks = 0;
inline int g_fail = 0;
}

#define CHECK(c) do { ++th::g_checks; if (!(c)) { ++th::g_fail; \
    std::cerr << "FALLO " << __FILE__ << ":" << __LINE__ << ": " #c "\n"; } } while (0)

#define CHECK_EQ(a, b) do { ++th::g_checks; auto va_ = (a); auto vb_ = (b); if (!(va_ == vb_)) { ++th::g_fail; \
    std::cerr << "FALLO " << __FILE__ << ":" << __LINE__ << ": " #a " == " #b "\n   obtenido: " << va_ \
              << "\n   esperado: " << vb_ << "\n"; } } while (0)

namespace th {

using K = sem::SemanticErrorKind;

// Catalogo de prueba en memoria (mismo formato que data/catalog.csv).
inline const char* kCsv =
    "table_name,column_name,data_type,is_primary_key\n"
    "usuarios,id,INT,true\n"
    "usuarios,nombre,VARCHAR,false\n"
    "usuarios,edad,INT,false\n"
    "usuarios,saldo,FLOAT,false\n"
    "usuarios,activo,BOOLEAN,false\n"
    "productos,id,INT,true\n"
    "productos,titulo,VARCHAR,false\n"
    "productos,precio,FLOAT,false\n";

inline sem::CatalogManager makeCatalog(const std::string& csv = kCsv) {
    sem::CatalogManager c;
    std::string err;
    c.loadFromString(csv, err);
    return c;
}

struct Run {
    bool lexOk = false, parsed = false, built = false;
    std::unique_ptr<sem::SelectStmtNode> ast;
    std::vector<sem::SemanticDiagnostic> diags;
    std::string before;   // AST sin decorar
    std::string after;    // AST tras el analizador
    bool hasKind(K k) const {
        for (const auto& d : diags) if (d.kind == k) return true;
        return false;
    }
};

inline Run run(const std::string& query, const sem::CatalogManager& cat) {
    Run r;
    Lexer lx(query);
    std::vector<Token> toks = lx.tokenizeAll();
    if (lx.hasErrors()) return r;
    r.lexOk = true;
    ll1::ParseResult pr = ll1::parseTokens(toks, ll1::defaultParseTable());
    if (!pr.accepted) return r;
    r.parsed = true;
    sem::BuildResult br = sem::buildAst(toks, pr.productions);
    if (!br.ok) return r;
    r.built = true;
    r.ast = std::move(br.ast);
    r.before = sem::astToString(*r.ast);
    sem::SemanticAnalyzer an(cat);
    an.analyze(*r.ast);
    r.diags = an.diagnostics();
    r.after = sem::astToString(*r.ast);
    return r;
}

inline std::string readFile(const std::string& path) {
    std::ifstream f(path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

inline int finish(const char* name) {
    std::cout << name << ": " << g_checks << " comprobaciones, " << g_fail << " fallidas\n";
    return g_fail == 0 ? 0 : 1;
}

} // namespace th

#endif // TEST_HELPERS_H
