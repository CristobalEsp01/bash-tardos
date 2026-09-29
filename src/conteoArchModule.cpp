#include "conteoArchModule.h"
#include "conteoTextoModule.h"   // se reutiliza el mismo conteo de la opcion 6
#include "utils.h"

#include <iostream>
#include <string>

using namespace std;

void conteoSobreArchivo(const string& carpetaSugerida) {
    while (true) {
        cout << "\n=================================\n";
        cout << "      CONTEO SOBRE ARCHIVO\n";
        cout << "=================================\n";
        cout << "Ingrese la ruta de un archivo de texto.\n";
        if (!carpetaSugerida.empty()) {
            cout << "(Ej: " << carpetaSugerida << "/ciencia_ficcion/frankenstein_mary_shelley.txt)\n";
        }
        cout << "Escriba 0 para VOLVER al menu principal.\n";
        string ruta = leerLinea("Ruta: ");

        if (ruta == "0") return;
        if (ruta.empty()) {
            cout << "Debe ingresar una ruta.\n";
            continue;
        }
        // permite pegar rutas entre comillas
        if (ruta.size() >= 2 && (ruta.front() == '"' || ruta.front() == '\'') && ruta.back() == ruta.front()) {
            ruta = ruta.substr(1, ruta.size() - 2);
        }
        conteoSobreTexto(ruta);
        return;
    }
}
