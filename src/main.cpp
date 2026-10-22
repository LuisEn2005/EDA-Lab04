#include "Lexer.h"
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
              << std::setw(20) << "Tipo de Token"
              << std::setw(10) << "Línea"
              << std::setw(10) << "Columna" << "\n";
    std::cout << std::string(62, '-') << "\n";

    for (const auto& token : tokens) {
        std::cout << std::left << std::setw(22) << token.lexeme
                  << std::setw(20) << tokenTypeToString(token.type)
                  << std::setw(10) << token.line
                  << std::setw(10) << token.column << "\n";
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

    runTestCase("Errores léxicos (cadena sin cerrar y símbolo no válido)",
        "SELECT @campo FROM tabla WHERE texto = 'cadena incompleta;");

    return 0;
}
