#ifndef USER_MODULE_H
#define USER_MODULE_H

#include <string>
#include <type_traits>
#include <vector>

using namespace std;

// Largo maximo de cada campo de texto (incluye el '\0' final)
const int LARGO_NOMBRE   = 50;
const int LARGO_USERNAME = 20;
const int LARGO_PASSWORD = 20;
const int LARGO_PERFIL   = 20;

// ---------------------------------------------------------------------
// Struct de tamano FIJO: no contiene std::string (que guarda un puntero
// a memoria dinamica), sino arreglos char. Asi el struct completo se
// puede escribir y leer tal cual en un archivo binario:
//
//   archivo.write(reinterpret_cast<const char*>(&usuario), sizeof(Usuario));
//   archivo.read (reinterpret_cast<char*>(&usuario),       sizeof(Usuario));
//
// Cada registro de USER_FILE ocupa exactamente sizeof(Usuario) bytes.
// ---------------------------------------------------------------------
struct Usuario {
    int  id = 0;
    char nombre[LARGO_NOMBRE] = {};
    char username[LARGO_USERNAME] = {};
    char password[LARGO_PASSWORD] = {};
    char perfil[LARGO_PERFIL] = {};   // debe existir en PERFIL_FILE (ej: "GENERAL" o "ADMIN")
};

// Garantia en tiempo de compilacion: el struct se puede copiar byte a byte
static_assert(is_trivially_copyable<Usuario>::value,
              "Usuario debe ser trivially copyable para leerlo/escribirlo como bloque binario");

// Menu completo del modulo de usuarios (0 volver, 1 ingresar, 2 listar, 3 eliminar)
//  - perfilesValidos: nombres de perfil permitidos al crear un usuario
//  - usernameActual : usuario con sesion iniciada (no puede eliminarse a si mismo)
void menuGestionUsuarios(vector<Usuario>& listaUsuarios, const string& userFile,
                         const vector<string>& perfilesValidos, const string& usernameActual);

// Funciones individuales
void ingresarUsuario(vector<Usuario>& listaUsuarios, const string& userFile,
                     const vector<string>& perfilesValidos);
void listarUsuarios(vector<Usuario>& listaUsuarios, const string& userFile);
void eliminarUsuario(vector<Usuario>& listaUsuarios, const string& userFile,
                     const string& usernameActual);

// Carga todos los structs Usuario del archivo binario hacia memoria.
// Devuelve false si el archivo no se pudo abrir o esta danado.
bool cargarUsuarios(vector<Usuario>& listaUsuarios, const string& userFile);

#endif
