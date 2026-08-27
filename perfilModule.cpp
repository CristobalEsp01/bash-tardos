#include "perfilModule.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>
#include <iomanip>
#include "utils.h"

using namespace std;

// Utilidad interna: separa una línea "perfil;permiso1,permiso2,...,permisoN"
static Perfil parsearLinea(const string& linea) {
    Perfil p;
    stringstream ss(linea);

    getline(ss, p.nombre, ';');
    string permisoStr;
    while (ss, permisoStr, ',') {
        if (!permisoStr.empty()) p.permisos.push_back(stoi(permisoStr));
    }

    return p;
}

// Convierte un Perfil a la línea que se guarda en el archivo
static string perfilALinea(const Perfil& p) {
    string linea = p.nombre + ";";
    for (size_t i = 0; i < p.permisos.size(); i++) {
        linea += p.permisos[i];
        if (i < p.permisos.size()) linea += ",";
    }
}

// Carga todos los perfiles del archivo hacia la lista en memoria
static void cargarPerfilesDesdeArchivo(vector<Perfil>& listaPerfiles, const string& perfilFile) {
    listaPerfiles.clear();
    ifstream archivo(perfilFile);
    string linea;

    if (archivo.is_open()) {
        while (getline(archivo, linea)) {
            if (!linea.empty()) {
                listaPerfiles.push_back(parsearLinea(linea));
            }
        }
        archivo.close();
    }

    else cerr << "Error: no se pudo cargar el archivo " << perfilFile << endl;
}

// Reescribe el archivo completo a partir de la lista en memoria
// (necesario tras una eliminación, ya que no se puede "borrar" una línea
// de un archivo de texto sin reescribirlo)
static void reescribirArchivoPerfiles(const vector<Perfil>& listaPerfiles, const string& perfilFile) {
    ofstream archivo(perfilFile, ios::trunc); // TODO: no falta una flag para escribir sobre el archivo?
    if (!archivo.is_open()) { // TODO: En userModule.cpp igualar mensaje de error y cambiar a cerr
        cerr << "Error: no se pudo abrir el archivo " << perfilFile << " para escritura." << endl;
        return;
    }
    for (const auto& p : listaPerfiles) {
        archivo << perfilALinea(p) << "\n";
    }
    archivo.close();
}

// Agregar permiso a perfil 
void ingresarPermisoPerfil(vector<Perfil>& listaPerfiles, const string& perfilFile) {

    // asegurarse de que esté en memoria
    if (listaPerfiles.empty()) {
        cargarPerfilesDesdeArchivo(listaPerfiles, perfilFile);
    }

    cout << "\n--- Ingreso de permiso a perfil ---\n";

    string perfilBuscado;
    cout << "nombre del perfil: ";
    cin >> perfilBuscado;
    limpiarBuffer();

    // Buscamos el perfil
    int indicePerfil = -1;
    for (size_t i = 0; i < listaPerfiles.size(); i++) {
        if (listaPerfiles[i].nombre == perfilBuscado) {
            indicePerfil = static_cast<int>(i);
            break;
        }
    }

    if (indicePerfil == -1) {
        cerr << "No existe un perfil con ese nombre.\n";
        return;
    }

    int permisoIngresar;
    cout << "ID permiso a ingresar: ";
    cin >> permisoIngresar;
    limpiarBuffer();

    // Buscamos el permiso
    int indicePermiso = -1;
    for (size_t i = 0; i < listaPerfiles[indicePerfil].permisos.size(); i++) {
        if (listaPerfiles[indicePerfil].permisos[i] == permisoIngresar) {
            indicePermiso = static_cast<int>(i);
            break;
        }
    }

    if (indicePermiso != -1) {
        cerr << "El perfil ya tiene el permiso con ese ID.\n";
        return;
    }

    int opcion;
    cout << "\n1) guardar   2) cancelar\n";
    cout << "Opcion: ";
    cin >> opcion;
    limpiarBuffer();

    if (opcion == 1) {
        // Se agrega a la lista en memoria...
        listaPerfiles[indicePerfil].permisos.push_back(permisoIngresar);
        // ...y se sobreescribe el registro en archivo
        reescribirArchivoPerfiles(listaPerfiles, perfilFile);
        cout << "permiso del perfil guardado correctamente.\n";
    } else {
        cout << "Ingreso cancelado.\n";
    }
}

