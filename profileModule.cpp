#include "profileModule.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>
#include <iomanip>
#include "utils.h"

using namespace std;

// ---------------------------------------------------------------
// Utilidades internas de parseo / formato
// Formato de linea en archivo: nombre;op1,op2,op3
// Ejemplo:  ADMIN;0,1,2,3,4
//           GENERAL;0,1,3
// ---------------------------------------------------------------

static Perfil parsearLineaPerfil(const string& linea) {
    Perfil p;
    size_t pos = linea.find(';');

    if (pos == string::npos) {
        // Linea sin opciones (perfil sin permisos asignados aun)
        p.nombre = linea;
        return p;
    }

    p.nombre = linea.substr(0, pos);
    string opcionesStr = linea.substr(pos + 1);

    stringstream ss(opcionesStr);
    string numStr;
    while (getline(ss, numStr, ',')) {
        if (!numStr.empty()) {
            p.opciones.push_back(stoi(numStr));
        }
    }
    return p;
}

static string perfilALinea(const Perfil& p) {
    string linea = p.nombre + ";";
    for (size_t i = 0; i < p.opciones.size(); i++) {
        linea += to_string(p.opciones[i]);
        if (i != p.opciones.size() - 1) {
            linea += ",";
        }
    }
    return linea;
}

// Carga todos los perfiles del archivo hacia la lista en memoria
static void cargarPerfilesDesdeArchivo(vector<Perfil>& listaPerfiles, const string& perfilFile) {
    listaPerfiles.clear();
    ifstream archivo(perfilFile);
    string linea;

    if (archivo.is_open()) {
        while (getline(archivo, linea)) {
            if (!linea.empty()) {
                listaPerfiles.push_back(parsearLineaPerfil(linea));
            }
        }
        archivo.close();
    }
}

// Reescribe el archivo completo a partir de la lista en memoria
// (necesario tras una eliminacion, ya que no se puede "borrar" una linea
// de un archivo de texto sin reescribirlo)
static void reescribirArchivoPerfiles(const vector<Perfil>& listaPerfiles, const string& perfilFile) {
    ofstream archivo(perfilFile, ios::trunc);
    if (!archivo.is_open()) {
        cout << "Error: no se pudo escribir en " << perfilFile << endl;
        return;
    }
    for (const auto& p : listaPerfiles) {
        archivo << perfilALinea(p) << "\n";
    }
    archivo.close();
}

// Agrega un unico registro al final del archivo (usado al guardar un ingreso nuevo)
static void agregarPerfilAlArchivo(const Perfil& p, const string& perfilFile) {
    ofstream archivo(perfilFile, ios::app);
    if (!archivo.is_open()) {
        cout << "Error: no se pudo abrir " << perfilFile << " para escritura." << endl;
        return;
    }
    archivo << perfilALinea(p) << "\n";
    archivo.close();
}

