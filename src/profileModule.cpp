#include "profileModule.h"
#include "utils.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace std;

bool Perfil::tienePermiso(int opcion) const {
    return opcion >= 0 && opcion <= OPCION_MAXIMA_MENU && opciones[opcion];
}

string opcionesComoTexto(const Perfil& p) {
    string texto;
    for (int i = 0; i <= OPCION_MAXIMA_MENU; i++) {
        if (p.opciones[i]) texto += (texto.empty() ? "" : ",") + to_string(i);
    }
    return texto;
}

// =====================================================================
// Lectura / escritura del struct COMPLETO en archivo binario
// =====================================================================

bool cargarPerfiles(vector<Perfil>& listaPerfiles, const string& perfilFile) {
    listaPerfiles.clear();
    ifstream archivo(perfilFile, ios::binary);
    if (!archivo.is_open()) {
        cerr << "[ERROR] No se pudo abrir el archivo de perfiles: '" << perfilFile << "'\n";
        return false;
    }

    archivo.seekg(0, ios::end);
    streamoff tamano = archivo.tellg();
    archivo.seekg(0, ios::beg);
    if (tamano % static_cast<streamoff>(sizeof(Perfil)) != 0) {
        cerr << "[ERROR] El archivo de perfiles '" << perfilFile << "' esta danado o no tiene el formato "
             << "binario esperado (" << tamano << " bytes no es multiplo de " << sizeof(Perfil) << ").\n";
        return false;
    }

    Perfil p;
    while (archivo.read(reinterpret_cast<char*>(&p), sizeof(Perfil))) {   // struct completo
        p.nombre[LARGO_PERFIL - 1] = '\0';
        listaPerfiles.push_back(p);
    }
    return true;
}

static bool reescribirArchivoPerfiles(const vector<Perfil>& listaPerfiles, const string& perfilFile) {
    ofstream archivo(perfilFile, ios::binary | ios::trunc);
    if (!archivo.is_open()) {
        cout << "[ERROR] No se pudo escribir en " << perfilFile << "\n";
        return false;
    }
    for (const auto& p : listaPerfiles) {
        archivo.write(reinterpret_cast<const char*>(&p), sizeof(Perfil));   // struct completo
    }
    return static_cast<bool>(archivo);
}

static bool agregarPerfilAlArchivo(const Perfil& p, const string& perfilFile) {
    ofstream archivo(perfilFile, ios::binary | ios::app);
    if (!archivo.is_open()) {
        cout << "[ERROR] No se pudo abrir " << perfilFile << " para escritura.\n";
        return false;
    }
    archivo.write(reinterpret_cast<const char*>(&p), sizeof(Perfil));       // struct completo
    return static_cast<bool>(archivo);
}

// =====================================================================
// Utilidades internas
// =====================================================================

const Perfil* buscarPerfil(const vector<Perfil>& listaPerfiles, const string& nombre) {
    for (const auto& p : listaPerfiles) {
        if (nombre == p.nombre) return &p;
    }
    return nullptr;
}

// Marca en p.opciones los numeros de "0, 2,3". Ignora (y avisa) valores
// que no esten entre 0 y OPCION_MAXIMA_MENU.
static void parsearOpciones(const string& texto, Perfil& p) {
    stringstream ss(texto);
    string numStr;
    while (getline(ss, numStr, ',')) {
        numStr = trim(numStr);
        if (numStr.empty()) continue;
        try {
            size_t pos = 0;
            int n = stoi(numStr, &pos);
            if (pos != numStr.size() || n < 0 || n > OPCION_MAXIMA_MENU) throw 1;
            p.opciones[n] = true;
        } catch (...) {
            cout << "Aviso: se ignoro el valor '" << numStr << "' (debe ser un numero entre 0 y "
                 << OPCION_MAXIMA_MENU << ").\n";
        }
    }
}

static void imprimirTablaPerfiles(const vector<Perfil>& listaPerfiles) {
    if (listaPerfiles.empty()) {
        cout << "(no hay perfiles registrados)\n";
        return;
    }
    cout << left << setw(15) << "Nombre" << "Opciones permitidas\n";
    for (const auto& p : listaPerfiles) {
        cout << left << setw(15) << p.nombre << opcionesComoTexto(p) << "\n";
    }
}

static void encabezado(const string& titulo) {
    limpiarPantalla();
    cout << "======================================\n";
    cout << "  " << titulo << "\n";
    cout << "======================================\n";
}

// =====================================================================
// Operaciones del modulo
// =====================================================================

