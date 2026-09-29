#ifndef UTILS_H
#define UTILS_H

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

// ---------------------------------------------------------------------
// Utilidades compartidas por todos los programas del sistema
// ---------------------------------------------------------------------

// Limpia el estado de error de cin y descarta lo que quede en la linea
inline void limpiarBuffer() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// Quita espacios, tabs y \r al inicio y al final
inline std::string trim(const std::string& s) {
    const char* ws = " \t\r\n";
    size_t ini = s.find_first_not_of(ws);
    if (ini == std::string::npos) return "";
    size_t fin = s.find_last_not_of(ws);
    return s.substr(ini, fin - ini + 1);
}

// Lee una linea completa desde teclado (sin espacios en los extremos).
// Si se cierra la entrada estandar (Ctrl+D) termina el programa de forma
// ordenada en vez de quedar en un bucle infinito.
inline std::string leerLinea(const std::string& mensaje) {
    std::cout << mensaje;
    std::string linea;
    if (!std::getline(std::cin, linea)) {
        std::cout << "\nEntrada finalizada. Saliendo del programa.\n";
        std::exit(0);
    }
    return trim(linea);
}

// Pide un numero entero y repite la pregunta hasta que sea valido.
// Evita que una letra deje a cin en estado de error (bucle infinito).
inline int leerEntero(const std::string& mensaje) {
    while (true) {
        std::string linea = leerLinea(mensaje);
        try {
            size_t pos = 0;
            int valor = std::stoi(linea, &pos);
            if (pos == linea.size()) return valor;
        } catch (...) {}
        std::cout << "Entrada invalida: debe ingresar un numero entero.\n";
    }
}

// Pide un numero real y repite la pregunta hasta que sea valido.
inline double leerReal(const std::string& mensaje) {
    while (true) {
        std::string linea = leerLinea(mensaje);
        for (char& c : linea) if (c == ',') c = '.';   // acepta 2,5 y 2.5
        try {
            size_t pos = 0;
            double valor = std::stod(linea, &pos);
            if (pos == linea.size()) return valor;
        } catch (...) {}
        std::cout << "Entrada invalida: debe ingresar un numero real (ej: 2.5).\n";
    }
}

inline void pausar(const std::string& mensaje = "Presione ENTER para VOLVER...") {
    std::cout << mensaje;
    std::string tmp;
    if (!std::getline(std::cin, tmp)) std::exit(0);
}

// ---------------------------------------------------------------------
// Soporte basico de UTF-8 para textos en espanol (vocales con tilde,
// u con dieresis y enie). Lee un "caracter" desde texto[i] y avanza i.
// Devuelve el codepoint Unicode (o 0xFFFD si la secuencia es invalida).
// ---------------------------------------------------------------------
inline unsigned int siguienteCaracterUtf8(const std::string& texto, size_t& i) {
    unsigned char c = texto[i];
    int largo = 1;
    unsigned int cp = c;
    if (c >= 0xF0)      { largo = 4; cp = c & 0x07; }
    else if (c >= 0xE0) { largo = 3; cp = c & 0x0F; }
    else if (c >= 0xC0) { largo = 2; cp = c & 0x1F; }
    else if (c >= 0x80) { i += 1; return 0xFFFD; }
    if (largo > 1) {
        if (i + largo > texto.size()) { i = texto.size(); return 0xFFFD; }
        for (int k = 1; k < largo; k++) {
            unsigned char cont = texto[i + k];
            if ((cont & 0xC0) != 0x80) { i += 1; return 0xFFFD; }
            cp = (cp << 6) | (cont & 0x3F);
        }
    }
    i += largo;
    return cp;
}

// Convierte una letra (incluidas las acentuadas) a su letra base en
// minuscula: 'A','a','á','Á' -> 'a'; 'ñ' -> 'n'.
// Devuelve 0 si el caracter no es una letra.
inline char letraBase(unsigned int cp) {
    if (cp < 128) {
        if (cp >= 'A' && cp <= 'Z') return static_cast<char>(cp - 'A' + 'a');
        if (cp >= 'a' && cp <= 'z') return static_cast<char>(cp);
        return 0;
    }
    switch (cp) {
        case 0xE1: case 0xC1: case 0xE0: case 0xC0: case 0xE4: case 0xC4: case 0xE2: case 0xC2: return 'a';
        case 0xE9: case 0xC9: case 0xE8: case 0xC8: case 0xEB: case 0xCB: case 0xEA: case 0xCA: return 'e';
        case 0xED: case 0xCD: case 0xEC: case 0xCC: case 0xEF: case 0xCF: case 0xEE: case 0xCE: return 'i';
        case 0xF3: case 0xD3: case 0xF2: case 0xD2: case 0xF6: case 0xD6: case 0xF4: case 0xD4: return 'o';
        case 0xFA: case 0xDA: case 0xF9: case 0xD9: case 0xFC: case 0xDC: case 0xFB: case 0xDB: return 'u';
        case 0xF1: case 0xD1: return 'n';   // enie
        case 0xE7: case 0xC7: return 'c';   // c cedilla
        default: return 0;
    }
}

inline bool esVocal(char base) {
    return base == 'a' || base == 'e' || base == 'i' || base == 'o' || base == 'u';
}

// Antes de agregar un registro al final de un archivo, se asegura de que
// la ultima linea termine en salto de linea (si no, el nuevo registro
// quedaria pegado al anterior y se corromperia el archivo).
inline void asegurarSaltoDeLineaFinal(const std::string& ruta) {
    std::ifstream in(ruta, std::ios::binary | std::ios::ate);
    if (!in.is_open() || in.tellg() == 0) return;
    in.seekg(-1, std::ios::end);
    char ultimo = 0;
    in.get(ultimo);
    in.close();
    if (ultimo != '\n') {
        std::ofstream out(ruta, std::ios::app);
        out << '\n';
    }
}

// Formatea un numero real sin ceros innecesarios (19.25 y no 19.250000)
inline std::string formatearReal(double valor) {
    std::ostringstream os;
    os << std::setprecision(12) << valor;
    return os.str();
}

#endif
