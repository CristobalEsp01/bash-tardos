#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "userModule.h"

using namespace std;

// Función para leer el archivo .env y extraer las rutas
void cargarConfiguracion(string& userFile, string& perfilFile) {
    ifstream archivo(".env");
    string linea;
    
    if (archivo.is_open()) {
        while (getline(archivo, linea)) {
            // Buscamos la posición del signo '='
            size_t pos = linea.find('=');
            if (pos != string::npos) {
                string clave = linea.substr(0, pos);
                string valor = linea.substr(pos + 1);
                
                // Asignamos según la clave encontrada
                if (clave == "USER_FILE") userFile = valor;
                if (clave == "PERFIL_FILE") perfilFile = valor;
            }
        }
        archivo.close();
    } else {
        cout << "Advertencia: No se encontró el archivo .env" << endl;
    }
}

int main() {
    string userFile = "data/USUARIOS.txt"; 
    string perfilFile = "data/PERFILES.txt";
    
    // Cargamos los datos reales desde el .env
    cargarConfiguracion(userFile, perfilFile);
    
    cout << "Ruta de usuarios cargada: " << userFile << endl;
    cout << "Ruta de perfiles cargada: " << perfilFile << endl;
    
    vector<Usuario> listaUsuarios;

    // Aquí iría el menú general del sistema (usuarios / perfiles / salir).
    // Por ahora se llama directo al módulo de usuarios:
    menuGestionUsuarios(listaUsuarios, userFile);
    
    return 0;
}