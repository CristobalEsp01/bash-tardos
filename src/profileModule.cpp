#include "profileModule.h"
#include "utils.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>

using namespace std;

bool Perfil::tienePermiso(int opcion) const {
    return find(opciones.begin(), opciones.end(), opcion) != opciones.end();
}

// =====================================================================
// Lectura / escritura del struct completo
// =====================================================================

// Convierte "0, 1,2" en {0,1,2}. Ignora (y avisa) valores fuera de 0..7
static vector<int> parsearOpciones(const string& texto, bool avisar) {
    vector<int> opciones;
    stringstream ss(texto);
    string numStr;
    while (getline(ss, numStr, ',')) {
        numStr = trim(numStr);
        if (numStr.empty()) continue;
        try {
            size_t pos = 0;
            int n = stoi(numStr, &pos);
            if (pos != numStr.size() || n < 0 || n > OPCION_MAXIMA_MENU) throw 1;
            if (find(opciones.begin(), opciones.end(), n) == opciones.end()) {
                opciones.push_back(n);
            }
        } catch (...) {
            if (avisar) {
                cout << "Aviso: se ignoro el valor '" << numStr << "' (debe ser un numero entre 0 y "
                     << OPCION_MAXIMA_MENU << ").\n";
            }
        }
    }
    sort(opciones.begin(), opciones.end());
    return opciones;
}

// Escribe el struct Perfil completo:  NOMBRE;op1,op2,op3
ostream& operator<<(ostream& os, const Perfil& p) {
    os << p.nombre << ';';
    for (size_t i = 0; i < p.opciones.size(); i++) {
        if (i > 0) os << ',';
        os << p.opciones[i];
    }
    return os;
}

// Lee el struct Perfil completo desde el siguiente registro no vacio
istream& operator>>(istream& is, Perfil& p) {
    string linea;
    while (getline(is, linea)) {
        linea = trim(linea);
        if (linea.empty()) continue;

        Perfil leido;
        size_t pos = linea.find(';');
        leido.nombre = trim(linea.substr(0, pos));
        if (pos != string::npos) {
            leido.opciones = parsearOpciones(linea.substr(pos + 1), false);
        }
        if (leido.nombre.empty()) {
            cerr << "[ADVERTENCIA] Registro de perfil mal formado, se ignora: " << linea << "\n";
            continue;
        }
        p = leido;
        return is;
    }
    return is;
}

// =====================================================================
// Utilidades internas
// =====================================================================

bool cargarPerfiles(vector<Perfil>& listaPerfiles, const string& perfilFile) {
    listaPerfiles.clear();
    ifstream archivo(perfilFile);
    if (!archivo.is_open()) {
        cerr << "[ERROR] No se pudo abrir el archivo de perfiles: '" << perfilFile << "'\n";
        return false;
    }
    Perfil p;
    while (archivo >> p) {             // lee el struct completo
        listaPerfiles.push_back(p);
    }
    return true;
}

const Perfil* buscarPerfil(const vector<Perfil>& listaPerfiles, const string& nombre) {
    for (const auto& p : listaPerfiles) {
        if (p.nombre == nombre) return &p;
    }
    return nullptr;
}

static bool reescribirArchivoPerfiles(const vector<Perfil>& listaPerfiles, const string& perfilFile) {
    ofstream archivo(perfilFile, ios::trunc);
    if (!archivo.is_open()) {
        cout << "Error: no se pudo escribir en " << perfilFile << endl;
        return false;
    }
    for (const auto& p : listaPerfiles) {
        archivo << p << '\n';          // escribe el struct completo
    }
    return true;
}

static bool agregarPerfilAlArchivo(const Perfil& p, const string& perfilFile) {
    asegurarSaltoDeLineaFinal(perfilFile);
    ofstream archivo(perfilFile, ios::app);
    if (!archivo.is_open()) {
        cout << "Error: no se pudo abrir " << perfilFile << " para escritura." << endl;
        return false;
    }
    archivo << p << '\n';              // escribe el struct completo
    return true;
}

// =====================================================================
// Operaciones del modulo
// =====================================================================

