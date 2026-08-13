#include <iostream>
#include <fstream>
#include <string>

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
    string userFile = "default_users.txt"; 
    string perfilFile = "default_profiles.txt";
    
    // Cargamos los datos reales desde el .env
    cargarConfiguracion(userFile, perfilFile);
    
    cout << "Ruta de usuarios cargada: " << userFile << endl;
    cout << "Ruta de perfiles cargada: " << perfilFile << endl;
    
    // Aquí irá el ciclo de tu menú principal (opción 0 para salir)
    
    return 0;
}