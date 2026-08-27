#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "userModule.h"
#include "profileModule.h"
#include "utils.h"

using namespace std;

// Función para leer el archivo .env y extraer las rutas
void cargarConfiguracion(string& userFile, string& perfilFile) {
    ifstream archivo(".env");
    string linea;

    if (archivo.is_open()) {
        while (getline(archivo, linea)) {
            size_t pos = linea.find('=');
            if (pos != string::npos) {
                string clave = linea.substr(0, pos);
                string valor = linea.substr(pos + 1);

                if (clave == "USER_FILE") userFile = valor;
                if (clave == "PERFIL_FILE") perfilFile = valor;
            }
        }
        archivo.close();
    } else {
        cout << "Advertencia: No se encontró el archivo .env" << endl;
    }
}

// Menú principal del sistema
void SistOpe(vector<Usuario>& listaUsuarios, vector<Perfil>& listaPerfiles, const string& userFile, const string& perfilFile) {
    int opcion = -1;

    do {
        cout << "\n======================================\n";
        cout << "   SISTEMA DE GESTION - MENU PRINCIPAL\n";
        cout << "======================================\n";
        cout << "0) Salir del Sistema\n";
        cout << "1) Administracion de Usuarios\n";
        cout << "2) Administracion de Perfiles\n";
        cout << "--------------------------------------\n";
        cout << "Seleccione una opcion: ";
        
        if (!(cin >> opcion)) {
            cout << "Entrada invalida. Ingrese un numero.\n";
            limpiarBuffer();
            continue;
        }
        limpiarBuffer();

        switch (opcion) {
            case 0:
                cout << "\nCerrando sesion y saliendo del sistema...\n";
                break;

            case 1:
                // Llama al CRUD de usuarios implementado en userModule
                menuGestionUsuarios(listaUsuarios, userFile);
                break;

            case 2:
                // Llama al CRUD de perfiles implementado en profileModule
                menuGestionPerfiles(listaPerfiles, perfilFile);
                break;

            default:
                cout << "\nOpcion no valida. Intente de nuevo.\n";
                break;
        }
    } while (opcion != 0);
}

int main() {
    string userFile = "data/USUARIOS.txt"; 
    string perfilFile = "data/PERFILES.txt";

    cargarConfiguracion(userFile, perfilFile);

    cout << "Ruta de usuarios cargada: " << userFile << endl;
    cout << "Ruta de perfiles cargada: " << perfilFile << endl;

    vector<Usuario> listaUsuarios;
    vector<Perfil> listaPerfiles;

    // Inicio del flujo principal
    SistOpe(listaUsuarios, listaPerfiles, userFile, perfilFile);

    return 0;
}