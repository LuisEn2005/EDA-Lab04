// Pruebas del catalogo (tabla de simbolos) y del AST (Practica 6): duplicados, ordinales, CRLF,
// carga del esquema de 9 tablas, AST exacto segun el SDT, precedencia y asociatividad,
// operadores, literales, posiciones y entradas corruptas del constructor.

#include "test_helpers.h"

using namespace th;

#ifndef SEMANTIC_DIR
#define SEMANTIC_DIR "semantic_analyzer"
#endif

// ---------------------------------------------------------------- catalogo

static void testCatalogoEnMemoria() {
    sem::CatalogManager c;
    std::string err;
    CHECK(c.addTable("t", "disk", err));
    CHECK(!c.addTable("t", "disk", err));                 // duplicada
    CHECK(err.find("ya existe") != std::string::npos);
    CHECK(!c.addColumn("nada", "x", sem::DataType::INT, false, true, err));   // tabla inexistente
    CHECK(c.addColumn("t", "a", sem::DataType::INT, true, false, err));
    CHECK(c.addColumn("t", "b", sem::DataType::VARCHAR, false, true, err));
    CHECK(c.addColumn("t", "c", sem::DataType::FLOAT, false, true, err));
    CHECK(!c.addColumn("t", "a", sem::DataType::INT, false, true, err));      // columna duplicada
    CHECK(err.find("ya existe") != std::string::npos);

    CHECK_EQ(c.tableCount(), (size_t)1);
    CHECK_EQ(c.findTable("t")->columns.size(), (size_t)3);
    CHECK_EQ(c.findColumn("t", "a")->ordinal, 0);          // ordinal desde 0, en orden de declaracion
    CHECK_EQ(c.findColumn("t", "b")->ordinal, 1);
    CHECK_EQ(c.findColumn("t", "c")->ordinal, 2);
    CHECK(c.findColumn("t", "a")->primaryKey);
    CHECK(!c.findColumn("t", "a")->nullable);
    CHECK(c.findColumn("t", "b")->nullable);
    CHECK(c.findColumn("t", "c")->type == sem::DataType::FLOAT);
    CHECK_EQ(c.findColumn("t", "b")->table, std::string("t"));
    CHECK_EQ(c.findTable("t")->storage, std::string("disk"));
}

static void testCatalogoBusquedas() {
    auto c = makeCatalog();
    CHECK(c.findTable("usuarios") != nullptr);
    CHECK(c.findTable("Usuarios") == nullptr);            // sensible a mayusculas
    CHECK(c.findTable("nada") == nullptr);
    CHECK(c.findColumn("usuarios", "edad") != nullptr);
    CHECK(c.findColumn("usuarios", "Edad") == nullptr);
    CHECK(c.findColumn("usuarios", "titulo") == nullptr); // pertenece a productos
    CHECK(c.findColumn("nada", "id") == nullptr);
    CHECK(c.findColumn("productos", "titulo") != nullptr);
}

static void testCargaDeCadenas() {
    std::string err;
    {   // CRLF + lineas vacias + encabezado omitido
        sem::CatalogManager c;
        CHECK(c.loadFromString("table_name,column_name,data_type,is_primary_key\r\n\r\n"
                               "t,a,INT,true\r\nt,b,VARCHAR,false\r\n\r\n", err));
        CHECK_EQ(c.tableCount(), (size_t)1);
        CHECK_EQ(c.findTable("t")->columns.size(), (size_t)2);
        CHECK(c.findColumn("t", "a")->primaryKey);
        CHECK(!c.findColumn("t", "b")->primaryKey);       // 'false\r' no debe romper el parseo
    }
    {   // tipo en minusculas y pk como "1"
        sem::CatalogManager c;
        CHECK(c.loadFromString("h\nt,a,int,1\nt,b,Float,0\n", err));
        CHECK(c.findColumn("t", "a")->type == sem::DataType::INT);
        CHECK(c.findColumn("t", "a")->primaryKey);
        CHECK(c.findColumn("t", "b")->type == sem::DataType::FLOAT);
    }
    {   // tipo desconocido: queda UNKNOWN sin error (limitacion documentada)
        sem::CatalogManager c;
        CHECK(c.loadFromString("h\nt,a,XYZ,false\n", err));
        CHECK(c.findColumn("t", "a")->type == sem::DataType::UNKNOWN);
    }
    {   // columna repetida en el archivo
        sem::CatalogManager c;
        CHECK(!c.loadFromString("h\nt,a,INT,true\nt,a,INT,false\n", err));
        CHECK(err.find("ya existe") != std::string::npos);
    }
    {   // vacio / solo encabezado
        sem::CatalogManager c;
        CHECK(c.loadFromString("", err));
        CHECK_EQ(c.tableCount(), (size_t)0);
        CHECK(c.loadFromString("table_name,column_name,data_type,is_primary_key\n", err));
        CHECK_EQ(c.tableCount(), (size_t)0);
    }
}

