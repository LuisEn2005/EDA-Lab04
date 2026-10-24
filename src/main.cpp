#include "../include/lexer.h"
#include <iostream>
#include <iomanip>

void runTestCase(const std::string& testName, const std::string& query) {
    std::cout << "========================================\n";
    std::cout << "Prueba: " << testName << "\n";
    std::cout << "Entrada:\n" << query << "\n";
    std::cout << "----------------------------------------\n";

    Lexer lexer(query);
    std::vector<Token> tokens = lexer.tokenizeAll();

    std::cout << std::left << std::setw(22) << "Lexema"
              << std::setw(20) << "Valor"
              << std::setw(20) << "Tipo de Token"
              << std::setw(10) << "Línea"
              << std::setw(10) << "Columna" << "\n";
    std::cout << std::string(62, '-') << "\n";

    for (const auto& token : tokens) {
        std::cout << std::left << std::setw(22) << token.lexeme
                  << std::setw(20) << token.value
                  << std::setw(20) << tokenTypeToString(token.type)
                  << std::setw(10) << token.line
                  << std::setw(10) << token.column << "\n";
    }

    const auto& diags = lexer.diagnostics();
    if (!diags.empty()) {
        std::cout << "\nDiagnosticos:\n";
        for (const auto& d : diags) {
            std::cout << "  [" << severityToString(d.severity) << "] "
                      << d.line << ":" << d.column << " " << d.message << "\n";
        }
    }
    std::cout << "\n";
}

int main() {
    runTestCase("Sentencia SELECT completa", 
        "SELECT nombre, edad\n"
        "FROM estudiantes\n"
        "WHERE edad >= 18 AND carrera = 'CC' AND promedio <> 10.5;");

    runTestCase("Operadores compuestos (<>, !=, <=, >=)",
        "SELECT * FROM datos WHERE a <= 10 AND b >= 20 AND c <> 30 AND d != 40;");

    runTestCase("Numeros: negativos, .5 y 5.",
        "SELECT a FROM t WHERE x = -3 AND y = -.5 AND z = .5 AND w = 5.;");
 
    runTestCase("Cadenas con escapes",
        "SELECT a FROM t WHERE n = 'it\\'s' AND m = 'a\\\\b' AND k = 'x\\ny';");

    runTestCase("Errores léxicos (cadena sin cerrar y símbolo no válido)",
        "SELECT @campo FROM tabla WHERE texto = 'cadena incompleta;");

    return 0;
}
