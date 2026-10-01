// =====================================================================
// multi - Programa multiplicador de matrices NxM
//
// Uso: ./bin/multi "<ruta completa A.txt>" "<ruta completa B.txt>" "<separador>" "<usuario>" "<perfil>"
// Ej:  ./bin/multi "/home/lvc/a.txt" "/home/lvc/b.txt" "#" lvc ADMIN
//
// Cada archivo contiene una matriz: una fila por linea y los elementos
// (numeros enteros o decimales, con punto) separados por el separador
// indicado. Ej. con '#':
//   1#2.5#3
//   -4#0.75#6
//
// Codigos de salida:
//   0 ok | 1 uso incorrecto | 2 archivo invalido | 3 separador invalido
//   4 contenido/formato invalido | 5 dimensiones incompatibles | 6 desborde
// =====================================================================
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
namespace fs = std::filesystem;

using Matriz = vector<vector<double>>;

enum CodigoSalida { OK = 0, USO = 1, ARCHIVO = 2, SEPARADOR = 3, FORMATO = 4, DIMENSIONES = 5, DESBORDE = 6 };

static string quitarEspacios(const string& s) {
    size_t ini = s.find_first_not_of(" \t\r");
    if (ini == string::npos) return "";
    size_t fin = s.find_last_not_of(" \t\r");
    return s.substr(ini, fin - ini + 1);
}

static string mostrarSeparador(char sep) {
    if (sep == ' ') return "' ' (espacio)";
    if (sep == '\t') return "'\\t' (tabulacion)";
    return string("'") + sep + "'";
}

// Valida que el token sea un numero real escrito de forma simple:
//   signo opcional, digitos y como maximo un punto decimal con al menos
//   un digito en total. Validos: 12  -3  +4  2.5  -0.75  .5  3.
//   Invalidos: 12a  1.2.3  1e5  ,5  nan  inf
static bool esNumero(const string& token) {
    size_t i = (token[0] == '-' || token[0] == '+') ? 1 : 0;
    bool hayDigito = false, hayPunto = false;
    for (; i < token.size(); i++) {
        char c = token[i];
        if (isdigit(static_cast<unsigned char>(c))) {
            hayDigito = true;
        } else if (c == '.' && !hayPunto) {
            hayPunto = true;
        } else {
            return false;
        }
    }
    return hayDigito;
}

// Muestra un numero sin ceros innecesarios: 7 -> "7", 2.50 -> "2.5",
// 0.1*3 -> "0.3" (se redondea a 6 decimales para ocultar el error de
// representacion de los double).
static string formatearNumero(double v) {
    if (fabs(v) >= 1e15) {
        ostringstream os;
        os << setprecision(6) << scientific << v;
        return os.str();
    }
    ostringstream os;
    os << fixed << setprecision(6) << v;
    string s = os.str();
    s.erase(s.find_last_not_of('0') + 1);
    if (s.back() == '.') s.pop_back();
    if (s == "-0") s = "0";
    return s;
}

// Valida que la ruta exista, sea completa, sea un archivo y se pueda leer
static bool validarArchivo(const string& ruta, const string& nombre) {
    fs::path p(ruta);
    error_code ec;
    if (!p.is_absolute()) {
        cerr << "[ERROR] La ruta del archivo " << nombre << " debe ser completa (absoluta): " << ruta << "\n";
        return false;
    }
    if (!fs::exists(p, ec)) {
        cerr << "[ERROR] El archivo " << nombre << " no existe: " << ruta << "\n";
        return false;
    }
    if (!fs::is_regular_file(p, ec)) {
        cerr << "[ERROR] La ruta del archivo " << nombre << " no corresponde a un archivo: " << ruta << "\n";
        return false;
    }
    ifstream prueba(p);
    if (!prueba.is_open()) {
        cerr << "[ERROR] No se pudo leer el archivo " << nombre << " (revise permisos): " << ruta << "\n";
        return false;
    }
    return true;
}

// Lee la matriz validando formato (separador), contenido (enteros) y que
// todas las filas tengan la misma cantidad de columnas.
static int leerMatriz(const string& ruta, const string& nombre, char sep, Matriz& m) {
    m.clear();
    ifstream archivo(ruta);
    string linea;
    int numLinea = 0;

    while (getline(archivo, linea)) {
        numLinea++;
        if (!linea.empty() && linea.back() == '\r') linea.pop_back();
        if (quitarEspacios(linea).empty()) continue;   // se ignoran lineas en blanco

        // Si el separador no es espacio/tab, se ignoran espacios alrededor de cada numero
        bool sepEsBlanco = (sep == ' ' || sep == '\t');
        string contenido = sepEsBlanco ? linea : quitarEspacios(linea);

        vector<double> fila;
        stringstream ss(contenido);
        string token;
        int columna = 0;
        bool terminaEnSeparador = !contenido.empty() && contenido.back() == sep;

        while (getline(ss, token, sep)) {
            columna++;
            string limpio = sepEsBlanco ? token : quitarEspacios(token);
            if (limpio.empty()) {
                cerr << "[ERROR] Matriz " << nombre << ", linea " << numLinea << ", elemento " << columna
                     << ": elemento vacio (separador " << mostrarSeparador(sep)
                     << " repetido o al inicio de la linea).\n";
                return FORMATO;
            }
            if (!esNumero(limpio)) {
                cerr << "[ERROR] Matriz " << nombre << ", linea " << numLinea << ", elemento " << columna
                     << ": '" << limpio << "' no es un numero valido (entero o decimal con punto).\n";
                bool tieneDigitos = false;
                for (char c : limpio) if (isdigit(static_cast<unsigned char>(c))) tieneDigitos = true;
                if (tieneDigitos) {
                    cerr << "        Verifique que el separador del archivo sea " << mostrarSeparador(sep) << ".\n";
                }
                return FORMATO;
            }
            try {
                double valor = stod(limpio);
                if (!isfinite(valor)) throw out_of_range("valor");
                fila.push_back(valor);
            } catch (const out_of_range&) {
                cerr << "[ERROR] Matriz " << nombre << ", linea " << numLinea << ": el valor '"
                     << limpio << "' es demasiado grande.\n";
                return FORMATO;
            }
        }
        if (terminaEnSeparador) {
            cerr << "[ERROR] Matriz " << nombre << ", linea " << numLinea
                 << ": la linea termina con el separador (falta un elemento).\n";
            return FORMATO;
        }

        if (!m.empty() && fila.size() != m[0].size()) {
            cerr << "[ERROR] Matriz " << nombre << ": la linea " << numLinea << " tiene " << fila.size()
                 << " columnas, pero las filas anteriores tienen " << m[0].size() << ".\n";
            return FORMATO;
        }
        m.push_back(fila);
    }

    if (m.empty()) {
        cerr << "[ERROR] El archivo de la matriz " << nombre << " esta vacio: " << ruta << "\n";
        return FORMATO;
    }
    return OK;
}

