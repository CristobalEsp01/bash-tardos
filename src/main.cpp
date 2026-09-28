#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include "userModule.h"
#include "profileModule.h"
#include "palindromoModule.h"
#include "conteoTextoModule.h"
#include "conteoArchModule.h"
#include "utils.h"

using namespace std;

// ---------------------------------------------------------------------
// Lee el archivo .env y extrae las rutas de usuarios y perfiles
// ---------------------------------------------------------------------
void cargarConfiguracion(string& userFile, string& perfilFile) {
    ifstream archivo(".env");
    string linea;

    if (archivo.is_open()) {
        while (getline(archivo, linea)) {
            // Eliminar salto de carro invisible de Windows (\r)
            if (!linea.empty() && linea.back() == '\r') {
                linea.pop_back();
            }
            
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
        cout << "Advertencia: No se encontro el archivo .env, se usaran valores por defecto.\n";
    }
}

// ---------------------------------------------------------------------
// Argumentos de ejecucion: -u usuario, -p password, -f archivo (opcional)
// ---------------------------------------------------------------------
struct Argumentos {
    string usuario;
    string password;
    string archivoF;
    bool usuarioOk = false;
    bool passwordOk = false;
};

Argumentos parsearArgumentos(int argc, char* argv[]) {
    Argumentos args;
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-u" && i + 1 < argc) {
            args.usuario = argv[++i];
            args.usuarioOk = true;
        } else if (arg == "-p" && i + 1 < argc) {
            args.password = argv[++i];
            args.passwordOk = true;
        } else if (arg == "-f" && i + 1 < argc) {
            args.archivoF = argv[++i];
        }
    }
    return args;
}

