#include "userModule.h"
#include "utils.h"

#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>

using namespace std;

// =====================================================================
// Lectura / escritura del struct COMPLETO en archivo binario
// =====================================================================

// Lee todos los registros: cada read() trae un struct Usuario completo
// (sizeof(Usuario) bytes) directamente a la memoria del struct.
bool cargarUsuarios(vector<Usuario>& listaUsuarios, const string& userFile) {
    listaUsuarios.clear();
    ifstream archivo(userFile, ios::binary);
    if (!archivo.is_open()) {
        cerr << "[ERROR] No se pudo abrir el archivo de usuarios: '" << userFile << "'\n";
        return false;
    }

    // Si el tamano no es multiplo de sizeof(Usuario), el archivo no fue
    // escrito por este sistema (o esta danado): no se lee para no cargar basura.
    archivo.seekg(0, ios::end);
    streamoff tamano = archivo.tellg();
    archivo.seekg(0, ios::beg);
    if (tamano % static_cast<streamoff>(sizeof(Usuario)) != 0) {
        cerr << "[ERROR] El archivo de usuarios '" << userFile << "' esta danado o no tiene el formato "
             << "binario esperado (" << tamano << " bytes no es multiplo de " << sizeof(Usuario) << ").\n";
        return false;
    }

    Usuario u;
    while (archivo.read(reinterpret_cast<char*>(&u), sizeof(Usuario))) {
        // por seguridad, se garantiza que cada texto termine en '\0'
        u.nombre[LARGO_NOMBRE - 1] = '\0';
        u.username[LARGO_USERNAME - 1] = '\0';
        u.password[LARGO_PASSWORD - 1] = '\0';
        u.perfil[LARGO_PERFIL - 1] = '\0';
        listaUsuarios.push_back(u);
    }
    return true;
}

// Reescribe el archivo completo desde la lista en memoria (tras eliminar)
static bool reescribirArchivoUsuarios(const vector<Usuario>& listaUsuarios, const string& userFile) {
    ofstream archivo(userFile, ios::binary | ios::trunc);
    if (!archivo.is_open()) {
        cout << "[ERROR] No se pudo escribir en " << userFile << "\n";
        return false;
    }
    for (const auto& u : listaUsuarios) {
        archivo.write(reinterpret_cast<const char*>(&u), sizeof(Usuario));   // struct completo
    }
    return static_cast<bool>(archivo);
}

// Agrega un struct completo al final del archivo
static bool agregarUsuarioAlArchivo(const Usuario& u, const string& userFile) {
    ofstream archivo(userFile, ios::binary | ios::app);
    if (!archivo.is_open()) {
        cout << "[ERROR] No se pudo abrir " << userFile << " para escritura.\n";
        return false;
    }
    archivo.write(reinterpret_cast<const char*>(&u), sizeof(Usuario));       // struct completo
    return static_cast<bool>(archivo);
}

// =====================================================================
// Utilidades internas
// =====================================================================

// Siguiente ID: 1001 si no hay usuarios, si no el mayor ID + 1
static int obtenerSiguienteId(const vector<Usuario>& listaUsuarios) {
    int maxId = 1000;
    for (const auto& u : listaUsuarios) {
        if (u.id > maxId) maxId = u.id;
    }
    return maxId + 1;
}

// Pide un campo de texto: no vacio, con largo maximo (cabe en el char[])
// y opcionalmente sin espacios.
static string leerCampo(const string& mensaje, int largoArreglo, bool sinEspacios) {
    size_t maximo = static_cast<size_t>(largoArreglo - 1);
    while (true) {
        string valor = leerLinea(mensaje);
        if (valor.empty()) {
            cout << "El campo no puede estar vacio.\n";
        } else if (valor.size() > maximo) {
            cout << "El campo admite como maximo " << maximo << " caracteres.\n";
        } else if (sinEspacios && valor.find(' ') != string::npos) {
            cout << "El campo no puede contener espacios.\n";
        } else {
            return valor;
        }
    }
}

