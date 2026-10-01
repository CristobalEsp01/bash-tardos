#include "config.h"
#include "utils.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>

using namespace std;

Config cargarConfiguracion(const string& rutaEnv) {
    Config cfg;
    map<string, string> valoresEnv;

    ifstream archivo(rutaEnv);
    if (archivo.is_open()) {
        string linea;
        while (getline(archivo, linea)) {
            linea = trim(linea);
            if (linea.empty() || linea[0] == '#') continue;   // comentarios
            size_t pos = linea.find('=');
            if (pos == string::npos) continue;
            string clave = trim(linea.substr(0, pos));
            string valor = trim(linea.substr(pos + 1));
            // permite valores entre comillas: CLAVE="valor"
            if (valor.size() >= 2 && (valor.front() == '"' || valor.front() == '\'') &&
                valor.back() == valor.front()) {
                valor = valor.substr(1, valor.size() - 2);
            }
            valoresEnv[clave] = valor;
        }
    } else {
        cerr << "[ADVERTENCIA] No se encontro el archivo " << rutaEnv
             << ". Se usaran valores por defecto.\n";
        cfg.huboAdvertencias = true;
    }

    // Resuelve una variable: entorno del sistema > .env > defecto
    auto resolver = [&](const string& clave, string& destino) {
        const char* delSistema = getenv(clave.c_str());
        if (delSistema && *delSistema) {
            destino = delSistema;
        } else if (valoresEnv.count(clave) && !valoresEnv[clave].empty()) {
            destino = valoresEnv[clave];
        } else if (archivo.is_open()) {
            cerr << "[ADVERTENCIA] Variable " << clave << " no definida en " << rutaEnv
                 << ", se usa el valor por defecto: " << destino << "\n";
            cfg.huboAdvertencias = true;
        }
    };

    resolver("USER_FILE", cfg.userFile);
    resolver("PERFIL_FILE", cfg.perfilFile);
    resolver("ADMIN_BIN", cfg.adminBin);
    resolver("MULTI_BIN", cfg.multiBin);
    resolver("LIBROS_DIR", cfg.librosDir);
    resolver("ADMIN_PERFIL", cfg.adminPerfil);

    return cfg;
}
