#include "palindromoModule.h"
#include <iostream>
#include <string>
#include <algorithm>

using namespace std;

bool validarPalindromo(const string& texto) {
    string textoLimpio = "";
    for (char c : texto) {
        if (isalpha((unsigned char)c) || isdigit((unsigned char)c)) {
            textoLimpio += tolower((unsigned char)c);
        }
    }
    string textoInvertido = textoLimpio;
    reverse(textoInvertido.begin(), textoInvertido.end());
    return (textoLimpio == textoInvertido && textoLimpio.length() > 0);
}

void menuPalindromo() {
    int opcion = -1;
    string texto;
    do {
        cout << "\n===============================\n";
        cout << "   REVISION DE PALINDROMOS  \n";
        cout << "===============================\n";
        cout << "1) Validar\n";
        cout << "2) Cancelar\n";
        cout << "Seleccione una opcion: ";
        cin >> opcion;

        if (opcion == 1) {
            cout << "\nIngrese una palabra o frase: ";
            getline(cin >> ws, texto); 

            if (validarPalindromo(texto)) {
                cout << "\nEl texto ES un palíndromo.\n";
            } else {
                cout << "\nEl texto NO es un palíndromo.\n";
            }
        } else if (opcion != 2) {
            cout << "Opcion invalida. Intente de nuevo.\n";
        }
    } while (opcion != 2);
}