// Pruebas de errores semanticos (Practica 6): cada tipo de error con su linea/columna e identificador,
// acumulacion sin cascada, expansion de '*', sensibilidad a mayusculas, nodos armados a mano,
// reutilizacion del analizador, compuertas lexica/sintactica y los 8 archivos de examples/.

#include "test_helpers.h"

using namespace th;

#ifndef SEMANTIC_DIR
#define SEMANTIC_DIR "semantic_analyzer"
#endif

static void testTablaInexistente() {
    auto cat = makeCatalog();
    auto r = run("SELECT id FROM clientes WHERE id = 1;", cat);
    CHECK(r.built);
    CHECK_EQ(r.diags.size(), (size_t)1);
    CHECK(r.diags[0].kind == K::TABLE_NOT_FOUND);
    CHECK_EQ(r.diags[0].line, 1);
    CHECK_EQ(r.diags[0].column, 16);
    CHECK_EQ(r.diags[0].subject, std::string("clientes"));
    CHECK_EQ(sem::format(r.diags[0]),
             std::string("[ERROR semantico] 1:16 TABLE_NOT_FOUND: La tabla no existe en el catalogo [clientes]"));

    // Sin tabla no hay contra que validar columnas: un unico error, sin cascada.
    auto r2 = run("SELECT zzz FROM clientes WHERE qqq = 1;", cat);
    CHECK_EQ(r2.diags.size(), (size_t)1);
    CHECK(r2.diags[0].kind == K::TABLE_NOT_FOUND);
}

static void testColumnaInexistente() {
    auto cat = makeCatalog();
    auto r = run("SELECT precio FROM usuarios WHERE edad > 18;", cat);
    CHECK_EQ(r.diags.size(), (size_t)1);
    CHECK(r.diags[0].kind == K::COLUMN_NOT_FOUND);
    CHECK_EQ(r.diags[0].line, 1);
    CHECK_EQ(r.diags[0].column, 8);
    CHECK_EQ(r.diags[0].subject, std::string("precio"));
    CHECK_EQ(sem::format(r.diags[0]),
             std::string("[ERROR semantico] 1:8 COLUMN_NOT_FOUND: La columna no pertenece a la tabla usuarios [precio]"));

    auto w = run("SELECT nombre FROM usuarios WHERE zzz = 1;", cat);
    CHECK_EQ(w.diags.size(), (size_t)1);
    CHECK(w.diags[0].kind == K::COLUMN_NOT_FOUND);
    CHECK_EQ(w.diags[0].column, 35);

    // Columna de OTRA tabla del catalogo: tampoco pertenece a la tabla del FROM.
    auto o = run("SELECT titulo FROM usuarios;", cat);
    CHECK_EQ(o.diags.size(), (size_t)1);
    CHECK(o.diags[0].kind == K::COLUMN_NOT_FOUND);

    // Lado derecho de la comparacion (Value -> id).
    auto d = run("SELECT id FROM usuarios WHERE edad = qqq;", cat);
    CHECK_EQ(d.diags.size(), (size_t)1);
    CHECK(d.diags[0].kind == K::COLUMN_NOT_FOUND);
    CHECK_EQ(d.diags[0].subject, std::string("qqq"));
}

static void testTiposIncompatibles() {
    auto cat = makeCatalog();
    auto a = run("SELECT nombre FROM usuarios WHERE edad = 'veinte';", cat);
    CHECK_EQ(a.diags.size(), (size_t)1);
    CHECK(a.diags[0].kind == K::TYPE_MISMATCH);
    CHECK_EQ(a.diags[0].column, 40);   // posicion del operador
    CHECK_EQ(a.diags[0].subject, std::string("INT vs VARCHAR"));

    auto b = run("SELECT nombre FROM usuarios WHERE nombre > 100;", cat);
    CHECK_EQ(b.diags.size(), (size_t)1);
    CHECK(b.diags[0].kind == K::TYPE_MISMATCH);
    CHECK_EQ(b.diags[0].column, 42);
    CHECK_EQ(b.diags[0].subject, std::string("VARCHAR vs INT"));

    auto c = run("SELECT id FROM usuarios WHERE activo = 1;", cat);
    CHECK(c.hasKind(K::TYPE_MISMATCH));
    CHECK_EQ(c.diags[0].subject, std::string("BOOLEAN vs INT"));

    auto d = run("SELECT id FROM usuarios WHERE saldo = 'x';", cat);
    CHECK_EQ(d.diags[0].subject, std::string("FLOAT vs VARCHAR"));

    auto e = run("SELECT id FROM usuarios WHERE id = nombre;", cat);   // columna contra columna
    CHECK(e.hasKind(K::TYPE_MISMATCH));
    CHECK_EQ(e.diags[0].subject, std::string("INT vs VARCHAR"));
}