static void testCargaDeArchivo() {
    sem::CatalogManager c;
    std::string err;
    CHECK(!c.loadFromFile("/ruta/que/no/existe.csv", err));
    CHECK(err.find("No se pudo abrir") != std::string::npos);

    CHECK(c.loadFromFile(std::string(SEMANTIC_DIR) + "/data/catalog.csv", err));
    CHECK_EQ(c.tableCount(), (size_t)9);
    struct T { const char* name; size_t cols; };
    const T expected[] = {{"usuarios", 5}, {"productos", 4}, {"ordenes", 4}, {"estudiantes", 8}, {"cursos", 6},
                          {"matriculas", 7}, {"empleados", 4}, {"departamentos", 2}, {"proyectos", 2}};
    for (const auto& t : expected) {
        const sem::TableInfo* ti = c.findTable(t.name);
        CHECK(ti != nullptr);
        if (ti) CHECK_EQ(ti->columns.size(), t.cols);
    }
    // Los cuatro tipos de dato del enunciado estan representados.
    CHECK(c.findColumn("usuarios", "id")->type == sem::DataType::INT);
    CHECK(c.findColumn("usuarios", "saldo")->type == sem::DataType::FLOAT);
    CHECK(c.findColumn("usuarios", "nombre")->type == sem::DataType::VARCHAR);
    CHECK(c.findColumn("usuarios", "activo")->type == sem::DataType::BOOLEAN);

    auto names = c.tableNames();                           // ordenadas alfabeticamente
    CHECK_EQ(names.size(), (size_t)9);
    CHECK_EQ(names.front(), std::string("cursos"));
    CHECK_EQ(names.back(), std::string("usuarios"));

    // Orden de columnas = orden de declaracion.
    const auto* p = c.findTable("productos");
    CHECK_EQ(p->columns[0].name, std::string("id"));
    CHECK_EQ(p->columns[1].name, std::string("titulo"));
    CHECK_EQ(p->columns[2].name, std::string("precio"));
    CHECK_EQ(p->columns[3].name, std::string("disponible"));
}

static void testTipoDesconocidoSinCascada() {
    auto c = makeCatalog("h\nt,a,XYZ,false\nt,b,INT,false\n");
    auto r = run("SELECT a FROM t WHERE a = 1 AND b = 2;", c);
    CHECK(r.built);
    CHECK(r.diags.empty());   // una columna UNKNOWN no produce errores de tipo
}

// ---------------------------------------------------------------- AST

static const sem::BinaryExprNode* bin(const sem::ASTNode* n) {
    return (n && n->kind() == sem::NodeKind::BINARY_EXPR) ? static_cast<const sem::BinaryExprNode*>(n) : nullptr;
}

static void testAstExacto() {
    auto c = makeCatalog();
    auto simple = run("SELECT nombre FROM usuarios;", c);
    CHECK_EQ(simple.before,
             std::string("SelectStmtNode\n"
                         "├─ selectList: ColumnNode(nombre)\n"
                         "└─ fromTable:  TableRefNode(usuarios)\n"));

    // Ejemplo de docs/sdt.md, seccion 5.
    auto r = run("SELECT nombre FROM usuarios WHERE edad >= 18 AND saldo > 0.5;", c);
    CHECK_EQ(r.before,
             std::string("SelectStmtNode\n"
                         "├─ selectList: ColumnNode(nombre)\n"
                         "├─ fromTable:  TableRefNode(usuarios)\n"
                         "└─ whereClause:\n"
                         "   BinaryExprNode(AND)\n"
                         "   ├─ BinaryExprNode(>=)\n"
                         "   │  ├─ ColumnNode(edad)\n"
                         "   │  └─ LiteralNode(18, INT)\n"
                         "   └─ BinaryExprNode(>)\n"
                         "      ├─ ColumnNode(saldo)\n"
                         "      └─ LiteralNode(0.5, FLOAT)\n"));
    CHECK_EQ(r.after,
             std::string("SelectStmtNode\n"
                         "├─ selectList: ColumnNode(nombre) : VARCHAR\n"
                         "├─ fromTable:  TableRefNode(usuarios)\n"
                         "└─ whereClause:\n"
                         "   BinaryExprNode(AND) : BOOLEAN\n"
                         "   ├─ BinaryExprNode(>=) : BOOLEAN\n"
                         "   │  ├─ ColumnNode(edad) : INT\n"
                         "   │  └─ LiteralNode(18, INT)\n"
                         "   └─ BinaryExprNode(>) : BOOLEAN\n"
                         "      ├─ ColumnNode(saldo) : FLOAT\n"
                         "      └─ LiteralNode(0.5, FLOAT)\n"));
}

