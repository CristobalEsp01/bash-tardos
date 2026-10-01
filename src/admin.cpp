// =====================================================================
// Programa "ADMINISTRACION DE USUARIOS Y PERFILES" (modulo de la Entrega 1)
//
// Es un ejecutable independiente (bin/admin) que el MENU PRINCIPAL
// invoca mediante llamadas a sistema (fork + execv + waitpid).
//
// Uso: ./bin/admin <username>
//   El username debe existir en USER_FILE y tener el perfil ADMIN
//   (ADMIN_PERFIL en el .env); en caso contrario el acceso se rechaza.
// =====================================================================
#include "config.h"
#include "profileModule.h"
#include "userModule.h"
#include "utils.h"

#include <iostream>

using namespace std;

static vector<string> nombresPerfiles(const vector<Perfil>& perfiles) {
    vector<string> nombres;
    for (const auto& p : perfiles) nombres.push_back(p.nombre);
    return nombres;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Uso: " << argv[0] << " <username>\n";
        cerr << "Este programa se ejecuta desde el MENU PRINCIPAL (opcion 1).\n";
        return 1;
    }

    Config cfg = cargarConfiguracion();
    string username = argv[1];

    vector<Usuario> usuarios;
    vector<Perfil> perfiles;
    if (!cargarUsuarios(usuarios, cfg.userFile)) return 2;
    if (!cargarPerfiles(perfiles, cfg.perfilFile)) return 2;

    // Verificacion de integridad: solo un usuario ADMIN existente puede entrar
    const Usuario* actual = nullptr;
    for (const auto& u : usuarios) {
        if (u.username == username) actual = &u;
    }
    if (!actual || actual->perfil != cfg.adminPerfil) {
        cerr << "ACCESO DENEGADO: solo usuarios con perfil " << cfg.adminPerfil
             << " pueden administrar usuarios y perfiles.\n";
        return 3;
    }
    string perfilActual = actual->perfil;   // copia: 'usuarios' puede cambiar

    int opcion = -1;
    do {
        limpiarPantalla();
        cout << "======================================\n";
        cout << "  ADMINISTRACION DE USUARIOS Y PERFILES\n";
        cout << "======================================\n";
        cout << "Usuario: " << username << "   |   Perfil: " << perfilActual << "\n";
        cout << "--------------------------------------\n";
        cout << "0) Volver al MENU PRINCIPAL\n";
        cout << "1) Gestion de Usuarios\n";
        cout << "2) Gestion de Perfiles\n";
        opcion = leerEntero("Seleccione una opcion: ");

        switch (opcion) {
            case 0:
                break;
            case 1:
                menuGestionUsuarios(usuarios, cfg.userFile, nombresPerfiles(perfiles), username);
                break;
            case 2:
                menuGestionPerfiles(perfiles, cfg.perfilFile);
                break;
            default:
                cout << "Opcion no valida. Intente de nuevo.\n";
                pausar("Presione ENTER para continuar...");
        }
    } while (opcion != 0);

    return 0;
}
