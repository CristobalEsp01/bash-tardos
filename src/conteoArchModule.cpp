#include "conteoArchModule.h"
#include "conteoTextoModule.h" // Incluimos este para reciclar la función y no copiar código a lo wn
#include <iostream>
#include <string>

using namespace std;

void conteoSobreArchivo() {
    string rutaArchivo;
    cout << "\n=================================\n";
    cout << "      CONTEO SOBRE ARCHIVO\n";
    cout << "=================================\n";
    cout << "Ingrese la ruta del archivo (path): ";
    getline(cin >> ws, rutaArchivo);
    
    // Llamamos a la función del otro módulo
    conteoSobreTexto(rutaArchivo);
}