// ---------------------------------------------------------------------
// Autentica contra la lista de usuarios (cargada desde USER_FILE)
// ---------------------------------------------------------------------
bool autenticarUsuario(vector<Usuario>& listaUsuarios, const string& userFile,
                        const string& usuario, const string& password,
                        Usuario& usuarioAutenticado) {
    if (listaUsuarios.empty()) {
        cargarUsuarios(listaUsuarios, userFile);
    }
    for (const auto& u : listaUsuarios) {
        if (u.username == usuario && u.password == password) {
            usuarioAutenticado = u;
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------
// Encabezado visible en TODAS las pantallas del menu principal
// ---------------------------------------------------------------------
void mostrarEncabezado(const Usuario& usuarioActual) {
    cout << "\n======================================\n";
    cout << "        SISTOPE - MENU PRINCIPAL\n";
    cout << "======================================\n";
    cout << "Usuario: " << usuarioActual.username
         << "   |   Perfil: " << usuarioActual.perfil << "\n";
    cout << "--------------------------------------\n";
}

// ---------------------------------------------------------------------
// Opcion 2: Multiplicacion de matrices -> se ejecuta como PROGRAMA APARTE
// (coincide con el target "multi" del Makefile)
// ---------------------------------------------------------------------
void ejecutarMultiplicacionMatrices() {
    string rutaA, rutaB, separador;
    cout << "\n--- Multiplicacion de matrices NxM ---\n";
    cout << "Ruta archivo A: ";
    getline(cin, rutaA);
    cout << "Ruta archivo B: ";
    getline(cin, rutaB);
    cout << "Separador usado en las matrices: ";
    getline(cin, separador);

    string comando = "./bin/multi \"" + rutaA + "\" \"" + rutaB + "\" \"" + separador + "\"";
    cout << "\nEjecutando: " << comando << "\n\n";
    int resultado = system(comando.c_str());

    if (resultado != 0) {
        cout << "\nEl programa de multiplicacion termino con un error (codigo "
             << resultado << "). Verifique rutas y formato de los archivos.\n";
    }
}

// ---------------------------------------------------------------------
// Opcion 3: Juego (placeholder segun enunciado: "mensaje en construccion")
// ---------------------------------------------------------------------
void menuJuego() {
    cout << "\n--- Juego ---\n";
    cout << "Funcionalidad en construccion.\n";
}

// ---------------------------------------------------------------------
// Opcion 5: f(x) = x^2 + 2x + 8, con opcion VOLVER
// ---------------------------------------------------------------------
void menuCalcularFuncion() {
    int opcion;
    do {
        double x, resultado;
        cout << "\n--- Calcular f(x) = x^2 + 2x + 8 ---\n";
        cout << "Ingrese valor de x: ";
        if (!(cin >> x)) {
            cout << "Valor invalido.\n";
            limpiarBuffer();
            continue;
        }
        limpiarBuffer();

        resultado = (x * x) + (2 * x) + 8;
        cout << "f(" << x << ") = " << resultado << "\n";

        cout << "\n1) Calcular otro valor   2) Volver\n";
        cout << "Opcion: ";
        cin >> opcion;
        limpiarBuffer();
    } while (opcion == 1);
}

// ---------------------------------------------------------------------
// Menu principal (7 opciones + salir), tal como pide el enunciado
// ---------------------------------------------------------------------
void SistOpe(vector<Usuario>& listaUsuarios, vector<Perfil>& listaPerfiles,
             const string& userFile, const string& perfilFile,
             const string& archivoParametroF, const Usuario& usuarioActual) {
    int opcion = -1;

    do {
        mostrarEncabezado(usuarioActual);
        cout << "0) Salir del Sistema\n";
        cout << "1) Administracion de Usuarios y Perfiles";
        if (usuarioActual.perfil != "ADMIN") cout << "  (solo ADMIN)";
        cout << "\n";
        cout << "2) Multiplicacion de Matrices NxM\n";
        cout << "3) Juego\n";
        cout << "4) Es Palindromo?\n";
        cout << "5) Calcular f(x) = x^2 + 2x + 8\n";
        cout << "6) Conteo sobre Texto (usa archivo entregado con -f)\n";
        cout << "7) Conteo sobre Archivo\n";
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
                if (usuarioActual.perfil != "ADMIN") {
                    cout << "\nACCESO DENEGADO: esta opcion es exclusiva del perfil ADMIN.\n";
                    break;
                }
                {
                    int sub;
                    cout << "\n--- Administracion de Usuarios y Perfiles ---\n";
                    cout << "1) Usuarios\n2) Perfiles\n0) Volver\n";
                    cout << "Opcion: ";
                    cin >> sub;
                    limpiarBuffer();
                    if (sub == 1) menuGestionUsuarios(listaUsuarios, userFile);
                    else if (sub == 2) menuGestionPerfiles(listaPerfiles, perfilFile);
                }
                break;

            case 2:
                ejecutarMultiplicacionMatrices();
                break;

            case 3:
                menuJuego();
                break;

            case 4:
                menuPalindromo();
                break;

            case 5:
                menuCalcularFuncion();
                break;

            case 6:
                if (archivoParametroF.empty()) {
                    cout << "\nError: no se especifico un archivo con -f al ejecutar el programa.\n";
                    break;
                }
                conteoSobreTexto(archivoParametroF);
                break;

            case 7:
                conteoSobreArchivo();
                break;

            default:
                cout << "\nOpcion no valida. Intente de nuevo.\n";
                break;
        }
    } while (opcion != 0);
}

int main(int argc, char* argv[]) {
    string userFile = "data/USUARIOS.txt";
    string perfilFile = "data/PERFILES.txt";

    cargarConfiguracion(userFile, perfilFile);

    Argumentos args = parsearArgumentos(argc, argv);

    // Validacion de argumentos obligatorios (protege integridad del sistema:
    // nadie entra sin credenciales)
    if (!args.usuarioOk || !args.passwordOk) {
        cerr << "Uso: " << argv[0] << " -u <usuario> -p <password> [-f <archivo>]\n";
        cerr << "Los parametros -u y -p son obligatorios para iniciar sesion.\n";
        return 1;
    }

    vector<Usuario> listaUsuarios;
    vector<Perfil> listaPerfiles;
    Usuario usuarioActual;

    bool autenticado = autenticarUsuario(listaUsuarios, userFile, args.usuario, args.password, usuarioActual);
    if (!autenticado) {
        cerr << "\nError de autenticacion: usuario o password incorrectos.\n";
        cerr << "Acceso denegado. El sistema se cerrara.\n";
        return 1;
    }

    cout << "Ruta de usuarios cargada: " << userFile << endl;
    cout << "Ruta de perfiles cargada: " << perfilFile << endl;
    if (!args.archivoF.empty()) {
        cout << "Archivo de texto cargado (-f): " << args.archivoF << endl;
    }

    SistOpe(listaUsuarios, listaPerfiles, userFile, perfilFile, args.archivoF, usuarioActual);

    return 0;
}