// Busca el indice de un perfil por nombre dentro de la lista. -1 si no existe.
static int buscarIndicePorNombre(const vector<Perfil>& listaPerfiles, const string& nombre) {
    for (size_t i = 0; i < listaPerfiles.size(); i++) {
        if (listaPerfiles[i].nombre == nombre) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// ---------------------------------------------------------------
// Operaciones del modulo
// ---------------------------------------------------------------

// Ingresar perfil
void ingresarPerfil(vector<Perfil>& listaPerfiles, const string& perfilFile) {
    // Aseguramos tener los datos cargados en memoria antes de operar
    if (listaPerfiles.empty()) {
        cargarPerfilesDesdeArchivo(listaPerfiles, perfilFile);
    }

    Perfil nuevo;
    int opcion;

    cout << "\n--- Ingreso de perfiles ---\n";

    cout << "Nombre del perfil (ej: GENERAL, ADMIN): ";
    getline(cin, nuevo.nombre);

    if (buscarIndicePorNombre(listaPerfiles, nuevo.nombre) != -1) {
        cout << "Error: ya existe un perfil con ese nombre. Ingreso cancelado.\n";
        return;
    }

    cout << "Opciones de menu que puede manipular (numeros separados por coma, ej: 0,1,2,3,4): ";
    string opcionesStr;
    getline(cin, opcionesStr);

    stringstream ss(opcionesStr);
    string numStr;
    while (getline(ss, numStr, ',')) {
        // Se limpian espacios que pudiera haber dejado el usuario
        size_t inicio = numStr.find_first_not_of(" \t");
        size_t fin = numStr.find_last_not_of(" \t");
        if (inicio != string::npos) {
            numStr = numStr.substr(inicio, fin - inicio + 1);
            try {
                nuevo.opciones.push_back(stoi(numStr));
            } catch (...) {
                cout << "Aviso: se ignoro un valor no numerico ('" << numStr << "').\n";
            }
        }
    }

    cout << "\n1) guardar   2) cancelar\n";
    cout << "Opcion: ";
    cin >> opcion;
    limpiarBuffer();

    if (opcion == 1) {
        // Se agrega a la lista en memoria...
        listaPerfiles.push_back(nuevo);
        // ...y se agrega un registro al final del archivo
        agregarPerfilAlArchivo(nuevo, perfilFile);
        cout << "Perfil guardado correctamente.\n";
    } else {
        cout << "Ingreso cancelado.\n";
    }
}

// Listar perfiles: si ya hay datos en memoria los usa, si no, lee el archivo
void listarPerfiles(vector<Perfil>& listaPerfiles, const string& perfilFile) {
    if (listaPerfiles.empty()) {
        cargarPerfilesDesdeArchivo(listaPerfiles, perfilFile);
    }

    cout << "\n--- Lista de perfiles ---\n";
    if (listaPerfiles.empty()) {
        cout << "(no hay perfiles registrados)\n";
        return;
    }

    cout << left << setw(15) << "Nombre" << "Opciones\n";

    for (const auto& p : listaPerfiles) {
        cout << left << setw(15) << p.nombre;
        for (size_t i = 0; i < p.opciones.size(); i++) {
            cout << p.opciones[i];
            if (i != p.opciones.size() - 1) cout << ",";
        }
        cout << "\n";
    }
}

// Eliminar perfil por nombre (con alerta si es ADMIN)
void eliminarPerfil(vector<Perfil>& listaPerfiles, const string& perfilFile) {
    // Aseguramos tener los datos cargados en memoria antes de operar
    if (listaPerfiles.empty()) {
        cargarPerfilesDesdeArchivo(listaPerfiles, perfilFile);
    }

    string nombreBuscado;
    cout << "\n--- Eliminar perfil ---\n";
    cout << "Nombre del perfil a borrar: ";
    getline(cin, nombreBuscado);

    int indice = buscarIndicePorNombre(listaPerfiles, nombreBuscado);

    if (indice == -1) {
        cout << "No existe un perfil con ese nombre.\n";
        return;
    }

    // Alerta si el perfil a eliminar es ADMIN
    if (listaPerfiles[indice].nombre == "ADMIN") {
        cout << "\n*** ALERTA: esta a punto de eliminar el perfil ADMIN. ***\n";
        cout << "*** Esto puede dejar sin permisos administrativos al sistema. ***\n";
    }

    int opcion;
    cout << "\n1) confirmar        2) cancelar\n";
    cout << "Opcion: ";
    cin >> opcion;
    limpiarBuffer();

    if (opcion == 1) {
        listaPerfiles.erase(listaPerfiles.begin() + indice);
        reescribirArchivoPerfiles(listaPerfiles, perfilFile);
        cout << "Perfil eliminado correctamente.\n";
    } else {
        cout << "Eliminacion cancelada.\n";
    }
}

// Menu del modulo
void menuGestionPerfiles(vector<Perfil>& listaPerfiles, const string& perfilFile) {
    int opcion = -1;

    do {
        cout << "\n--- Modulo - Gestion de Perfiles ---\n";
        cout << "0) Salir\n";
        cout << "1) Ingresar Perfiles\n";
        cout << "2) Listar Perfiles\n";
        cout << "3) Eliminar Perfiles\n";
        cout << "Opcion: ";
        cin >> opcion;
        limpiarBuffer();

        switch (opcion) {
            case 0:
                cout << "Saliendo del modulo de perfiles...\n";
                break;
            case 1:
                ingresarPerfil(listaPerfiles, perfilFile);
                break;
            case 2:
                listarPerfiles(listaPerfiles, perfilFile);
                break;
            case 3:
                eliminarPerfil(listaPerfiles, perfilFile);
                break;
            default:
                cout << "Opcion invalida.\n";
        }
    } while (opcion != 0);
}