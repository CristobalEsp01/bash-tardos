#include "config.h"
#include "utils.h"

#include <arpa/inet.h>
#include <unistd.h>

#include <charconv>
#include <filesystem>
#include <fstream>
#include <map>

using namespace std;
namespace fs = std::filesystem;

Config cargarConfiguracion(const string& rutaEnv) {
    Config cfg;
    map<string, string> valoresEnv;
    ifstream archivo(rutaEnv);
    string linea;
    while (getline(archivo, linea)) {
        linea = trim(linea);
        if (linea.empty() || linea.front() == '#') continue;
        const size_t pos = linea.find('=');
        if (pos == string::npos) continue;
        const string clave = trim(linea.substr(0, pos));
        string valor = trim(linea.substr(pos + 1));
        if (valor.size() >= 2 && (valor.front() == '"' || valor.front() == '\'') &&
            valor.back() == valor.front()) {
            valor = valor.substr(1, valor.size() - 2);
        }
        valoresEnv[clave] = valor;
    }

    auto resolver = [&](const string& clave, string& destino) {
        const char* delSistema = getenv(clave.c_str());
        if (delSistema != nullptr) {
            destino = trim(delSistema);
        } else {
            auto it = valoresEnv.find(clave);
            if (it != valoresEnv.end()) destino = it->second;
        }
    };

    resolver("USER_FILE", cfg.userFile);
    resolver("PERFIL_FILE", cfg.perfilFile);
    resolver("ADMIN_BIN", cfg.adminBin);
    resolver("MULTI_BIN", cfg.multiBin);
    resolver("JUEGO_SERVIDOR_BIN", cfg.servidorBin);
    resolver("JUEGO_CLIENTE_BIN", cfg.clienteBin);
    resolver("LIBROS_DIR", cfg.librosDir);
    resolver("ADMIN_PERFIL", cfg.adminPerfil);
    resolver("N_THREADS", cfg.nThreads);
    resolver("INDICE_INVERTIDO_PATH", cfg.indiceInvertidoPath);
    resolver("MAPEO_LIBROS_PATH", cfg.mapeoLibrosPath);
    resolver("LOG_FILE", cfg.logFile);
    resolver("HOST", cfg.host);
    resolver("BIND_ADDR", cfg.bindAddr);

    string puerto = "5000";
    resolver("PORT", puerto);
    const char* comienzo = puerto.data();
    const char* fin = comienzo + puerto.size();
    int parsed = 0;
    const auto resultado = from_chars(comienzo, fin, parsed);
    if (puerto.empty() || resultado.ec != errc{} || resultado.ptr != fin || parsed < 1 || parsed > 65535) {
        cfg.portError = "PORT debe ser un entero entre 1 y 65535.";
    } else {
        cfg.port = parsed;
    }
    return cfg;
}

static bool validarRutaSalida(const string& valor, const string& clave, string& error,
                             bool requiereDirectorioEscribible) {
    if (valor.empty()) {
        error = clave + " es obligatorio y no puede estar vacio.";
        return false;
    }
    error_code ec;
    const fs::path ruta(valor);
    const fs::path padre = ruta.has_parent_path() ? ruta.parent_path() : fs::path(".");
    if (!fs::is_directory(padre, ec)) {
        error = clave + ": carpeta de salida inexistente o invalida: " + padre.string();
        return false;
    }
    const bool existe = fs::exists(ruta, ec);
    if (ec) {
        error = clave + ": no se pudo comprobar la ruta: " + ec.message();
        return false;
    }
    if (existe && (!fs::is_regular_file(ruta, ec) || access(ruta.c_str(), W_OK) != 0)) {
        error = clave + ": archivo de salida no escribible: " + valor;
        return false;
    }
    if (ec) {
        error = clave + ": no se pudo comprobar la ruta: " + ec.message();
        return false;
    }
    if ((!existe || requiereDirectorioEscribible) && access(padre.c_str(), W_OK | X_OK) != 0) {
        error = clave + ": carpeta de salida no escribible: " + padre.string();
        return false;
    }
    return true;
}

static bool validarIPv4(const string& valor, const string& clave, string& error) {
    in_addr addr{};
    if (inet_pton(AF_INET, valor.c_str(), &addr) != 1) {
        error = clave + " debe ser una direccion IPv4 valida: " + valor;
        return false;
    }
    return true;
}

