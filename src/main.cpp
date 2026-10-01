// =====================================================================
// SistOpe - MENU PRINCIPAL
//
// Uso: ./bin/SistOpe -u <usuario> -p <password> [-f <archivo.txt>]
// =====================================================================
#include "config.h"
#include "conteoArchModule.h"
#include "conteoTextoModule.h"
#include "palindromoModule.h"
#include "profileModule.h"
#include "userModule.h"
#include "utils.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

using namespace std;
namespace fs = std::filesystem;

// ---------------------------------------------------------------------
// Argumentos de ejecucion: -u usuario, -p password, -f archivo
// ---------------------------------------------------------------------
struct Argumentos {
    string usuario;
    string password;
    string archivoF;
};

static void mostrarUso(const char* programa) {
    cerr << "Uso: " << programa << " -u <usuario> -p <password> [-f <archivo.txt>]\n";
    cerr << "  -u  nombre de usuario (obligatorio)\n";
    cerr << "  -p  password (obligatorio)\n";
    cerr << "  -f  archivo de texto para la opcion 6 (CONTEO SOBRE TEXTO)\n";
    cerr << "Ejemplo: " << programa << " -u lvc -p 1001 -f \"/home/lvc/archivo.txt\"\n";
}

// Devuelve false si los argumentos no son validos
static bool parsearArgumentos(int argc, char* argv[], Argumentos& args) {
    bool vistoU = false, vistoP = false, vistoF = false;
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        string* destino = nullptr;
        bool* visto = nullptr;
        if (arg == "-u")      { destino = &args.usuario;  visto = &vistoU; }
        else if (arg == "-p") { destino = &args.password; visto = &vistoP; }
        else if (arg == "-f") { destino = &args.archivoF; visto = &vistoF; }
        else {
            cerr << "Argumento no reconocido: " << arg << "\n";
            return false;
        }
        if (*visto) {
            cerr << "El argumento " << arg << " esta repetido.\n";
            return false;
        }
        if (i + 1 >= argc || argv[i + 1][0] == '\0') {
            cerr << "Falta el valor del argumento " << arg << ".\n";
            return false;
        }
        *destino = argv[++i];
        *visto = true;
    }
    if (!vistoU || !vistoP) {
        cerr << "Los argumentos -u y -p son obligatorios para iniciar sesion.\n";
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------
// Ejecuta otro programa mediante llamadas a sistema:
//   fork()   -> crea un proceso hijo
//   execv()  -> el hijo se reemplaza por el programa indicado
//   waitpid()-> el menu espera a que el hijo termine
// No usa system() ni una shell, asi que rutas con espacios o caracteres
// especiales no pueden inyectar comandos.
// Devuelve el codigo de salida del programa, o -1 si no se pudo ejecutar.
// ---------------------------------------------------------------------
static int ejecutarPrograma(const vector<string>& argumentos) {
    if (access(argumentos[0].c_str(), X_OK) != 0) {
        cout << "\n[ERROR] No se encontro el ejecutable '" << argumentos[0]
             << "' o no tiene permisos de ejecucion. Compile el sistema con 'make'.\n";
        return -1;
    }

    cout.flush();
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }
    if (pid == 0) {
        vector<char*> argv;
        for (const auto& a : argumentos) argv.push_back(const_cast<char*>(a.c_str()));
        argv.push_back(nullptr);
        execv(argv[0], argv.data());
        perror("execv");   // solo llega aqui si execv fallo
        _exit(127);
    }

    int estado = 0;
    if (waitpid(pid, &estado, 0) < 0) {
        perror("waitpid");
        return -1;
    }
    if (WIFEXITED(estado)) return WEXITSTATUS(estado);
    if (WIFSIGNALED(estado)) {
        cout << "\n[ERROR] El programa termino por la senal " << WTERMSIG(estado) << ".\n";
    }
    return -1;
}

