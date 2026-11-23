#include <iostream>
#include <string>

#include "ast_builder.h"
#include "ast_printer.h"
#include "catalog.h"
#include "lexer.h"
#include "parse_table.h"
#include "parser.h"
#include "semantic_analyzer.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { ++failures; std::cerr << "FALLO l." << __LINE__ << ": " #c "\n"; } } while (0)

static const char* kCatalog =
    "table_name,column_name,data_type,is_primary_key\n"
    "usuarios,id,INT,true\nusuarios,nombre,VARCHAR,false\nusuarios,edad,INT,false\nusuarios,saldo,FLOAT,false\n";

struct Out { bool built = false; std::string tree; std::vector<sem::SemanticErrorKind> errs; };

static Out run(const std::string& q) {
    sem::CatalogManager cat; std::string e;
    cat.loadFromString(kCatalog, e);
    Lexer lx(q); auto toks = lx.tokenizeAll();
    auto pr = ll1::parseTokens(toks, ll1::defaultParseTable());
    Out o; if (!pr.accepted) return o;
    auto br = sem::buildAst(toks, pr.productions);
    if (!br.ok) return o;
    o.built = true;
    sem::SemanticAnalyzer an(cat); an.analyze(*br.ast);
    for (auto& d : an.diagnostics()) o.errs.push_back(d.kind);
    o.tree = sem::astToString(*br.ast);
    return o;
}

int main() {
    auto a = run("SELECT nombre FROM usuarios WHERE edad >= 18 AND saldo > 0.5;");
    CHECK(a.built && a.errs.empty());
    CHECK(a.tree.find("BinaryExprNode(AND) : BOOLEAN") != std::string::npos);

    auto s = run("SELECT * FROM usuarios;");
    CHECK(s.built && s.errs.empty());
    CHECK(s.tree.find("ColumnNode(saldo) : FLOAT") != std::string::npos);

    // asociatividad izquierda: AND(AND(a,b),c)
    auto l = run("SELECT id FROM usuarios WHERE id = 1 AND edad = 2 AND saldo = 3;");
    CHECK(l.built && l.tree.find("   ├─ BinaryExprNode(AND)") != std::string::npos);

    auto t = run("SELECT id FROM nada;");
    CHECK(t.built && t.errs.size() == 1 && t.errs[0] == sem::SemanticErrorKind::TABLE_NOT_FOUND);
    auto c = run("SELECT zzz FROM usuarios;");
    CHECK(c.built && c.errs.size() == 1 && c.errs[0] == sem::SemanticErrorKind::COLUMN_NOT_FOUND);
    auto m = run("SELECT nombre FROM usuarios WHERE edad = 'veinte';");
    CHECK(m.built && m.errs.size() == 1 && m.errs[0] == sem::SemanticErrorKind::TYPE_MISMATCH);
    auto d = run("SELECT nombre, nombre FROM usuarios;");
    CHECK(d.built && d.errs.size() == 1 && d.errs[0] == sem::SemanticErrorKind::DUPLICATE_COLUMN);

    // INT con FLOAT permitido
    auto p = run("SELECT id FROM usuarios WHERE edad > 1.5;");
    CHECK(p.built && p.errs.empty());

    // producciones corruptas: el builder no debe lanzar
    Lexer lx("SELECT * FROM t;"); auto toks = lx.tokenizeAll();
    CHECK(!sem::buildAst(toks, {}).ok);
    CHECK(!sem::buildAst(toks, {1, 99}).ok);

    if (failures == 0) std::cout << "Todas las pruebas semanticas pasaron\n";
    return failures ? 1 : 0;
}