// C (filas(A) x columnas(B)) = A x B. Devuelve false si algun resultado
// se desborda (queda infinito o indefinido).
static bool multiplicar(const Matriz& a, const Matriz& b, Matriz& c) {
    size_t n = a.size(), k = b.size(), p = b[0].size();
    c.assign(n, vector<double>(p, 0.0));
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < p; j++) {
            double suma = 0.0;
            for (size_t t = 0; t < k; t++) {
                suma += a[i][t] * b[t][j];
            }
            if (!isfinite(suma)) return false;
            c[i][j] = suma;
        }
    }
    return true;
}

static void imprimirMatriz(const string& titulo, const Matriz& m) {
    size_t ancho = 1;
    for (const auto& fila : m)
        for (double v : fila) ancho = max(ancho, formatearNumero(v).size());

    cout << titulo << " (" << m.size() << "x" << m[0].size() << "):\n";
    for (const auto& fila : m) {
        cout << "  ";
        for (size_t j = 0; j < fila.size(); j++) {
            cout << setw(static_cast<int>(ancho)) << formatearNumero(fila[j]) << (j + 1 < fila.size() ? "  " : "");
        }
        cout << "\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc != 6) {
        cerr << "Uso: " << argv[0]
             << " \"<ruta completa A.txt>\" \"<ruta completa B.txt>\" \"<separador>\" \"<usuario>\" \"<perfil>\"\n";
        cerr << "Ej:  " << argv[0] << " \"/home/lvc/a.txt\" \"/home/lvc/b.txt\" \"#\" lvc ADMIN\n";
        return USO;
    }

    string rutaA = argv[1], rutaB = argv[2], sepStr = argv[3];
    string usuario = argv[4], perfil = argv[5];

    cout << "\n======================================\n";
    cout << "     MULTIPLICADOR DE MATRICES NxM\n";
    cout << "======================================\n";
    cout << "Usuario: " << usuario << "   |   Perfil: " << perfil << "\n";
    cout << "--------------------------------------\n";

    if (usuario.empty() || perfil.empty()) {
        cerr << "[ERROR] El usuario y el perfil no pueden estar vacios.\n";
        return USO;
    }

    // --- validar separador ---
    if (sepStr.size() != 1) {
        cerr << "[ERROR] El separador debe ser exactamente un caracter (recibido: \"" << sepStr << "\").\n";
        return SEPARADOR;
    }
    char sep = sepStr[0];
    if (isdigit(static_cast<unsigned char>(sep)) || sep == '-' || sep == '+' || sep == '.' ||
        sep == '\n' || sep == '\r') {
        cerr << "[ERROR] El separador " << mostrarSeparador(sep)
             << " no es valido (no puede ser un digito, signo, punto decimal ni salto de linea).\n";
        return SEPARADOR;
    }

    // --- validar archivos ---
    if (!validarArchivo(rutaA, "A") || !validarArchivo(rutaB, "B")) return ARCHIVO;

    // --- leer y validar contenido ---
    Matriz A, B, C;
    int codigo = leerMatriz(rutaA, "A", sep, A);
    if (codigo != OK) return codigo;
    codigo = leerMatriz(rutaB, "B", sep, B);
    if (codigo != OK) return codigo;

    cout << "Archivo A: " << rutaA << "\n";
    cout << "Archivo B: " << rutaB << "\n";
    cout << "Separador: " << mostrarSeparador(sep) << "\n\n";
    imprimirMatriz("Matriz A", A);
    imprimirMatriz("Matriz B", B);

    // --- validar que se puedan multiplicar ---
    if (A[0].size() != B.size()) {
        cerr << "\n[ERROR] No es posible multiplicar: A es " << A.size() << "x" << A[0].size()
             << " y B es " << B.size() << "x" << B[0].size() << ".\n";
        cerr << "        Las columnas de A (" << A[0].size() << ") deben ser iguales a las filas de B ("
             << B.size() << ").\n";
        return DIMENSIONES;
    }

    if (!multiplicar(A, B, C)) {
        cerr << "\n[ERROR] El resultado es demasiado grande (desborde numerico).\n";
        return DESBORDE;
    }

    cout << "\n";
    imprimirMatriz("Resultado A x B", C);
    return OK;
}