static void imprimirTablaUsuarios(const vector<Usuario>& listaUsuarios) {
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

static void encabezado(const string& titulo) {
    limpiarPantalla();
    cout << "======================================\n";
    cout << "  " << titulo << "\n";
    cout << "======================================\n";
}

// =====================================================================
// Operaciones del modulo
// =====================================================================

void ingresarUsuario(vector<Usuario>& listaUsuarios, const string& userFile,
                     const vector<string>& perfilesValidos) {
    if (listaUsuarios.empty()) cargarUsuarios(listaUsuarios, userFile);
    encabezado("INGRESO DE USUARIOS");

    Usuario nuevo;
    // Se pone en cero TODO el struct, incluidos los bytes de relleno
    // (padding) que agrega el compilador, para no escribir basura de
    // memoria en el archivo binario.
    memset(static_cast<void*>(&nuevo), 0, sizeof(Usuario));
    nuevo.id = obtenerSiguienteId(listaUsuarios);
    cout << "Id asignado: " << nuevo.id << "\n";

    copiarTexto(nuevo.nombre, LARGO_NOMBRE, leerCampo("Nombre: ", LARGO_NOMBRE, false));

    while (true) {
        string username = leerCampo("Username: ", LARGO_USERNAME, true);
        bool repetido = false;
        for (const auto& u : listaUsuarios) {
            if (username == u.username) repetido = true;
        }
        if (!repetido) {
            copiarTexto(nuevo.username, LARGO_USERNAME, username);
            break;
        }
        cout << "Ese username ya existe. Ingrese otro.\n";
    }

    copiarTexto(nuevo.password, LARGO_PASSWORD, leerCampo("Password: ", LARGO_PASSWORD, true));

    string listaTxt;
    for (size_t i = 0; i < perfilesValidos.size(); i++) {
        listaTxt += perfilesValidos[i] + (i + 1 < perfilesValidos.size() ? "/" : "");
    }
    while (true) {
        string perfil = leerCampo("Perfil (" + listaTxt + "): ", LARGO_PERFIL, true);
        bool valido = false;
        for (const auto& p : perfilesValidos) {
            if (p == perfil) valido = true;
        }
        if (valido) {
            copiarTexto(nuevo.perfil, LARGO_PERFIL, perfil);
            break;
        }
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
    pausar("\nPresione ENTER para continuar...");
}

// Listar: si hay datos en memoria los usa, si no, lee el archivo
void listarUsuarios(vector<Usuario>& listaUsuarios, const string& userFile) {
    if (listaUsuarios.empty()) cargarUsuarios(listaUsuarios, userFile);
    encabezado("LISTA DE USUARIOS");
    imprimirTablaUsuarios(listaUsuarios);
    pausar("\nPresione ENTER para continuar...");
}

// Eliminar por ID (alerta si es ADMIN, no permite eliminar la propia cuenta)
void eliminarUsuario(vector<Usuario>& listaUsuarios, const string& userFile,
                     const string& usernameActual) {
    if (listaUsuarios.empty()) cargarUsuarios(listaUsuarios, userFile);
    encabezado("ELIMINAR USUARIO");
    imprimirTablaUsuarios(listaUsuarios);
    cout << "\n";

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
    } else if (usernameActual == listaUsuarios[indice].username) {
        cout << "No puede eliminar el usuario con el que inicio sesion.\n";
    } else {
        if (string(listaUsuarios[indice].perfil) == "ADMIN") {
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
    pausar("\nPresione ENTER para continuar...");
}

void menuGestionUsuarios(vector<Usuario>& listaUsuarios, const string& userFile,
                         const vector<string>& perfilesValidos, const string& usernameActual) {
    int opcion = -1;
    do {
        encabezado("GESTION DE USUARIOS");
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
            default:
                cout << "Opcion invalida.\n";
                pausar("Presione ENTER para continuar...");
        }
    } while (opcion != 0);
}