static void testTiposCompatibles() {
    auto cat = makeCatalog();
    const char* ok[] = {
        "SELECT id FROM usuarios WHERE edad > 1.5;",        // INT con FLOAT
        "SELECT id FROM usuarios WHERE saldo > 1;",         // FLOAT con INT
        "SELECT id FROM usuarios WHERE saldo >= saldo;",    // FLOAT con FLOAT
        "SELECT id FROM usuarios WHERE edad <> 3;",
        "SELECT id FROM usuarios WHERE edad != 3;",
        "SELECT id FROM usuarios WHERE nombre = 'ana';",    // VARCHAR con VARCHAR
        "SELECT id FROM usuarios WHERE activo = activo;",   // BOOLEAN con BOOLEAN
        "SELECT id FROM usuarios WHERE edad = id;",         // INT con INT
        "select id from usuarios where edad > 3;",          // palabras clave en minuscula
    };
    for (const char* q : ok) {
        auto r = run(q, cat);
        CHECK(r.built);
        CHECK(r.diags.empty());
    }
    auto r = run("SELECT id FROM usuarios WHERE edad > 1.5;", cat);
    CHECK(r.after.find("BinaryExprNode(>) : BOOLEAN") != std::string::npos);
}

static void testDuplicados() {
    auto cat = makeCatalog();
    auto a = run("SELECT nombre, nombre FROM usuarios;", cat);
    CHECK_EQ(a.diags.size(), (size_t)1);
    CHECK(a.diags[0].kind == K::DUPLICATE_COLUMN);
    CHECK_EQ(a.diags[0].column, 16);   // posicion de la repeticion
    CHECK_EQ(a.diags[0].subject, std::string("nombre"));

    auto b = run("SELECT nombre, edad, nombre, nombre FROM usuarios;", cat);
    CHECK_EQ(b.diags.size(), (size_t)2);   // la 2.a y 3.a aparicion

    auto c = run("SELECT nombre, edad FROM usuarios;", cat);
    CHECK(c.diags.empty());
}

static void testAcumulacionSinCascada() {
    auto cat = makeCatalog();
    auto r = run("SELECT zzz, nombre, nombre FROM usuarios WHERE edad = 'a' AND qqq > 1;", cat);
    CHECK_EQ(r.diags.size(), (size_t)4);
    if (r.diags.size() == 4) {
        CHECK(r.diags[0].kind == K::COLUMN_NOT_FOUND);  CHECK_EQ(r.diags[0].column, 8);
        CHECK(r.diags[1].kind == K::DUPLICATE_COLUMN);  CHECK_EQ(r.diags[1].column, 21);
        CHECK(r.diags[2].kind == K::TYPE_MISMATCH);     CHECK_EQ(r.diags[2].column, 53);
        CHECK(r.diags[3].kind == K::COLUMN_NOT_FOUND);  CHECK_EQ(r.diags[3].column, 63);
    }
    // Un hijo con error queda en UNKNOWN: el AND/OR padre no agrega otro error.
    auto a = run("SELECT id FROM usuarios WHERE zzz = 1 AND edad = 2;", cat);
    CHECK_EQ(a.diags.size(), (size_t)1);
    auto b = run("SELECT id FROM usuarios WHERE edad = 'a' OR saldo > 1;", cat);
    CHECK_EQ(b.diags.size(), (size_t)1);
    CHECK(!b.hasKind(K::LOGICAL_OPERAND_NOT_BOOLEAN));
}

