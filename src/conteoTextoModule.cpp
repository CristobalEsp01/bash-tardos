#include "conteoTextoModule.h"
#include "utils.h"

#include <filesystem>
#include <fstream>
#include <iostream>

using namespace std;
namespace fs = std::filesystem;

bool contarArchivo(const string& rutaArchivo, ResultadoConteo& r) {
    r = ResultadoConteo();

    error_code ec;
    if (!fs::exists(rutaArchivo, ec)) {
        cout << "\n[ERROR] El archivo no existe: " << rutaArchivo << "\n";
        return false;
    }
    if (!fs::is_regular_file(rutaArchivo, ec)) {
        cout << "\n[ERROR] La ruta no corresponde a un archivo: " << rutaArchivo << "\n";
        return false;
    }
    ifstream archivo(rutaArchivo, ios::binary);
    if (!archivo.is_open()) {
        cout << "\n[ERROR] No se pudo abrir el archivo (revise permisos): " << rutaArchivo << "\n";
        return false;
    }

    // Se procesa linea a linea (un caracter UTF-8 nunca cruza lineas),
    // asi archivos grandes (libros de varios MB) no se cargan completos.
    string linea;
    while (getline(archivo, linea)) {
        bool enPalabra = false;
        size_t i = 0;
        while (i < linea.size()) {
            unsigned int cp = siguienteCaracterUtf8(linea, i);
            char base = letraBase(cp);
            bool esDigito = (cp >= '0' && cp <= '9');

            if (base) {
                if (esVocal(base)) r.vocales++;
                else r.consonantes++;
            } else if (!esDigito) {
                bool esEspacio = (cp == ' ' || cp == '\t' || cp == '\r' ||
                                  cp == '\v' || cp == '\f' || cp == 0xA0 || cp == 0xFEFF);
                if (!esEspacio) r.especiales++;
            }

            if (base || esDigito) {
                if (!enPalabra) { r.palabras++; enPalabra = true; }
            } else {
                enPalabra = false;
            }
        }
    }
    return true;
}

void conteoSobreTexto(const string& rutaArchivo) {
    ResultadoConteo r;
    cout << "\n=================================\n";
    cout << "        CONTEO SOBRE TEXTO\n";
    cout << "=================================\n";
    cout << "Archivo: " << rutaArchivo << "\n";

    if (contarArchivo(rutaArchivo, r)) {
        cout << "---------------------------------\n";
        cout << "Vocales               : " << r.vocales << "\n";
        cout << "Consonantes           : " << r.consonantes << "\n";
        cout << "Caracteres especiales : " << r.especiales << "\n";
        cout << "Palabras              : " << r.palabras << "\n";
        cout << "---------------------------------\n";
    }
    pausar("Presione ENTER para VOLVER...");
}