bool validarConfigIndice(const Config& cfg, string& error, int* numeroThreads) {
    error.clear();
    const char* comienzo = cfg.nThreads.data();
    const char* fin = comienzo + cfg.nThreads.size();
    int parsed = 0;
    const auto resultado = from_chars(comienzo, fin, parsed);
    if (cfg.nThreads.empty() || resultado.ec != errc{} || resultado.ptr != fin || parsed <= 0) {
        error = "N_THREADS debe ser un entero positivo.";
        return false;
    }
    constexpr int limiteThreads = 256;
    if (parsed > limiteThreads) {
        error = "N_THREADS supera el limite operativo de 256 threads.";
        return false;
    }
    error_code ec;
    for (const fs::path& salida : {fs::path(cfg.indiceInvertidoPath), fs::path(cfg.mapeoLibrosPath)}) {
        ec.clear();
        const bool enlace = fs::is_symlink(salida, ec);
        if (ec && ec != std::errc::no_such_file_or_directory) {
            error = "No se pudo inspeccionar salida del indice: " + ec.message();
            return false;
        }
        if (enlace) {
            error = "Las salidas del indice no pueden ser enlaces simbolicos.";
            return false;
        }
    }
    // El indice escribe temporales hermanos y los renombra: la carpeta necesita W_OK.
    if (!validarRutaSalida(cfg.indiceInvertidoPath, "INDICE_INVERTIDO_PATH", error, true) ||
        !validarRutaSalida(cfg.mapeoLibrosPath, "MAPEO_LIBROS_PATH", error, true)) return false;
    const fs::path indice = fs::weakly_canonical(cfg.indiceInvertidoPath, ec);
    if (ec) { error = "INDICE_INVERTIDO_PATH: " + ec.message(); return false; }
    const fs::path mapeo = fs::weakly_canonical(cfg.mapeoLibrosPath, ec);
    if (ec) { error = "MAPEO_LIBROS_PATH: " + ec.message(); return false; }
    bool mismaRuta = indice == mapeo;
    if (!mismaRuta && fs::exists(indice) && fs::exists(mapeo)) {
        mismaRuta = fs::equivalent(indice, mapeo, ec);
        if (ec) { error = "No se pudieron comparar las rutas del indice: " + ec.message(); return false; }
    }
    if (mismaRuta) {
        error = "INDICE_INVERTIDO_PATH y MAPEO_LIBROS_PATH deben ser rutas distintas.";
        return false;
    }
    // Comparar tanto rutas canonicas como identidad fisica: los hardlinks
    // comparten inodo aunque tengan nombres y directorios diferentes.
    const auto contiene = [](const fs::path& carpeta, const fs::path& ruta) {
        auto a = carpeta.begin();
        auto b = ruta.begin();
        for (; a != carpeta.end() && b != ruta.end(); ++a, ++b) {
            if (*a != *b) return false;
        }
        return a == carpeta.end();
    };
    const auto alias = [&](const fs::path& salida, const fs::path& protegida) {
        const fs::path canonica = fs::weakly_canonical(protegida);
        if (salida == canonica) return true;
        return fs::exists(salida) && fs::exists(protegida) && fs::equivalent(salida, protegida);
    };
    try {
        const fs::path libros = fs::weakly_canonical(cfg.librosDir);
        for (const fs::path& salida : {indice, mapeo}) {
            if (contiene(libros, salida)) {
                error = "Las salidas del indice no pueden estar dentro de LIBROS_DIR.";
                return false;
            }
            if (alias(salida, cfg.userFile) || alias(salida, cfg.perfilFile)) {
                error = "Las salidas del indice no pueden coincidir con USER_FILE ni PERFIL_FILE.";
                return false;
            }
            if (fs::is_directory(libros)) {
                for (fs::recursive_directory_iterator it(libros), fin; it != fin; ++it) {
                    if (it->is_regular_file() && it->path().extension() == ".txt" &&
                        alias(salida, it->path())) {
                        error = "Las salidas del indice no pueden coincidir con un libro.";
                        return false;
                    }
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        error = "No se pudieron verificar rutas protegidas del indice: " + string(e.what());
        return false;
    }
    if (numeroThreads) *numeroThreads = parsed;
    return true;
}

bool validarConfigServidor(const Config& cfg, string& error) {
    error.clear();
    if (!cfg.portError.empty()) { error = cfg.portError; return false; }
    if (!validarIPv4(cfg.bindAddr, "BIND_ADDR", error) ||
        !validarRutaSalida(cfg.logFile, "LOG_FILE", error, false)) return false;

    try {
        if (fs::is_symlink(cfg.logFile)) {
            error = "LOG_FILE no puede ser un enlace simbolico.";
            return false;
        }
        const fs::path log = fs::weakly_canonical(cfg.logFile);
        const auto coincide = [&](const string& protegida) {
            if (protegida.empty()) return false;
            const fs::path destino = fs::weakly_canonical(protegida);
            return log == destino ||
                   (fs::exists(log) && fs::exists(destino) && fs::equivalent(log, destino));
        };
        for (const auto& [ruta, nombre] : {
                 pair<string, const char*>{cfg.userFile, "USER_FILE"},
                 {cfg.perfilFile, "PERFIL_FILE"},
                 {cfg.indiceInvertidoPath, "INDICE_INVERTIDO_PATH"},
                 {cfg.mapeoLibrosPath, "MAPEO_LIBROS_PATH"}}) {
            if (coincide(ruta)) {
                error = string("LOG_FILE no puede coincidir con ") + nombre + ".";
                return false;
            }
        }
        if (!cfg.librosDir.empty()) {
            const fs::path libros = fs::weakly_canonical(cfg.librosDir);
            auto carpeta = libros.begin();
            auto archivo = log.begin();
            while (carpeta != libros.end() && archivo != log.end() && *carpeta == *archivo) {
                ++carpeta;
                ++archivo;
            }
            if (carpeta == libros.end()) {
                error = "LOG_FILE no puede estar dentro de LIBROS_DIR.";
                return false;
            }
            if (fs::exists(log) && fs::is_directory(libros)) {
                for (fs::recursive_directory_iterator it(libros), fin; it != fin; ++it) {
                    if (it->is_regular_file() && it->path().extension() == ".txt" &&
                        fs::equivalent(log, it->path())) {
                        error = "LOG_FILE no puede coincidir con un libro de LIBROS_DIR.";
                        return false;
                    }
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        error = "No se pudieron verificar rutas protegidas para LOG_FILE: " + string(e.what());
        return false;
    }
    return true;
}

bool validarConfigCliente(const Config& cfg, string& error) {
    error.clear();
    if (!cfg.portError.empty()) { error = cfg.portError; return false; }
    return validarIPv4(cfg.host, "HOST", error);
}
