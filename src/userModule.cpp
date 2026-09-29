#include "userModule.h"
#include "utils.h"

#include <fstream>
#include <iomanip>
#include <sstream>

using namespace std;

// =====================================================================
// Lectura / escritura del struct completo
// =====================================================================

// Escribe el struct Usuario completo:  id;nombre;username;password;perfil
ostream& operator<<(ostream& os, const Usuario& u) {
    os << u.id << ';' << u.nombre << ';' << u.username << ';'
       << u.password << ';' << u.perfil;
    return os;
}

// Lee el struct Usuario completo desde el siguiente registro valido.
// Las lineas vacias se saltan; las mal formadas se informan y se saltan
// para no corromper la lista en memoria.
istream& operator>>(istream& is, Usuario& u) {
    string linea;
    while (getline(is, linea)) {
        linea = trim(linea);
        if (linea.empty()) continue;

        stringstream ss(linea);
        string idStr;
        Usuario leido;
        if (getline(ss, idStr, ';') && getline(ss, leido.nombre, ';') &&
            getline(ss, leido.username, ';') && getline(ss, leido.password, ';') &&
            getline(ss, leido.perfil)) {
            try {
                size_t pos = 0;
                leido.id = stoi(trim(idStr), &pos);
                if (pos == trim(idStr).size()) {
                    leido.nombre = trim(leido.nombre);
                    leido.username = trim(leido.username);
                    leido.password = trim(leido.password);
                    leido.perfil = trim(leido.perfil);
                    u = leido;
                    return is;
                }
            } catch (...) {}
        }
        cerr << "[ADVERTENCIA] Registro de usuario mal formado, se ignora: " << linea << "\n";
    }
    return is;   // fin de archivo: getline ya dejo el stream en estado de fallo
}

// =====================================================================
// Utilidades internas
// =====================================================================

bool cargarUsuarios(vector<Usuario>& listaUsuarios, const string& userFile) {
    listaUsuarios.clear();
    ifstream archivo(userFile);
    if (!archivo.is_open()) {
        cerr << "[ERROR] No se pudo abrir el archivo de usuarios: '" << userFile << "'\n";
        return false;
    }
    Usuario u;
    while (archivo >> u) {            // lee el struct completo
        listaUsuarios.push_back(u);
    }
    return true;
}

// Reescribe el archivo completo a partir de la lista en memoria
// (necesario tras una eliminacion)
static bool reescribirArchivoUsuarios(const vector<Usuario>& listaUsuarios, const string& userFile) {
    ofstream archivo(userFile, ios::trunc);
    if (!archivo.is_open()) {
        cout << "Error: no se pudo escribir en " << userFile << endl;
        return false;
    }
    for (const auto& u : listaUsuarios) {
        archivo << u << '\n';          // escribe el struct completo
    }
    return true;
}

// Agrega un unico registro al final del archivo
static bool agregarUsuarioAlArchivo(const Usuario& u, const string& userFile) {
    asegurarSaltoDeLineaFinal(userFile);
    ofstream archivo(userFile, ios::app);
    if (!archivo.is_open()) {
        cout << "Error: no se pudo abrir " << userFile << " para escritura." << endl;
        return false;
    }
    archivo << u << '\n';              // escribe el struct completo
    return true;
}

// Siguiente ID: 1001 si no hay usuarios, si no el mayor ID + 1
static int obtenerSiguienteId(const vector<Usuario>& listaUsuarios) {
    int maxId = 1000;
    for (const auto& u : listaUsuarios) {
        if (u.id > maxId) maxId = u.id;
    }
    return maxId + 1;
}

// Valida un campo de texto: no vacio y sin ';' (separador del archivo)
static string leerCampo(const string& mensaje, bool sinEspacios) {
    while (true) {
        string valor = leerLinea(mensaje);
        if (valor.empty()) {
            cout << "El campo no puede estar vacio.\n";
        } else if (valor.find(';') != string::npos) {
            cout << "El campo no puede contener el caracter ';'.\n";
        } else if (sinEspacios && valor.find(' ') != string::npos) {
            cout << "El campo no puede contener espacios.\n";
        } else {
            return valor;
        }
    }
}

// =====================================================================
// Operaciones del modulo
// =====================================================================