static void testProyeccionYDestinos() {
    auto c = makeCatalog();
    auto r = run("SELECT id, nombre, edad FROM usuarios;", c);
    CHECK_EQ(r.ast->selectList.size(), (size_t)3);
    CHECK_EQ(static_cast<sem::ColumnNode*>(r.ast->selectList[0].get())->columnName, std::string("id"));
    CHECK_EQ(static_cast<sem::ColumnNode*>(r.ast->selectList[2].get())->columnName, std::string("edad"));
    CHECK(r.ast->whereClause == nullptr);
    CHECK_EQ(r.ast->fromTable->tableName, std::string("usuarios"));

    auto s = run("SELECT * FROM usuarios;", c);
    CHECK(s.before.find("selectList: StarNode") != std::string::npos);
}

static void testPrecedenciaYAsociatividad() {
    auto c = makeCatalog();
    {   // a AND b AND c  ->  AND(AND(a,b), c)
        auto r = run("SELECT id FROM usuarios WHERE id = 1 AND edad = 2 AND saldo = 3;", c);
        auto* root = bin(r.ast->whereClause.get());
        CHECK(root && root->op == sem::BinaryOp::AND);
        auto* l = root ? bin(root->left.get()) : nullptr;
        auto* rr = root ? bin(root->right.get()) : nullptr;
        CHECK(l && l->op == sem::BinaryOp::AND);
        CHECK(rr && rr->op == sem::BinaryOp::EQ);
    }
    {   // a OR b OR c  ->  OR(OR(a,b), c)
        auto r = run("SELECT id FROM usuarios WHERE id = 1 OR edad = 2 OR saldo = 3;", c);
        auto* root = bin(r.ast->whereClause.get());
        CHECK(root && root->op == sem::BinaryOp::OR);
        auto* l = root ? bin(root->left.get()) : nullptr;
        CHECK(l && l->op == sem::BinaryOp::OR);
    }
    {   // a OR b AND c  ->  OR(a, AND(b,c))   (AND liga mas fuerte)
        auto r = run("SELECT id FROM usuarios WHERE id = 1 OR edad = 2 AND saldo = 3;", c);
        auto* root = bin(r.ast->whereClause.get());
        CHECK(root && root->op == sem::BinaryOp::OR);
        auto* rr = root ? bin(root->right.get()) : nullptr;
        CHECK(rr && rr->op == sem::BinaryOp::AND);
        auto* l = root ? bin(root->left.get()) : nullptr;
        CHECK(l && l->op == sem::BinaryOp::EQ);
    }
    {   // a AND b OR c AND d  ->  OR(AND(a,b), AND(c,d))
        auto r = run("SELECT id FROM usuarios WHERE id = 1 AND edad = 2 OR saldo = 3 AND id = 4;", c);
        auto* root = bin(r.ast->whereClause.get());
        CHECK(root && root->op == sem::BinaryOp::OR);
        auto* l = root ? bin(root->left.get()) : nullptr;
        auto* rr = root ? bin(root->right.get()) : nullptr;
        CHECK(l && l->op == sem::BinaryOp::AND);
        CHECK(rr && rr->op == sem::BinaryOp::AND);
    }
}

static void testOperadores() {
    auto c = makeCatalog();
    struct Op { const char* text; sem::BinaryOp op; };
    const Op ops[] = {{"=", sem::BinaryOp::EQ}, {"<>", sem::BinaryOp::NEQ}, {"!=", sem::BinaryOp::NEQ},
                      {"<", sem::BinaryOp::LT}, {">", sem::BinaryOp::GT}, {"<=", sem::BinaryOp::LE},
                      {">=", sem::BinaryOp::GE}};
    for (const auto& o : ops) {
        auto r = run(std::string("SELECT id FROM usuarios WHERE id ") + o.text + " 1;", c);
        auto* b = r.built ? bin(r.ast->whereClause.get()) : nullptr;
        CHECK(b != nullptr);
        if (b) CHECK(b->op == o.op);
    }
}

