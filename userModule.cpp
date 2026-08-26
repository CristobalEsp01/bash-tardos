#include "userModule.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>
#include <iomanip>

using namespace std;

// ---------------------------------------------------------------------
// Utilidad interna: separa una línea "id;nombre;username;password;perfil"
// ---------------------------------------------------------------------
static Usuario parsearLinea(const string& linea) {
    Usuario u;
    stringstream ss(linea);
    string idStr;

    getline(ss, idStr, ';');
    getline(ss, u.nombre, ';');
    getline(ss, u.username, ';');
    getline(ss, u.password, ';');
    getline(ss, u.perfil, ';');

    u.id = idStr.empty() ? 0 : stoi(idStr);
    return u;
}

// Convierte un Usuario a la línea que se guarda en el archivo
static string usuarioALinea(const Usuario& u) {
    return to_string(u.id) + ";" + u.nombre + ";" + u.username + ";" +
           u.password + ";" + u.perfil;
}

// ---------------------------------------------------------------------
// Carga todos los usuarios del archivo hacia la lista en memoria
// ---------------------------------------------------------------------
static void cargarUsuariosDesdeArchivo(vector<Usuario>& listaUsuarios, const string& userFile) {
    listaUsuarios.clear();
    ifstream archivo(userFile);
    string linea;

    if (archivo.is_open()) {
        while (getline(archivo, linea)) {
            if (!linea.empty()) {
                listaUsuarios.push_back(parsearLinea(linea));
            }
        }
        archivo.close();
    }
}

// Reescribe el archivo completo a partir de la lista en memoria
// (necesario tras una eliminación, ya que no se puede "borrar" una línea
// de un archivo de texto sin reescribirlo)
static void reescribirArchivoUsuarios(const vector<Usuario>& listaUsuarios, const string& userFile) {
    ofstream archivo(userFile, ios::trunc);
    if (!archivo.is_open()) {
        cout << "Error: no se pudo escribir en " << userFile << endl;
        return;
    }
    for (const auto& u : listaUsuarios) {
        archivo << usuarioALinea(u) << "\n";
    }
    archivo.close();
}

// Agrega un único registro al final del archivo (usado al guardar un ingreso nuevo)
static void agregarUsuarioAlArchivo(const Usuario& u, const string& userFile) {
    ofstream archivo(userFile, ios::app);
    if (!archivo.is_open()) {
        cout << "Error: no se pudo abrir " << userFile << " para escritura." << endl;
        return;
    }
    archivo << usuarioALinea(u) << "\n";
    archivo.close();
}

// Limpia el buffer de entrada tras usar cin >> algo
static void limpiarBuffer() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}


// Calcula el siguiente ID disponible.
// Regla: arranca en 1001 si no hay usuarios, y si ya hay, es el
// mayor ID actual + 1 
static int obtenerSiguienteId(vector<Usuario>& listaUsuarios, const string& userFile) {
    // Si la lista en memoria está vacía, intentamos cargarla del archivo
    // para no perder de vista los IDs ya usados anteriormente.
    if (listaUsuarios.empty()) {
        cargarUsuariosDesdeArchivo(listaUsuarios, userFile);
    }

    if (listaUsuarios.empty()) {
        return 1001;
    }

    int maxId = listaUsuarios[0].id;
    for (const auto& u : listaUsuarios) {
        if (u.id > maxId) {
            maxId = u.id;
        }
    }
    return maxId + 1;
}

// ---------------------------------------------------------------------
// Ingresar usuario (pantalla "admin-ingresar" del diagrama)
// ---------------------------------------------------------------------
void ingresarUsuario(vector<Usuario>& listaUsuarios, const string& userFile) {
    Usuario nuevo;
    int opcion;

    cout << "\n--- Ingreso de usuarios ---\n";

    // El ID se genera automáticamente (último id de la lista + 1)
    nuevo.id = obtenerSiguienteId(listaUsuarios, userFile);
    cout << "Id asignado: " << nuevo.id << "\n";

    cout << "Nombre: ";
    getline(cin, nuevo.nombre);

    cout << "username: ";
    getline(cin, nuevo.username);

    cout << "password: ";
    getline(cin, nuevo.password);

    // Validación del perfil: solo permite GENERAL o ADMIN
    do {
        cout << "perfil (GENERAL/ADMIN): ";
        getline(cin, nuevo.perfil);

        if (nuevo.perfil != "GENERAL" && nuevo.perfil != "ADMIN") {
            cout << "Error: El perfil debe ser obligatoriamente 'GENERAL' o 'ADMIN'. Intente nuevamente.\n";
        }
    } while (nuevo.perfil != "GENERAL" && nuevo.perfil != "ADMIN");

    cout << "\n1) guardar   2) cancelar\n";
    cout << "Opcion: ";
    cin >> opcion;
    limpiarBuffer();

    if (opcion == 1) {
        // Se agrega a la lista en memoria...
        listaUsuarios.push_back(nuevo);
        // ...y se agrega un registro al final del archivo
        agregarUsuarioAlArchivo(nuevo, userFile);
        cout << "Usuario guardado correctamente.\n";
    } else {
        cout << "Ingreso cancelado.\n";
    }
}