void ingresarPerfil(vector<Perfil>& listaPerfiles, const string& perfilFile) {
    if (listaPerfiles.empty()) cargarPerfiles(listaPerfiles, perfilFile);

    Perfil nuevo;
    cout << "\n--- Ingreso de perfiles ---\n";

    while (true) {
        nuevo.nombre = leerLinea("Nombre del perfil (ej: GENERAL, ADMIN): ");
        transform(nuevo.nombre.begin(), nuevo.nombre.end(), nuevo.nombre.begin(), ::toupper);
        if (nuevo.nombre.empty() || nuevo.nombre.find_first_of("; ,") != string::npos) {
            cout << "El nombre no puede estar vacio ni contener espacios, ';' o ','.\n";
            continue;
        }
        if (buscarPerfil(listaPerfiles, nuevo.nombre)) {
            cout << "Error: ya existe un perfil con ese nombre. Ingreso cancelado.\n";
            return;
        }
        break;
    }

    cout << "Opciones del menu principal que puede usar (0 a " << OPCION_MAXIMA_MENU
         << ", separadas por coma, ej: 0,2,3,4)\n";
    nuevo.opciones = parsearOpciones(leerLinea("Opciones: "), true);
    if (!nuevo.tienePermiso(0)) {
        nuevo.opciones.insert(nuevo.opciones.begin(), 0);   // Salir siempre permitido
        cout << "Se agrego la opcion 0 (Salir), que es obligatoria para todo perfil.\n";
    }

    cout << "\n1) Guardar   2) Cancelar\n";
    int opcion = leerEntero("Opcion: ");

    if (opcion == 1) {
        if (agregarPerfilAlArchivo(nuevo, perfilFile)) {
            listaPerfiles.push_back(nuevo);
            cout << "Perfil guardado correctamente.\n";
        }
    } else {
        cout << "Ingreso cancelado.\n";
    }
}

void listarPerfiles(vector<Perfil>& listaPerfiles, const string& perfilFile) {
    if (listaPerfiles.empty()) cargarPerfiles(listaPerfiles, perfilFile);

    cout << "\n--- Lista de perfiles ---\n";
    if (listaPerfiles.empty()) {
        cout << "(no hay perfiles registrados)\n";
        return;
    }

    cout << left << setw(15) << "Nombre" << "Opciones permitidas\n";
    for (const auto& p : listaPerfiles) {
        cout << left << setw(15) << p.nombre;
        for (size_t i = 0; i < p.opciones.size(); i++) {
            cout << p.opciones[i] << (i + 1 < p.opciones.size() ? "," : "");
        }
        cout << "\n";
    }
}

void eliminarPerfil(vector<Perfil>& listaPerfiles, const string& perfilFile) {
    if (listaPerfiles.empty()) cargarPerfiles(listaPerfiles, perfilFile);

    cout << "\n--- Eliminar perfil ---\n";
    string nombreBuscado = leerLinea("Nombre del perfil a borrar: ");
    transform(nombreBuscado.begin(), nombreBuscado.end(), nombreBuscado.begin(), ::toupper);

    int indice = -1;
    for (size_t i = 0; i < listaPerfiles.size(); i++) {
        if (listaPerfiles[i].nombre == nombreBuscado) indice = static_cast<int>(i);
    }
    if (indice == -1) {
        cout << "No existe un perfil con ese nombre.\n";
        return;
    }

    if (listaPerfiles[indice].nombre == "ADMIN") {
        cout << "\n*** ALERTA: esta a punto de eliminar el perfil ADMIN. ***\n";
        cout << "*** Esto puede dejar sin permisos administrativos al sistema. ***\n";
    }

    cout << "\n1) Confirmar   2) Cancelar\n";
    int opcion = leerEntero("Opcion: ");

    if (opcion == 1) {
        vector<Perfil> copia = listaPerfiles;
        copia.erase(copia.begin() + indice);
        if (reescribirArchivoPerfiles(copia, perfilFile)) {
            listaPerfiles = copia;
            cout << "Perfil eliminado correctamente.\n";
        }
    } else {
        cout << "Eliminacion cancelada.\n";
    }
}

void menuGestionPerfiles(vector<Perfil>& listaPerfiles, const string& perfilFile) {
    int opcion = -1;
    do {
        cout << "\n--- Modulo - Gestion de Perfiles ---\n";
        cout << "0) Volver\n";
        cout << "1) Ingresar Perfiles\n";
        cout << "2) Listar Perfiles\n";
        cout << "3) Eliminar Perfiles\n";
        opcion = leerEntero("Opcion: ");

        switch (opcion) {
            case 0: break;
            case 1: ingresarPerfil(listaPerfiles, perfilFile); break;
            case 2: listarPerfiles(listaPerfiles, perfilFile); break;
            case 3: eliminarPerfil(listaPerfiles, perfilFile); break;
            default: cout << "Opcion invalida.\n";
        }
    } while (opcion != 0);
}
