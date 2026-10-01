#ifndef PROFILE_MODULE_H
#define PROFILE_MODULE_H

#include "userModule.h"   // LARGO_PERFIL

#include <string>
#include <type_traits>
#include <vector>

using namespace std;

// Opcion mas alta del menu principal (0..7)
const int OPCION_MAXIMA_MENU = 7;

// ---------------------------------------------------------------------
// Un perfil: su nombre y que opciones del MENU PRINCIPAL puede usar.
// opciones[i] == true  ->  el perfil tiene permiso para la opcion i.
// Ej: ADMIN -> opciones 0..7      GENERAL -> opciones 0,2,3,4,5,6,7
//
// Igual que Usuario, es un struct de tamano FIJO que se lee y escribe
// completo en PERFIL_FILE con read() / write() en modo binario.
// ---------------------------------------------------------------------
struct Perfil {
    char nombre[LARGO_PERFIL] = {};
    bool opciones[OPCION_MAXIMA_MENU + 1] = {};

    bool tienePermiso(int opcion) const;
};

static_assert(is_trivially_copyable<Perfil>::value,
              "Perfil debe ser trivially copyable para leerlo/escribirlo como bloque binario");

// Menu completo del modulo de perfiles (0 volver, 1 ingresar, 2 listar, 3 eliminar)
void menuGestionPerfiles(vector<Perfil>& listaPerfiles, const string& perfilFile);

// Funciones individuales
void ingresarPerfil(vector<Perfil>& listaPerfiles, const string& perfilFile);
void listarPerfiles(vector<Perfil>& listaPerfiles, const string& perfilFile);
void eliminarPerfil(vector<Perfil>& listaPerfiles, const string& perfilFile);

// Carga todos los structs Perfil del archivo binario.
// Devuelve false si el archivo no se pudo abrir o esta danado.
bool cargarPerfiles(vector<Perfil>& listaPerfiles, const string& perfilFile);

// Busca un perfil por nombre. Devuelve nullptr si no existe.
const Perfil* buscarPerfil(const vector<Perfil>& listaPerfiles, const string& nombre);

// Texto "0,2,3" con las opciones permitidas (para mostrar en pantalla)
string opcionesComoTexto(const Perfil& p);

#endif