static void testExpansionEstrella() {
    auto cat = makeCatalog();
    auto r = run("SELECT * FROM usuarios;", cat);
    CHECK(r.diags.empty());
    CHECK_EQ(r.ast->selectList.size(), (size_t)5);
    const char* names[] = {"id", "nombre", "edad", "saldo", "activo"};
    const sem::DataType types[] = {sem::DataType::INT, sem::DataType::VARCHAR, sem::DataType::INT,
                                   sem::DataType::FLOAT, sem::DataType::BOOLEAN};
    for (size_t i = 0; i < r.ast->selectList.size() && i < 5; ++i) {
        CHECK(r.ast->selectList[i]->kind() == sem::NodeKind::COLUMN);
        auto* c = static_cast<sem::ColumnNode*>(r.ast->selectList[i].get());
        CHECK_EQ(c->columnName, std::string(names[i]));      // orden del catalogo
        CHECK(c->inferredType == types[i]);
    }
    CHECK(r.before.find("StarNode") != std::string::npos);     // antes: comodin
    CHECK(r.after.find("StarNode") == std::string::npos);      // despues: lista explicita

    // Regresion: con tabla inexistente el '*' NO se descarta de la lista.
    auto m = run("SELECT * FROM clientes;", cat);
    CHECK(m.hasKind(K::TABLE_NOT_FOUND));
    CHECK_EQ(m.ast->selectList.size(), (size_t)1);
    CHECK(m.ast->selectList[0]->kind() == sem::NodeKind::STAR);
}

static void testMayusculas() {
    auto cat = makeCatalog();
    CHECK(run("SELECT NOMBRE FROM usuarios;", cat).hasKind(K::COLUMN_NOT_FOUND));
    CHECK(run("SELECT nombre FROM USUARIOS;", cat).hasKind(K::TABLE_NOT_FOUND));
    CHECK(run("SELECT nombre FROM usuarios;", cat).diags.empty());
}

static std::unique_ptr<sem::SelectStmtNode> handBuilt(std::unique_ptr<sem::ExprNode> where) {
    auto s = std::make_unique<sem::SelectStmtNode>(1, 1);
    s->selectList.push_back(std::make_unique<sem::ColumnNode>(1, 8, "nombre"));
    s->fromTable = std::make_unique<sem::TableRefNode>(1, 20, "usuarios");
    s->whereClause = std::move(where);
    return s;
}
static std::unique_ptr<sem::ExprNode> col(int c, const char* n) { return std::make_unique<sem::ColumnNode>(1, c, n); }

static void testNodosArmadosAMano() {
    auto cat = makeCatalog();
    sem::SemanticAnalyzer an(cat);

    // Con la gramatica actual AND/OR siempre reciben comparaciones; solo un AST manual puede violarlo.
    auto bad = handBuilt(std::make_unique<sem::BinaryExprNode>(1, 50, sem::BinaryOp::AND, col(40, "activo"), col(60, "edad")));
    CHECK(!an.analyze(*bad));
    CHECK_EQ(an.diagnostics().size(), (size_t)1);
    CHECK(an.diagnostics()[0].kind == K::LOGICAL_OPERAND_NOT_BOOLEAN);
    CHECK_EQ(an.diagnostics()[0].line, 1);
    CHECK_EQ(an.diagnostics()[0].column, 50);

    // BOOLEAN AND BOOLEAN es valido.
    auto good = handBuilt(std::make_unique<sem::BinaryExprNode>(1, 50, sem::BinaryOp::OR, col(40, "activo"), col(60, "activo")));
    CHECK(an.analyze(*good));   // reutilizacion: los diagnosticos previos se limpian
    CHECK(an.diagnostics().empty());
    CHECK(good->whereClause->inferredType == sem::DataType::BOOLEAN);

    // Un WHERE que no evalua a BOOLEAN.
    auto notBool = handBuilt(col(40, "edad"));
    CHECK(!an.analyze(*notBool));
    CHECK(an.diagnostics()[0].kind == K::TYPE_MISMATCH);

    // Arbol sin WHERE y sin FROM: no debe fallar.
    sem::SelectStmtNode vacio(1, 1);
    CHECK(an.analyze(vacio));
    auto noWhere = handBuilt(nullptr);
    CHECK(an.analyze(*noWhere));
}

