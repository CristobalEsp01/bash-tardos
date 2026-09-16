#include "conteoTextoModule.h"
#include <iostream>
#include <fstream>
#include <cctype>

using namespace std;

void conteoSobreTexto(const string& rutaArchivo) {
    ifstream archivo(rutaArchivo);
    if (!archivo.is_open()) {
        cout << "\n[ERROR] No se pudo abrir el archivo: " << rutaArchivo << "\n";
        return; 
    }

    int vocales = 0, consonantes = 0, especiales = 0, palabras = 0;
    bool enPalabra = false;
    char c;

    while (archivo.get(c)) {
        unsigned char uc = c;
        char minuscula = tolower(uc);

        if (isalpha(uc)) {
            if (minuscula == 'a' || minuscula == 'e' || minuscula == 'i' || 
                minuscula == 'o' || minuscula == 'u') {
                vocales++;
            } else {
                consonantes++;
            }
            if (!enPalabra) {
                palabras++;
                enPalabra = true;
            }
        } else {
            enPalabra = false;
            if (!isalnum(uc) && !isspace(uc)) {
                especiales++;
            }
        }
    }
    archivo.close();

    int opcion;
    do {
        cout << "\n=================================\n";
        cout << "             CONTEO\n";
        cout << "=================================\n";
        cout << "Vocales: " << vocales << "\n";
        cout << "Consonantes: " << consonantes << "\n";
        cout << "Caracteres especiales: " << especiales << "\n";
        cout << "Palabras: " << palabras << "\n";
        cout << "---------------------------------\n";
        cout << "1) VOLVER\n";
        cout << "Seleccione una opcion: ";
        cin >> opcion;
    } while (opcion != 1);
}