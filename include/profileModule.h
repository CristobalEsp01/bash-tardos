#ifndef PROFILE_MODULE_H
#define PROFILE_MODULE_H

#include <iostream>
#include <string>
#include <vector>

using namespace std;

// Representa un perfil: un nombre y las opciones del MENU PRINCIPAL
// (numeros 0..7) que ese perfil puede usar. Ej: ADMIN;0,1,2,3,4,5,6,7
struct Perfil {
    string nombre;
    vector<int> opciones;

    bool tienePermiso(int opcion) const;
};

// Opcion mas alta del menu principal (0..7)
const int OPCION_MAXIMA_MENU = 7;

// ---------------------------------------------------------------------
// Lectura / escritura del struct COMPLETO (sobrecarga de operadores).
// Permite hacer:   archivo << perfil;   y   archivo >> perfil;
// Formato de cada registro (una linea):  NOMBRE;op1,op2,op3
// ---------------------------------------------------------------------
ostream& operator<<(ostream& os, const Perfil& p);
istream& operator>>(istream& is, Perfil& p);

// Menu completo del modulo de perfiles (0 volver, 1 ingresar, 2 listar, 3 eliminar)
void menuGestionPerfiles(vector<Perfil>& listaPerfiles, const string& perfilFile);

// Funciones individuales
void ingresarPerfil(vector<Perfil>& listaPerfiles, const string& perfilFile);
void listarPerfiles(vector<Perfil>& listaPerfiles, const string& perfilFile);
void eliminarPerfil(vector<Perfil>& listaPerfiles, const string& perfilFile);

// Carga todos los perfiles del archivo. Devuelve false si no se pudo abrir.
bool cargarPerfiles(vector<Perfil>& listaPerfiles, const string& perfilFile);

// Busca un perfil por nombre. Devuelve nullptr si no existe.
const Perfil* buscarPerfil(const vector<Perfil>& listaPerfiles, const string& nombre);

#endif
