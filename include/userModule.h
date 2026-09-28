#ifndef USER_MODULE_H
#define USER_MODULE_H

#include <string>
#include <vector>

using namespace std;

struct Usuario {
    int id;
    string nombre;
    string username;
    string password;
    string perfil; // "GENERAL" o "ADMIN"
};

// Menú completo del módulo de usuarios (0 salir, 1 ingresar, 2 listar, 3 eliminar)
void menuGestionUsuarios(vector<Usuario>& listaUsuarios, const string& userFile);

// Funciones individuales
void ingresarUsuario(vector<Usuario>& listaUsuarios, const string& userFile);
void listarUsuarios(vector<Usuario>& listaUsuarios, const string& userFile);
void eliminarUsuario(vector<Usuario>& listaUsuarios, const string& userFile);

// NUEVO: carga la lista completa de usuarios desde archivo hacia memoria.
// Se necesita como función pública para poder autenticar al usuario en
// main.cpp ANTES de entrar al menú (login con -u / -p).
void cargarUsuarios(vector<Usuario>& listaUsuarios, const string& userFile);

#endif
