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

// Funciones individuales (por si se quieren llamar por separado desde el main)
void ingresarUsuario(vector<Usuario>& listaUsuarios, const string& userFile);
void listarUsuarios(vector<Usuario>& listaUsuarios, const string& userFile);
void eliminarUsuario(vector<Usuario>& listaUsuarios, const string& userFile);

#endif