// ---------------------------------------------------------------------
// Autentica contra la lista de usuarios (USER_FILE)
// ---------------------------------------------------------------------
static bool autenticarUsuario(const vector<Usuario>& listaUsuarios, const string& usuario,
                              const string& password, Usuario& usuarioAutenticado) {
    for (const auto& u : listaUsuarios) {
        if (usuario == u.username && password == u.password) {
            usuarioAutenticado = u;
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------
// Permisos: la opcion debe estar en la lista del perfil (PERFIL_FILE).
// Ademas la opcion 1 es exclusiva del perfil ADMIN (enunciado).
// ---------------------------------------------------------------------
struct Sesion {
    Usuario usuario;
    Perfil perfil;          // copia del perfil del usuario (puede no tener opciones)
    string archivoF;
    Config cfg;
};

static bool tienePermiso(const Sesion& s, int opcion) {
    if (opcion == 0) return true;                                   // salir siempre
    if (opcion == 1 && s.cfg.adminPerfil != s.usuario.perfil) return false;
    return s.perfil.tienePermiso(opcion);
}

static void mostrarEncabezado(const Sesion& s) {
    limpiarPantalla();
    cout << "======================================\n";
    cout << "        SISTOPE - MENU PRINCIPAL\n";
    cout << "======================================\n";
    cout << "Usuario: " << s.usuario.username << " (" << s.usuario.nombre << ")"
         << "   |   Perfil: " << s.usuario.perfil << "\n";
    cout << "--------------------------------------\n";
}

// ---------------------------------------------------------------------
// Opcion 1: llama al programa de ADMINISTRACION DE USUARIOS Y PERFILES
// ---------------------------------------------------------------------
static void opcionAdministracion(const Sesion& s) {
    int codigo = ejecutarPrograma({s.cfg.adminBin, s.usuario.username});
    if (codigo != 0) pausar();   // hubo un error: se deja leer el mensaje
}

// ---------------------------------------------------------------------
// Opcion 2: multiplicacion de matrices -> programa aparte (MULTI_BIN)
// ---------------------------------------------------------------------
static string pedirRutaArchivo(const string& mensaje) {
    while (true) {
        string ruta = leerLinea(mensaje);
        if (ruta.size() >= 2 && (ruta.front() == '"' || ruta.front() == '\'') && ruta.back() == ruta.front()) {
            ruta = ruta.substr(1, ruta.size() - 2);
        }
        if (ruta == "0") return "";
        if (ruta.empty()) {
            cout << "Debe ingresar una ruta (o 0 para volver).\n";
            continue;
        }
        // el programa multi exige rutas completas: se convierten aqui
        error_code ec;
        fs::path absoluta = fs::absolute(ruta, ec);
        if (ec) {
            cout << "Ruta invalida.\n";
            continue;
        }
        return absoluta.lexically_normal().string();
    }
}

static void opcionMultiplicarMatrices(const Sesion& s) {
    limpiarPantalla();
    cout << "======================================\n";
    cout << "     MULTIPLICACION DE MATRICES NxM\n";
    cout << "======================================\n";
    cout << "Ingrese las rutas de los archivos con las matrices (0 para volver).\n";
    cout << "Los elementos pueden ser enteros o decimales con punto (ej: 2.5).\n";
    cout << "(Ej: data/test_matrices/A.txt y data/test_matrices/B.txt con separador #)\n\n";

    string rutaA = pedirRutaArchivo("Ruta archivo A: ");
    if (rutaA.empty()) return;
    string rutaB = pedirRutaArchivo("Ruta archivo B: ");
    if (rutaB.empty()) return;

    string separador;
    while (true) {
        separador = leerLinea("Separador de los elementos (un caracter, ej: # , ;): ");
        if (separador.size() == 1) break;
        cout << "El separador debe ser exactamente un caracter.\n";
    }

    int codigo = ejecutarPrograma({s.cfg.multiBin, rutaA, rutaB, separador,
                                   s.usuario.username, s.usuario.perfil});
    if (codigo > 0) {
        cout << "\nLa multiplicacion no se pudo realizar (codigo " << codigo
             << "). Revise el mensaje anterior.\n";
    }
    pausar();
}

// ---------------------------------------------------------------------
// Opcion 3: Juego (segun enunciado: mensaje en construccion)
// ---------------------------------------------------------------------
static void opcionJuego() {
    limpiarPantalla();
    cout << "======================================\n";
    cout << "                JUEGO\n";
    cout << "======================================\n";
    cout << "Funcionalidad EN CONSTRUCCION.\n";
    pausar();
}

// ---------------------------------------------------------------------
// Opcion 5: f(x) = x^2 + 2x + 8 con numeros reales y opcion VOLVER
// ---------------------------------------------------------------------
static void opcionCalcularFuncion() {
    int opcion = -1;
    do {
        limpiarPantalla();
        cout << "======================================\n";
        cout << "     CALCULAR f(x) = x*x + 2x + 8\n";
        cout << "======================================\n";
        cout << "1) Ingresar valor de x\n";
        cout << "0) VOLVER\n";
        opcion = leerEntero("Seleccione una opcion: ");

        if (opcion == 1) {
            double x = leerReal("x = ");
            double cuadrado = x * x;
            double doble = 2 * x;
            double resultado = cuadrado + doble + 8;

            string xs = formatearReal(x);
            cout << "\nf(x) = x*x + 2x + 8\n";
            cout << "f(" << xs << ") = (" << xs << ")*(" << xs << ") + 2*(" << xs << ") + 8\n";
            string dobleTxt = formatearReal(doble);
            if (doble < 0) dobleTxt = "(" + dobleTxt + ")";
            cout << "f(" << xs << ") = " << formatearReal(cuadrado) << " + "
                 << dobleTxt << " + 8\n";
            cout << "f(" << xs << ") = " << formatearReal(resultado) << "\n";
            pausar("\nPresione ENTER para continuar...");
        } else if (opcion != 0) {
            cout << "Opcion no valida.\n";
            pausar("Presione ENTER para continuar...");
        }
    } while (opcion != 0);
}

// ---------------------------------------------------------------------
// Menu principal: 7 opciones + salir
// ---------------------------------------------------------------------
static void menuPrincipal(const Sesion& s) {
    const vector<string> nombres = {
        "Salir",
        "Administracion de usuarios y perfiles",
        "Multiplicar matrices NxM",
        "Juego",
        "Es palindromo?",
        "Calcular f(x) = x*x + 2x + 8",
        "Conteo sobre texto (archivo de -f)",
        "Conteo sobre archivo",
    };

    int opcion = -1;
    string aviso;   // mensaje a mostrar en la proxima pantalla (error, acceso denegado)
    do {
        mostrarEncabezado(s);
        if (!aviso.empty()) {
            cout << aviso << "\n--------------------------------------\n";
            aviso.clear();
        }
        for (int i = 0; i <= OPCION_MAXIMA_MENU; i++) {
            cout << i << ") " << nombres[i];
            if (!tienePermiso(s, i)) cout << "   [sin permiso]";
            cout << "\n";
        }
        cout << "--------------------------------------\n";
        opcion = leerEntero("Seleccione una opcion: ");

        if (opcion < 0 || opcion > OPCION_MAXIMA_MENU) {
            aviso = "Opcion no valida. Ingrese un numero entre 0 y " + to_string(OPCION_MAXIMA_MENU) + ".";
            continue;
        }
        if (!tienePermiso(s, opcion)) {
            aviso = "ACCESO DENEGADO: su perfil (" + string(s.usuario.perfil) +
                    ") no tiene permiso para la opcion " + to_string(opcion) + ".";
            continue;
        }

        switch (opcion) {
            case 0:
                limpiarPantalla();
                cout << "Sesion cerrada. Hasta luego, " << s.usuario.nombre << ".\n";
                break;
            case 1: opcionAdministracion(s); break;
            case 2: opcionMultiplicarMatrices(s); break;
            case 3: opcionJuego(); break;
            case 4: menuPalindromo(); break;
            case 5: opcionCalcularFuncion(); break;
            case 6:
                if (s.archivoF.empty()) {
                    aviso = "No se indico un archivo con -f al ejecutar el programa.\n"
                            "Ejemplo: ./bin/SistOpe -u usuario -p clave -f \"/ruta/archivo.txt\"";
                } else {
                    conteoSobreTexto(s.archivoF);
                }
                break;
            case 7: conteoSobreArchivo(s.cfg.librosDir); break;
        }
    } while (opcion != 0);
}

int main(int argc, char* argv[]) {
    Argumentos args;
    if (!parsearArgumentos(argc, argv, args)) {
        mostrarUso(argv[0]);
        return 1;
    }

    Sesion s;
    s.cfg = cargarConfiguracion();
    s.archivoF = args.archivoF;

    vector<Usuario> usuarios;
    if (!cargarUsuarios(usuarios, s.cfg.userFile)) {
        cerr << "No es posible iniciar sesion sin el archivo de usuarios (USER_FILE en .env).\n";
        return 2;
    }
    if (!autenticarUsuario(usuarios, args.usuario, args.password, s.usuario)) {
        // mensaje generico: no revela si fallo el usuario o la password
        cerr << "Error de autenticacion: usuario o password incorrectos. Acceso denegado.\n";
        return 3;
    }

    vector<Perfil> perfiles;
    cargarPerfiles(perfiles, s.cfg.perfilFile);
    const Perfil* perfil = buscarPerfil(perfiles, s.usuario.perfil);
    if (perfil) {
        s.perfil = *perfil;
    } else {
        cerr << "[ADVERTENCIA] El perfil '" << s.usuario.perfil << "' no existe en "
             << s.cfg.perfilFile << ". Solo podra usar la opcion 0 (Salir).\n";
        s.cfg.huboAdvertencias = true;
        copiarTexto(s.perfil.nombre, LARGO_PERFIL, s.usuario.perfil);
    }

    if (!s.archivoF.empty()) {
        error_code ec;
        if (!fs::is_regular_file(s.archivoF, ec)) {
            cerr << "[ADVERTENCIA] El archivo indicado con -f no existe o no es un archivo: "
                 << s.archivoF << "\n";
            s.cfg.huboAdvertencias = true;
        }
    }

    // las advertencias se muestran antes de que la primera pantalla limpie la consola
    if (s.cfg.huboAdvertencias) pausar("\nPresione ENTER para continuar...");
    menuPrincipal(s);
    return 0;
}