static void testLiterales() {
    auto c = makeCatalog();
    auto lit = [&](const std::string& rhs) {
        auto r = run("SELECT id FROM usuarios WHERE id = " + rhs + ";", c);
        auto* b = r.built ? bin(r.ast->whereClause.get()) : nullptr;
        return (b && b->right && b->right->kind() == sem::NodeKind::LITERAL)
                   ? static_cast<const sem::LiteralNode*>(b->right.get()) : nullptr;
    };
    auto* i = lit("42");
    CHECK(i && i->literalType == sem::DataType::INT && i->value == "42");
    auto* f = lit("3.5");
    CHECK(f && f->literalType == sem::DataType::FLOAT);
    auto* s = lit("'hola'");
    CHECK(s && s->literalType == sem::DataType::VARCHAR && s->value == "hola");   // sin comillas

    // Value -> id produce una columna, no un literal.
    auto r = run("SELECT id FROM usuarios WHERE id = edad;", c);
    auto* b = bin(r.ast->whereClause.get());
    CHECK(b && b->right && b->right->kind() == sem::NodeKind::COLUMN);
}

static void testPosiciones() {
    auto c = makeCatalog();
    auto r = run("SELECT id FROM usuarios WHERE id = 1;", c);
    CHECK_EQ(r.ast->line, 1);
    CHECK_EQ(r.ast->column, 1);                    // SELECT
    CHECK_EQ(r.ast->fromTable->column, 16);        // el identificador de la tabla
    auto* b = bin(r.ast->whereClause.get());
    CHECK(b != nullptr);
    if (b) {
        CHECK_EQ(b->column, 34);                   // el OPERADOR
        CHECK_EQ(b->left->column, 31);
        CHECK_EQ(b->right->column, 36);
    }
    auto m = run("SELECT id\nFROM usuarios\nWHERE id = 1;", c);
    CHECK_EQ(m.ast->fromTable->line, 2);
    CHECK_EQ(m.ast->fromTable->column, 6);
    auto* mb = bin(m.ast->whereClause.get());
    CHECK(mb && mb->line == 3 && mb->column == 10 && mb->left->column == 7);
}

static void testConstructorConEntradasCorruptas() {
    Lexer lx("SELECT * FROM t;");
    auto toks = lx.tokenizeAll();
    auto pr = ll1::parseTokens(toks, ll1::defaultParseTable());
    CHECK(pr.accepted);
    CHECK(sem::buildAst(toks, pr.productions).ok);            // control: la entrada buena se construye

    auto safe = [&](const std::vector<Token>& t, std::vector<int> p) {
        bool threw = false, ok = true;
        try { auto r = sem::buildAst(t, p); ok = r.ok; if (!ok) { CHECK(r.ast == nullptr); CHECK(!r.error.empty()); } }
        catch (...) { threw = true; }
        CHECK(!threw);
        return ok;
    };
    CHECK(!safe(toks, {}));                                   // vacia
    CHECK(!safe(toks, {1, 99}));                              // numero invalido
    CHECK(!safe(toks, {0}));
    CHECK(!safe(toks, {-3}));
    CHECK(!safe(toks, {2, 1}));                               // orden incorrecto
    auto truncated = pr.productions; truncated.pop_back();
    CHECK(!safe(toks, truncated));                            // incompleta
    auto extra = pr.productions; extra.push_back(5);
    CHECK(!safe(toks, extra));                                // con sobrantes
    CHECK(!safe({}, pr.productions));                         // sin tokens
    std::vector<Token> soloEof = {toks.back()};
    CHECK(!safe(soloEof, pr.productions));
}

int main() {
    testCatalogoEnMemoria();
    testCatalogoBusquedas();
    testCargaDeCadenas();
    testCargaDeArchivo();
    testTipoDesconocidoSinCascada();
    testAstExacto();
    testProyeccionYDestinos();
    testPrecedenciaYAsociatividad();
    testOperadores();
    testLiterales();
    testPosiciones();
    testConstructorConEntradasCorruptas();
    return finish("test_semantic_ast");
}