// Listar perfiles: si ya hay datos en memoria los usa, si no, lee el archivo y carga en memoria  
// TODO: cambiar comentario en otro archivo para que sea igual a este
void listarPerfiles(vector<Perfil>& listaPerfiles, const string& perfilFile) {
    if (listaPerfiles.empty()) {
        cargarPerfilesDesdeArchivo(listaPerfiles, perfilFile);
    }

    cout << "\n--- Lista de perfiles ---\n";
    if (listaPerfiles.empty()) {
        cout << "(no hay perfiles registrados)\n";
        return;
    }

    // Cabecera con ancho fijo por columna
    cout << left << setw(8) << "Nombre" 
         << setw(10) << "permisos" << "\n";

    // Filas alineadas con el mismo ancho
    for (const auto& p : listaPerfiles) {
        cout << left << setw(8) << p.nombre << setw(10);
        for (const auto& permiso : p.permisos) {
            cout << permiso << " ";
        }
        cout << endl;
    }
}

// Eliminar permiso de un perfil (con alerta si es ADMIN)
void eliminarPermisoPerfil(vector<Perfil>& listaPerfiles, const string& perfilFile) {
    // Aseguramos tener los datos cargados en memoria antes de operar
    if (listaPerfiles.empty()) {
        cargarPerfilesDesdeArchivo(listaPerfiles, perfilFile);
    }

    string perfilBuscado;
    cout << "\n--- Eliminar permiso de un perfil ---\n";
    cout << "nombre del perfil: ";
    cin >> perfilBuscado;
    limpiarBuffer();

    // Buscamos el perfil
    int indicePerfil = -1;
    for (size_t i = 0; i < listaPerfiles.size(); i++) {
        if (listaPerfiles[i].nombre == perfilBuscado) {
            indicePerfil = static_cast<int>(i);
            break;
        }
    }

    if (indicePerfil == -1) {
        cerr << "No existe un perfil con ese nombre.\n";
        return;
    }

    // Alerta si el perfil es ADMIN
    if (perfilBuscado == "ADMIN") {
        cerr << "\n*** ALERTA: el perfil seleccionado es ADMIN. ***\n";
        cerr << "*** Eliminar un permiso puede generar problemas. ***\n";
    }

    int permisoBuscado;
    cout << "ID permiso a borrar: ";
    cin >> permisoBuscado;
    limpiarBuffer();

    // Buscamos el permiso
    int indicePermiso = -1;
    for (size_t i = 0; i < listaPerfiles[indicePerfil].permisos.size(); i++) {
        if (listaPerfiles[indicePerfil].permisos[i] == permisoBuscado) {
            indicePermiso = static_cast<int>(i);
            break;
        }
    }

    if (indicePermiso == -1) {
        cerr << "No existe un permiso con ese ID.\n";
        return;
    }

    int opcion;
    cout << "\n1) confirmar        2) cancelar\n";
    cout << "Opcion: ";
    cin >> opcion;
    limpiarBuffer();

    if (opcion == 1) {
        listaPerfiles[indicePerfil].permisos.erase(listaPerfiles[indicePerfil].permisos.begin() + indicePermiso);
        reescribirArchivoPerfiles(listaPerfiles, perfilFile);
        cout << "Permiso eliminado correctamente.\n";
    } else {
        cout << "Eliminacion cancelada.\n";
    }
}

// Menú del módulo
void menuGestionPerfiles(vector<Perfil>& listaPerfiles, const string& perfilFile) {
    int opcion = -1;

    do {
        cout << "\n--- Modulo - Gestion de Perfiles ---\n";
        cout << "0) Salir\n";
        cout << "1) Ingresar Permiso A Perfil\n";
        cout << "2) Listar Perfiles\n";
        cout << "3) Eliminar Permiso A Perfil\n";
        cout << "Opcion: ";
        cin >> opcion;
        limpiarBuffer();

        switch (opcion) {
            case 0:
                cout << "Saliendo del modulo de perfiles...\n";
                break;
            case 1:
                ingresarPermisoPerfil(listaPerfiles, perfilFile);
                break;
            case 2:
                listarPerfiles(listaPerfiles, perfilFile);
                break;
            case 3:
                eliminarPermisoPerfil(listaPerfiles, perfilFile);
                break;
            default:
                cout << "Opcion invalida.\n";
        }
    } while (opcion != 0);
}