static void testReutilizacionDelAnalizador() {
    auto cat = makeCatalog();
    sem::SemanticAnalyzer an(cat);
    auto a = handBuilt(col(40, "zzz"));
    CHECK(!an.analyze(*a));
    CHECK_EQ(an.diagnostics().size(), (size_t)1);
    auto b = handBuilt(nullptr);
    CHECK(an.analyze(*b));
    CHECK(an.diagnostics().empty());
}

static void testCompuertas() {
    auto cat = makeCatalog();
    auto lex = run("SELECT nombre FROM usuarios WHERE edad @ 3;", cat);
    CHECK(!lex.lexOk);
    CHECK(!lex.built);
    auto syn = run("SELECT * FROM usuarios WHERE edad = ;", cat);
    CHECK(syn.lexOk);
    CHECK(!syn.parsed);
    CHECK(!syn.built);
    CHECK(syn.diags.empty());
    CHECK(!run("FROM usuarios SELECT nombre;", cat).parsed);
}

static void testRobustez() {
    auto cat = makeCatalog();
    CHECK(!run("", cat).built);
    CHECK(!run("   \n\t ", cat).built);
    CHECK(!run("SELECT", cat).built);
    CHECK(!run(";", cat).built);

    // Catalogo vacio: toda tabla es inexistente, sin caidas.
    sem::CatalogManager vacio;
    auto v = run("SELECT * FROM usuarios WHERE edad = 1;", vacio);
    CHECK(v.hasKind(K::TABLE_NOT_FOUND));

    // Condicion muy larga (200 AND) y multilinea.
    std::string q = "SELECT id FROM usuarios WHERE id = 1";
    for (int i = 0; i < 200; ++i) q += " AND id = 1";
    q += ";";
    auto big = run(q, cat);
    CHECK(big.built);
    CHECK(big.diags.empty());

    auto ml = run("SELECT zzz\nFROM usuarios\nWHERE edad = 'a';", cat);
    CHECK_EQ(ml.diags.size(), (size_t)2);
    CHECK_EQ(ml.diags[0].line, 1);
    CHECK_EQ(ml.diags[1].line, 3);
}

static void testFormatoAdvertencia() {
    sem::SemanticDiagnostic w;
    w.severity = Severity::WARNING; w.kind = K::DUPLICATE_COLUMN; w.message = "msg"; w.line = 2; w.column = 3;
    CHECK_EQ(sem::format(w), std::string("[ADVERTENCIA semantica] 2:3 DUPLICATE_COLUMN: msg"));  // sin subject, sin corchetes
}

static void testArchivosDeEjemplo() {
    sem::CatalogManager cat;
    std::string err;
    CHECK(cat.loadFromFile(std::string(SEMANTIC_DIR) + "/data/catalog.csv", err));
    struct Case { const char* file; int kind; };   // kind: -1 = valida
    const Case cases[] = {
        {"semantic_valid_proyeccion.sql", -1},
        {"semantic_valid_star.sql", -1},
        {"semantic_valid_coercion.sql", -1},
        {"semantic_invalid_tabla.sql", (int)K::TABLE_NOT_FOUND},
        {"semantic_invalid_columna.sql", (int)K::COLUMN_NOT_FOUND},
        {"semantic_invalid_tipos.sql", (int)K::TYPE_MISMATCH},
        {"semantic_invalid_tipos_varchar_int.sql", (int)K::TYPE_MISMATCH},
        {"semantic_invalid_duplicados.sql", (int)K::DUPLICATE_COLUMN},
    };
    for (const auto& c : cases) {
        std::string text = readFile(std::string(SEMANTIC_DIR) + "/examples/" + c.file);
        CHECK(!text.empty());
        auto r = run(text, cat);
        CHECK(r.built);
        if (c.kind < 0) {
            CHECK(r.diags.empty());
        } else {
            CHECK_EQ(r.diags.size(), (size_t)1);
            CHECK(r.hasKind((K)c.kind));
        }
    }
}

int main() {
    testTablaInexistente();
    testColumnaInexistente();
    testTiposIncompatibles();
    testTiposCompatibles();
    testDuplicados();
    testAcumulacionSinCascada();
    testExpansionEstrella();
    testMayusculas();
    testNodosArmadosAMano();
    testReutilizacionDelAnalizador();
    testCompuertas();
    testRobustez();
    testFormatoAdvertencia();
    testArchivosDeEjemplo();
    return finish("test_semantic_errores");
}
