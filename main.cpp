#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "userModule.h"

using namespace std;

// Listar los usuarios leyendo desde memoria o desde el archivo
void listarUsuarios(vector<Usuario>& usuarios, const string& userFile) {
    cout << "\nId\tNombre\tPerfil\n";
    
    if (usuarios.size() > 0) {
        // Si hay datos en el arreglo, lee de la memoria
        for (int i = 0; i < usuarios.size(); i++) {
            cout << usuarios[i].id << "\t" << usuarios[i].nombre << "\t" << usuarios[i].perfil << "\n";
        }
    } else {
        // Si no, los lee del archivo de texto
        ifstream archivo(userFile);
        string linea;
        if (archivo.is_open()) {
            while (getline(archivo, linea)) {
                cout << linea << "\n";
            }
            archivo.close();
        }
    }
    
    // Volver al menu anterior
    int opc;
    do {
        cout << "\n0) para Volver : ";
        cin >> opc;
    } while (opc != 0);
}

// Menu de opciones con usuarios (0 salir, 1 ingresar, 2 listar, 3 eliminar)
void menuUsuarios(vector<Usuario>& usuarios, const string& userFile) {
    int opc;
    do {
        cout << "\n--- Modulo - Gestion de Usuarios ---\n";
        cout << "0) Salir\n1) Ingresar Usuarios\n2) Listar Usuarios\n3) Eliminar Usuarios\nOpcion : ";
        cin >> opc;
        
        if (opc == 2) {
            listarUsuarios(usuarios, userFile);
        } else if (opc == 1 || opc == 3) {
            cout << "Opcion en desarrollo...\n";
        }
    } while (opc != 0);
}

int main() {
    const string userFile = "data/USUARIOS.txt"; 
    const string perfilFile = "data/PERFILES.txt";
    vector<Usuario> usuarios;

    // Aquí iría el menú general del sistema (usuarios / perfiles / salir).
    int opcPrincipal;
    do {
        cout << "\n=== SistOpe ===\n";
        cout << "0) Salir\n1) Gestion de Usuarios\n2) Gestion de Perfiles\nOpcion : ";
        cin >> opcPrincipal;
        
        if (opcPrincipal == 1) {
            menuUsuarios(usuarios, userFile);
        } else if (opcPrincipal == 2) {
            cout << "Modulo de perfiles en construccion\n";
        }
    } while (opcPrincipal != 0);
    
    return 0;
}