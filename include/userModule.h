#ifndef USER_MODULE_H
#define USER_MODULE_H

#include <iostream>
#include <string>
#include <vector>

using namespace std;

struct Usuario {
    int id = 0;
    string nombre;
    string username;
    string password;
    string perfil; // debe existir en PERFIL_FILE (ej: "GENERAL" o "ADMIN")
};

// ---------------------------------------------------------------------
// Lectura / escritura del struct COMPLETO (sobrecarga de operadores).
// Permite hacer:   archivo << usuario;   y   archivo >> usuario;
// Formato de cada registro (una linea):  id;nombre;username;password;perfil
// ---------------------------------------------------------------------
ostream& operator<<(ostream& os, const Usuario& u);
istream& operator>>(istream& is, Usuario& u);

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

// Carga la lista completa de usuarios desde archivo hacia memoria.
// Devuelve false si el archivo no se pudo abrir.
bool cargarUsuarios(vector<Usuario>& listaUsuarios, const string& userFile);

#endif