// ---------------------------------------------------------------------
// Listar usuarios: si ya hay datos en memoria los usa, si no, lee el archivo
// ---------------------------------------------------------------------
void listarUsuarios(vector<Usuario>& listaUsuarios, const string& userFile) {
    if (listaUsuarios.empty()) {
        cargarUsuariosDesdeArchivo(listaUsuarios, userFile);
    }

    cout << "\n--- Lista de usuarios ---\n";
    if (listaUsuarios.empty()) {
        cout << "(no hay usuarios registrados)\n";
        return;
    }

    // Cabecera con ancho fijo por columna
    cout << left << setw(8) << "Id" 
         << setw(20) << "Nombre" 
         << setw(10) << "Perfil" << "\n";

    // Filas alineadas con el mismo ancho
    for (const auto& u : listaUsuarios) {
        cout << left << setw(8) << u.id 
             << setw(20) << u.nombre 
             << setw(10) << u.perfil << "\n";
    }
}

// ---------------------------------------------------------------------
// Eliminar usuario por ID (con alerta si es ADMIN)
// ---------------------------------------------------------------------
void eliminarUsuario(vector<Usuario>& listaUsuarios, const string& userFile) {
    // Aseguramos tener los datos cargados en memoria antes de operar
    if (listaUsuarios.empty()) {
        cargarUsuariosDesdeArchivo(listaUsuarios, userFile);
    }

    int idBuscado;
    cout << "\n--- Eliminar usuario ---\n";
    cout << "ID usuario a borrar: ";
    cin >> idBuscado;
    limpiarBuffer();

    // Buscamos el usuario
    int indice = -1;
    for (size_t i = 0; i < listaUsuarios.size(); i++) {
        if (listaUsuarios[i].id == idBuscado) {
            indice = static_cast<int>(i);
            break;
        }
    }

    if (indice == -1) {
        cout << "No existe un usuario con ese ID.\n";
        return;
    }

    // Alerta si el perfil es ADMIN
    if (listaUsuarios[indice].perfil == "ADMIN") {
        cout << "\n*** ALERTA: el usuario seleccionado tiene perfil ADMIN. ***\n";
        cout << "*** Eliminarlo puede dejar al sistema sin administradores. ***\n";
    }

    int opcion;
    cout << "\n1) confirmar        2) cancelar\n";
    cout << "Opcion: ";
    cin >> opcion;
    limpiarBuffer();

    if (opcion == 1) {
        listaUsuarios.erase(listaUsuarios.begin() + indice);
        reescribirArchivoUsuarios(listaUsuarios, userFile);
        cout << "Usuario eliminado correctamente.\n";
    } else {
        cout << "Eliminacion cancelada.\n";
    }
}

// ---------------------------------------------------------------------
// Menú del módulo (0 Salir, 1 Ingresar, 2 Listar, 3 Eliminar)
// ---------------------------------------------------------------------
void menuGestionUsuarios(vector<Usuario>& listaUsuarios, const string& userFile) {
    int opcion = -1;

    do {
        cout << "\n--- Modulo - Gestion de Usuarios ---\n";
        cout << "0) Salir\n";
        cout << "1) Ingresar Usuarios\n";
        cout << "2) Listar Usuarios\n";
        cout << "3) Eliminar Usuarios\n";
        cout << "Opcion: ";
        cin >> opcion;
        limpiarBuffer();

        switch (opcion) {
            case 0:
                cout << "Saliendo del modulo de usuarios...\n";
                break;
            case 1:
                ingresarUsuario(listaUsuarios, userFile);
                break;
            case 2:
                listarUsuarios(listaUsuarios, userFile);
                break;
            case 3:
                eliminarUsuario(listaUsuarios, userFile);
                break;
            default:
                cout << "Opcion invalida.\n";
        }
    } while (opcion != 0);
}