void ingresarPerfil(vector<Perfil>& listaPerfiles, const string& perfilFile) {
    if (listaPerfiles.empty()) cargarPerfiles(listaPerfiles, perfilFile);
    encabezado("INGRESO DE PERFILES");

    Perfil nuevo;
    string nombre;
    while (true) {
        nombre = leerLinea("Nombre del perfil (ej: GENERAL, ADMIN): ");
        transform(nombre.begin(), nombre.end(), nombre.begin(), ::toupper);
        if (nombre.empty() || nombre.find_first_of(" ,") != string::npos) {
            cout << "El nombre no puede estar vacio ni contener espacios o ','.\n";
        } else if (nombre.size() > static_cast<size_t>(LARGO_PERFIL - 1)) {
            cout << "El nombre admite como maximo " << LARGO_PERFIL - 1 << " caracteres.\n";
        } else {
            break;
        }
    }

    if (buscarPerfil(listaPerfiles, nombre)) {
        cout << "Error: ya existe un perfil con ese nombre. Ingreso cancelado.\n";
        pausar("\nPresione ENTER para continuar...");
        return;
    }
    copiarTexto(nuevo.nombre, LARGO_PERFIL, nombre);

    cout << "Opciones del menu principal que puede usar (0 a " << OPCION_MAXIMA_MENU
         << ", separadas por coma, ej: 0,2,3,4)\n";
    parsearOpciones(leerLinea("Opciones: "), nuevo);
    if (!nuevo.opciones[0]) {
        nuevo.opciones[0] = true;   // Salir siempre permitido
        cout << "Se agrego la opcion 0 (Salir), que es obligatoria para todo perfil.\n";
    }
    cout << "Opciones del perfil " << nuevo.nombre << ": " << opcionesComoTexto(nuevo) << "\n";

    cout << "\n1) Guardar   2) Cancelar\n";
    int opcion = leerEntero("Opcion: ");

    if (opcion == 1) {
        if (agregarPerfilAlArchivo(nuevo, perfilFile)) {
            listaPerfiles.push_back(nuevo);
            cout << "Perfil guardado correctamente.\n";
        }
    } else {
        cout << "Ingreso cancelado.\n";
    }
    pausar("\nPresione ENTER para continuar...");
}

void listarPerfiles(vector<Perfil>& listaPerfiles, const string& perfilFile) {
    if (listaPerfiles.empty()) cargarPerfiles(listaPerfiles, perfilFile);
    encabezado("LISTA DE PERFILES");
    imprimirTablaPerfiles(listaPerfiles);
    pausar("\nPresione ENTER para continuar...");
}

void eliminarPerfil(vector<Perfil>& listaPerfiles, const string& perfilFile) {
    if (listaPerfiles.empty()) cargarPerfiles(listaPerfiles, perfilFile);
    encabezado("ELIMINAR PERFIL");
    imprimirTablaPerfiles(listaPerfiles);
    cout << "\n";

    string nombreBuscado = leerLinea("Nombre del perfil a borrar: ");
    transform(nombreBuscado.begin(), nombreBuscado.end(), nombreBuscado.begin(), ::toupper);

    int indice = -1;
    for (size_t i = 0; i < listaPerfiles.size(); i++) {
        if (nombreBuscado == listaPerfiles[i].nombre) indice = static_cast<int>(i);
    }

    if (indice == -1) {
        cout << "No existe un perfil con ese nombre.\n";
    } else {
        if (nombreBuscado == "ADMIN") {
            cout << "\n*** ALERTA: esta a punto de eliminar el perfil ADMIN. ***\n";
            cout << "*** Esto puede dejar sin permisos administrativos al sistema. ***\n";
        }

        cout << "\n1) Confirmar   2) Cancelar\n";
        int opcion = leerEntero("Opcion: ");

        if (opcion == 1) {
            vector<Perfil> copia = listaPerfiles;
            copia.erase(copia.begin() + indice);
            if (reescribirArchivoPerfiles(copia, perfilFile)) {
                listaPerfiles = copia;
                cout << "Perfil eliminado correctamente.\n";
            }
        } else {
            cout << "Eliminacion cancelada.\n";
        }
    }
    pausar("\nPresione ENTER para continuar...");
}

void menuGestionPerfiles(vector<Perfil>& listaPerfiles, const string& perfilFile) {
    int opcion = -1;
    do {
        encabezado("GESTION DE PERFILES");
        cout << "0) Volver\n";
        cout << "1) Ingresar Perfiles\n";
        cout << "2) Listar Perfiles\n";
        cout << "3) Eliminar Perfiles\n";
        opcion = leerEntero("Opcion: ");

        switch (opcion) {
            case 0: break;
            case 1: ingresarPerfil(listaPerfiles, perfilFile); break;
            case 2: listarPerfiles(listaPerfiles, perfilFile); break;
            case 3: eliminarPerfil(listaPerfiles, perfilFile); break;
            default:
                cout << "Opcion invalida.\n";
                pausar("Presione ENTER para continuar...");
        }
    } while (opcion != 0);
}
