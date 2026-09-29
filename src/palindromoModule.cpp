#include "palindromoModule.h"
#include "utils.h"

#include <algorithm>
#include <iostream>
#include <string>

using namespace std;

// Un texto es palindromo si, considerando solo letras y digitos, se lee
// igual en ambos sentidos. Ignora mayusculas, espacios, signos y tildes
// (ej: "Dabale arroz a la zorra el abad" con o sin tildes).
bool validarPalindromo(const string& texto) {
    string limpio;
    size_t i = 0;
    while (i < texto.size()) {
        unsigned int cp = siguienteCaracterUtf8(texto, i);
        char base = letraBase(cp);
        if (base) limpio += base;
        else if (cp >= '0' && cp <= '9') limpio += static_cast<char>(cp);
    }
    if (limpio.empty()) return false;
    string invertido(limpio.rbegin(), limpio.rend());
    return limpio == invertido;
}

void menuPalindromo() {
    while (true) {
        cout << "\n===============================\n";
        cout << "        ES PALINDROMO?\n";
        cout << "===============================\n";
        string texto = leerLinea("Escriba un texto: ");

        cout << "\n1) Validar\n";
        cout << "2) Cancelar\n";
        int opcion = leerEntero("Seleccione una opcion: ");
        while (opcion != 1 && opcion != 2) {
            cout << "Opcion invalida. Ingrese 1 o 2.\n";
            opcion = leerEntero("Seleccione una opcion: ");
        }

        if (opcion == 2) {
            cout << "Volviendo al MENU PRINCIPAL...\n";
            return;
        }

        if (texto.empty()) {
            cout << "\nNo ingreso ningun texto.\n";
        } else if (validarPalindromo(texto)) {
            cout << "\n\"" << texto << "\" SI es un palindromo.\n";
        } else {
            cout << "\n\"" << texto << "\" NO es un palindromo.\n";
        }
    }
}