void ingresarUsuario(vector<Usuario>& listaUsuarios, const string& userFile,
                     const vector<string>& perfilesValidos) {
    if (listaUsuarios.empty()) cargarUsuarios(listaUsuarios, userFile);

    Usuario nuevo;
    cout << "\n--- Ingreso de usuarios ---\n";

    nuevo.id = obtenerSiguienteId(listaUsuarios);
    cout << "Id asignado: " << nuevo.id << "\n";

    nuevo.nombre = leerCampo("Nombre: ", false);

    while (true) {
        nuevo.username = leerCampo("Username: ", true);
        bool repetido = false;
        for (const auto& u : listaUsuarios) {
            if (u.username == nuevo.username) repetido = true;
        }
        if (!repetido) break;
        cout << "Ese username ya existe. Ingrese otro.\n";
    }

    nuevo.password = leerCampo("Password: ", true);

    string listaTxt;
    for (size_t i = 0; i < perfilesValidos.size(); i++) {
        listaTxt += perfilesValidos[i] + (i + 1 < perfilesValidos.size() ? "/" : "");
    }
    while (true) {
        nuevo.perfil = leerCampo("Perfil (" + listaTxt + "): ", true);
        bool valido = false;
        for (const auto& p : perfilesValidos) {
            if (p == nuevo.perfil) valido = true;
        }
        if (valido) break;
        cout << "Error: el perfil debe ser uno de los existentes (" << listaTxt << ").\n";
    }

    cout << "\n1) Guardar   2) Cancelar\n";
    int opcion = leerEntero("Opcion: ");

    if (opcion == 1) {
        if (agregarUsuarioAlArchivo(nuevo, userFile)) {
            listaUsuarios.push_back(nuevo);
            cout << "Usuario guardado correctamente.\n";
        }
    } else {
        cout << "Ingreso cancelado.\n";
    }
}

// Listar: si hay datos en memoria los usa, si no, lee el archivo
void listarUsuarios(vector<Usuario>& listaUsuarios, const string& userFile) {
    if (listaUsuarios.empty()) cargarUsuarios(listaUsuarios, userFile);

    cout << "\n--- Lista de usuarios ---\n";
    if (listaUsuarios.empty()) {
        cout << "(no hay usuarios registrados)\n";
        return;
    }

    cout << left << setw(8) << "Id" << setw(22) << "Nombre"
         << setw(15) << "Username" << setw(10) << "Perfil" << "\n";
    for (const auto& u : listaUsuarios) {
        cout << left << setw(8) << u.id << setw(22) << u.nombre
             << setw(15) << u.username << setw(10) << u.perfil << "\n";
    }
}

// Eliminar por ID (alerta si es ADMIN, no permite eliminar la propia cuenta)
void eliminarUsuario(vector<Usuario>& listaUsuarios, const string& userFile,
                     const string& usernameActual) {
    if (listaUsuarios.empty()) cargarUsuarios(listaUsuarios, userFile);

    cout << "\n--- Eliminar usuario ---\n";
    int idBuscado = leerEntero("ID del usuario a borrar: ");

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

    if (listaUsuarios[indice].username == usernameActual) {
        cout << "No puede eliminar el usuario con el que inicio sesion.\n";
        return;
    }

    if (listaUsuarios[indice].perfil == "ADMIN") {
        cout << "\n*** ALERTA: el usuario seleccionado tiene perfil ADMIN. ***\n";
        cout << "*** Eliminarlo puede dejar al sistema sin administradores. ***\n";
    }

    cout << "\n1) Confirmar   2) Cancelar\n";
    int opcion = leerEntero("Opcion: ");

    if (opcion == 1) {
        vector<Usuario> copia = listaUsuarios;
        copia.erase(copia.begin() + indice);
        if (reescribirArchivoUsuarios(copia, userFile)) {
            listaUsuarios = copia;
            cout << "Usuario eliminado correctamente.\n";
        }
    } else {
        cout << "Eliminacion cancelada.\n";
    }
}

void menuGestionUsuarios(vector<Usuario>& listaUsuarios, const string& userFile,
                         const vector<string>& perfilesValidos, const string& usernameActual) {
    int opcion = -1;
    do {
        cout << "\n--- Modulo - Gestion de Usuarios ---\n";
        cout << "0) Volver\n";
        cout << "1) Ingresar Usuarios\n";
        cout << "2) Listar Usuarios\n";
        cout << "3) Eliminar Usuarios\n";
        opcion = leerEntero("Opcion: ");

        switch (opcion) {
            case 0: break;
            case 1: ingresarUsuario(listaUsuarios, userFile, perfilesValidos); break;
            case 2: listarUsuarios(listaUsuarios, userFile); break;
            case 3: eliminarUsuario(listaUsuarios, userFile, usernameActual); break;
            default: cout << "Opcion invalida.\n";
        }
    } while (opcion != 0);
}
