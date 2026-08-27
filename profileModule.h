#ifndef PROFILE_MODULE_H
#define PROFILE_MODULE_H

#include <string>
#include <vector>

using namespace std;

// Representa un perfil: un nombre y las opciones de menu (numeros)
// que ese perfil puede manipular. Ej: ADMIN;0,1,2,3,4
struct Perfil {
    string nombre;
    vector<int> opciones;
};

// Menú completo del módulo de perfiles (0 salir, 1 ingresar, 2 listar, 3 eliminar)
void menuGestionPerfiles(vector<Perfil>& listaPerfiles, const string& perfilFile);

// Funciones individuales
void ingresarPerfil(vector<Perfil>& listaPerfiles, const string& perfilFile);
void listarPerfiles(vector<Perfil>& listaPerfiles, const string& perfilFile);
void eliminarPerfil(vector<Perfil>& listaPerfiles, const string& perfilFile);

#endif