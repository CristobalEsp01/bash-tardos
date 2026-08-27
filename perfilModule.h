#ifndef PROFILE_MODULE_H
#define PROFILE_MODULE_H

#include <string>
#include <vector>

using namespace std;

struct Perfil {
    string nombre; // "GENERAL" o "ADMIN"
    vector<int> permisos;
};

// Menú completo del módulo de usuarios (0 salir, 1 ingresar, 2 listar, 3 eliminar)
void menuGestionPerfiles(vector<Perfil>& listaPerfiles, const string& perfilFile);

// Funciones individuales
void ingresarPermisoPerfil(vector<Perfil>& listaPerfiles, const string& perfilFile);
void listarPerfiles(vector<Perfil>& listaPerfiles, const string& perfilFile);
void eliminarPermisoPerfil(vector<Perfil>& listaPerfiles, const string& perfilFile);

#endif
