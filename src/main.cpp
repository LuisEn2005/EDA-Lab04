#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include "lexer.h"
#include "token.h"
#include "input.h"

void printTokens(const std::vector<Token>& tokens) {
    std::cout << std::left
              << std::setw(22) << "Lexema"
              << std::setw(20) << "Valor"
              << std::setw(20) << "Tipo de Token"
              << std::setw(8)  << "Linea"
              << std::setw(8)  << "Columna"
              << "\n";
    std::cout << std::string(78, '-') << "\n";

    for (const auto& token : tokens) {
        std::cout << std::left
                  << std::setw(22) << token.lexeme
                  << std::setw(20) << token.value
                  << std::setw(20) << tokenTypeToString(token.type)
                  << std::setw(8)  << token.line
                  << std::setw(8)  << token.column
                  << "\n";
    }
}

void printDiagnostics(const std::vector<Diagnostic>& diags) {
    if (diags.empty()) return;

    std::cout << "\nDiagnosticos:\n";
    for (const auto& diag : diags) {
        std::string severity = (diag.severity == Severity::ERROR) ? "[ERROR]" : "[WARNING]";
        std::cout << "  " << severity << " " << diag.line << ":" << diag.column
                  << " " << diag.message << "\n";
    }
}

// Ejecuta un caso de prueba leyendo directamente un archivo .sql
void runTestCaseFile(const std::string& title, const std::string& filepath) {
    std::cout << "========================================\n";
    std::cout << "Prueba: " << title << "\n";
    std::cout << "Archivo: " << filepath << "\n";
    std::cout << "----------------------------------------\n";

    std::string source;
    std::string errorMessage;

    if (!readFile(filepath, source, errorMessage)) {
        std::cerr << errorMessage << "\n\n";
        return;
    }

    std::cout << "Entrada:\n" << source << "\n";
    std::cout << "----------------------------------------\n";

    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenizeAll();
    printTokens(tokens);
    printDiagnostics(lexer.diagnostics());
    std::cout << "\n";
}

void runDemo() {
    runTestCaseFile("1. Sentencia SELECT completa", "examples/valida.sql");
    runTestCaseFile("2. Operadores compuestos (<>, !=, <=, >=)", "examples/operadores.sql");
    runTestCaseFile("3. Numeros: negativos, .5 y 5.", "examples/literales.sql");
    runTestCaseFile("4. Cadenas con secuencias de escape", "examples/cadenas.sql");
    runTestCaseFile("5. Manejo de espaciado y tabulaciones", "examples/espacios.sql");
    runTestCaseFile("6. Errores lexicos (simbolo no valido y cadena sin- cerrar)", "examples/errores.sql");
    runTestCaseFile("7. Caso limite (archivo vacio -> solo EOF)", "examples/limite.sql");
}

int main(int argc, char* argv[]) {
    
    if (argc > 1 && std::string(argv[1]) == "--demo") {
        runDemo();
        return 0;
    }

    std::string source;

    
    if (argc > 1) {
        std::string filepath = argv[1];
        std::string errorMessage;
        if (!readFile(filepath, source, errorMessage)) {
            std::cerr << errorMessage << "\n";
            return 1;
        }
    } 
    
    else {
        source = readStandardInput();
    }

    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenizeAll();

    printTokens(tokens);
    printDiagnostics(lexer.diagnostics());

    
    return lexer.hasErrors() ? 1 : 0;
}