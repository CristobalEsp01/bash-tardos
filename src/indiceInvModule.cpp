#include "indiceInvModule.h"
#include "utils.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <mutex>
#include <pthread.h>
#include <queue>
#include <semaphore.h>
#include <sstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;

namespace {
struct Libro {
    int id;
    fs::path absoluta;
    std::string relativa;
};

using Conteo = std::map<std::string, std::uint64_t>;
using Indice = std::map<std::string, std::map<int, std::uint64_t>>;

struct Trabajo {
    const std::vector<Libro>* libros = nullptr;
    std::queue<int> cola;
    Indice indice;
    sem_t disponibles{};
    sem_t mutexCola{};
    sem_t mutexIndice{};
    std::atomic<bool> fallo{false};
    std::mutex mutexError;
    std::string error;

    void registrarError(const std::string& mensaje) {
        fallo = true;
        std::lock_guard<std::mutex> lock(mutexError);
        if (error.empty()) error = mensaje;
    }
};

bool esperar(sem_t* sem, Trabajo& trabajo, const char* nombre) {
    int resultado;
    do { resultado = sem_wait(sem); } while (resultado == -1 && errno == EINTR);
    if (resultado == 0) return true;
    trabajo.registrarError(std::string(nombre) + ": " + std::strerror(errno));
    return false;
}

bool senalar(sem_t* sem, Trabajo& trabajo, const char* nombre) {
    if (sem_post(sem) == 0) return true;
    trabajo.registrarError(std::string(nombre) + ": " + std::strerror(errno));
    return false;
}

std::vector<std::string> normalizar(const std::string& palabra) {
    std::vector<std::string> resultado;
    std::string actual;
    for (size_t i = 0; i < palabra.size();) {
        const unsigned int cp = siguienteCaracterUtf8(palabra, i);
        const char base = letraBase(cp);
        if (base != 0) actual.push_back(base);
        else if (cp >= '0' && cp <= '9') actual.push_back(static_cast<char>(cp));
        else if (cp >= 0x300 && cp <= 0x36F) continue; // tilde combinante
        else if (!actual.empty()) {
            resultado.push_back(std::move(actual));
            actual.clear();
        }
    }
    if (!actual.empty()) resultado.push_back(std::move(actual));
    return resultado;
}

bool descubrirLibros(const fs::path& directorio, std::vector<Libro>& libros, std::string& error) {
    std::error_code ec;
    if (!fs::is_directory(directorio, ec) || ec) {
        error = "LIBROS_DIR no existe o no es directorio: " + directorio.string();
        return false;
    }
    if (access(directorio.c_str(), R_OK | X_OK) != 0) {
        error = "LIBROS_DIR no se puede recorrer: " + directorio.string();
        return false;
    }
    fs::recursive_directory_iterator it(directorio, fs::directory_options::none, ec), fin;
    if (ec) { error = "No se puede recorrer LIBROS_DIR: " + ec.message(); return false; }
    std::vector<std::pair<std::string, fs::path>> encontrados;
    while (it != fin) {
        const fs::path ruta = it->path();
        const bool regular = it->is_regular_file(ec);
        if (ec) { error = "No se puede inspeccionar libro: " + ec.message(); return false; }
        if (regular && ruta.extension() == ".txt") {
            const std::string relativa = ruta.lexically_relative(directorio).generic_string();
            if (relativa.empty() || relativa.find('\n') != std::string::npos ||
                relativa.find('\r') != std::string::npos || relativa.find(';') != std::string::npos) {
                error = "Ruta de libro incompatible con formato de mapeo: " + ruta.string();
                return false;
            }
            std::ifstream entrada(ruta, std::ios::binary);
            if (!entrada) { error = "No se puede abrir libro: " + ruta.string(); return false; }
            entrada.peek();
            if (entrada.bad()) { error = "No se puede leer libro: " + ruta.string(); return false; }
            encontrados.emplace_back(relativa, ruta);
        }
        it.increment(ec);
        if (ec) { error = "No se puede recorrer LIBROS_DIR: " + ec.message(); return false; }
    }
    if (encontrados.empty()) {
        error = "LIBROS_DIR no contiene libros .txt legibles.";
        return false;
    }
    std::sort(encontrados.begin(), encontrados.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });
    if (encontrados.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
        error = "Demasiados libros para asignar IDs.";
        return false;
    }
    libros.reserve(encontrados.size());
    for (size_t i = 0; i < encontrados.size(); ++i)
        libros.push_back({static_cast<int>(i + 1), encontrados[i].second, encontrados[i].first});
    return true;
}

bool contarLibro(const Libro& libro, Conteo& conteo, std::string& error) {
    std::ifstream entrada(libro.absoluta, std::ios::binary);
    if (!entrada) { error = "No se puede abrir libro: " + libro.absoluta.string(); return false; }
    std::string linea, palabra;
    while (std::getline(entrada, linea)) {
        std::istringstream separador(linea);
        while (separador >> palabra) {
            for (const std::string& clave : normalizar(palabra)) {
                auto& cantidad = conteo[clave];
                if (cantidad == std::numeric_limits<std::uint64_t>::max()) {
                    error = "Conteo excede capacidad: " + libro.absoluta.string();
                    return false;
                }
                ++cantidad;
            }
        }
    }
    if (entrada.bad()) { error = "Error al leer libro: " + libro.absoluta.string(); return false; }
    return true;
}

void* trabajador(void* argumento) {
    auto& trabajo = *static_cast<Trabajo*>(argumento);
    try {
        while (true) {
            if (!esperar(&trabajo.disponibles, trabajo, "sem_wait disponibles")) break;
            if (!esperar(&trabajo.mutexCola, trabajo, "sem_wait cola")) break;
            if (trabajo.cola.empty()) {
                trabajo.registrarError("Cola vacia despues de senal de trabajo.");
                senalar(&trabajo.mutexCola, trabajo, "sem_post cola");
                break;
            }
            const int posicion = trabajo.cola.front();
            trabajo.cola.pop();
            if (!senalar(&trabajo.mutexCola, trabajo, "sem_post cola")) break;
            if (posicion < 0) break;

            Conteo local;
            std::string error;
            if (!contarLibro((*trabajo.libros)[static_cast<size_t>(posicion)], local, error)) {
                trabajo.registrarError(error);
                continue;
            }
            if (!esperar(&trabajo.mutexIndice, trabajo, "sem_wait indice")) break;
            const int id = (*trabajo.libros)[static_cast<size_t>(posicion)].id;
            try {
                for (const auto& par : local) trabajo.indice[par.first][id] = par.second;
            } catch (...) {
                senalar(&trabajo.mutexIndice, trabajo, "sem_post indice");
                throw;
            }
            if (!senalar(&trabajo.mutexIndice, trabajo, "sem_post indice")) break;
        }
    } catch (const std::exception& e) {
        trabajo.registrarError(std::string("Worker fallo: ") + e.what());
    } catch (...) {
        trabajo.registrarError("Worker fallo por error desconocido.");
    }
    return nullptr;
}

bool encolar(Trabajo& trabajo, int posicion) {
    if (!esperar(&trabajo.mutexCola, trabajo, "sem_wait cola")) return false;
    try {
        trabajo.cola.push(posicion);
    } catch (const std::exception& e) {
        trabajo.registrarError(std::string("No se puede encolar libro: ") + e.what());
        senalar(&trabajo.mutexCola, trabajo, "sem_post cola");
        return false;
    }
    if (!senalar(&trabajo.mutexCola, trabajo, "sem_post cola")) return false;
    return senalar(&trabajo.disponibles, trabajo, "sem_post disponibles");
}

bool ejecutarWorkers(const std::vector<Libro>& libros, int numero, Indice& indice, std::string& error) {
    Trabajo trabajo;
    trabajo.libros = &libros;
    std::vector<pthread_t> threads;
    threads.reserve(static_cast<size_t>(numero));
    if (sem_init(&trabajo.disponibles, 0, 0) != 0) {
        error = "sem_init disponibles: " + std::string(std::strerror(errno)); return false;
    }
    if (sem_init(&trabajo.mutexCola, 0, 1) != 0) {
        error = "sem_init cola: " + std::string(std::strerror(errno));
        sem_destroy(&trabajo.disponibles); return false;
    }
    if (sem_init(&trabajo.mutexIndice, 0, 1) != 0) {
        error = "sem_init indice: " + std::string(std::strerror(errno));
        sem_destroy(&trabajo.mutexCola); sem_destroy(&trabajo.disponibles); return false;
    }
    for (int i = 0; i < numero; ++i) {
        pthread_t thread;
        const int resultado = pthread_create(&thread, nullptr, trabajador, &trabajo);
        if (resultado != 0) {
            trabajo.registrarError("pthread_create: " + std::string(std::strerror(resultado)));
            break;
        }
        threads.push_back(thread);
    }
    if (!trabajo.fallo) {
        for (size_t i = 0; i < libros.size(); ++i) {
            if (!encolar(trabajo, static_cast<int>(i))) break;
        }
    }
    for (size_t i = 0; i < threads.size(); ++i) {
        if (!encolar(trabajo, -1)) {
            for (pthread_t thread : threads) pthread_cancel(thread);
            break;
        }
    }
    for (pthread_t thread : threads) {
        const int resultado = pthread_join(thread, nullptr);
        if (resultado != 0) trabajo.registrarError("pthread_join: " + std::string(std::strerror(resultado)));
    }
    if (sem_destroy(&trabajo.mutexIndice) != 0) trabajo.registrarError("sem_destroy indice: " + std::string(std::strerror(errno)));
    if (sem_destroy(&trabajo.mutexCola) != 0) trabajo.registrarError("sem_destroy cola: " + std::string(std::strerror(errno)));
    if (sem_destroy(&trabajo.disponibles) != 0) trabajo.registrarError("sem_destroy disponibles: " + std::string(std::strerror(errno)));
    if (trabajo.fallo) { error = trabajo.error; return false; }
    indice = std::move(trabajo.indice);
    return true;
}

bool escribirTemporal(const fs::path& destino,
                      const std::function<void(std::ostream&)>& escribir,
                      fs::path& temporal, std::string& error) {
    struct stat estado{};
    mode_t modoFinal;
    if (stat(destino.c_str(), &estado) == 0) {
        modoFinal = estado.st_mode & 07777;
    } else if (errno == ENOENT) {
        const mode_t mascara = umask(0);
        umask(mascara);
        modoFinal = 0644 & ~mascara;
    } else {
        error = "No se puede consultar salida: " + std::string(std::strerror(errno));
        return false;
    }
    std::string plantilla = destino.string() + ".tmp.XXXXXX";
    std::vector<char> nombre(plantilla.begin(), plantilla.end());
    nombre.push_back('\0');
    const int descriptor = mkstemp(nombre.data());
    if (descriptor < 0) { error = "No se puede crear temporal: " + std::string(std::strerror(errno)); return false; }
    temporal = nombre.data();
    if (close(descriptor) != 0) {
        error = "No se puede cerrar temporal: " + std::string(std::strerror(errno));
        fs::remove(temporal); return false;
    }
    std::ofstream salida(temporal, std::ios::binary | std::ios::trunc);
    if (!salida) { error = "No se puede abrir temporal: " + temporal.string(); fs::remove(temporal); return false; }
    escribir(salida);
    salida.flush();
    const bool bien = static_cast<bool>(salida);
    salida.close();
    if (!bien || !salida) {
        error = "No se puede escribir temporal: " + temporal.string();
        fs::remove(temporal); return false;
    }
    if (chmod(temporal.c_str(), modoFinal) != 0) {
        error = "No se puede fijar modo de temporal: " + std::string(std::strerror(errno));
        fs::remove(temporal); return false;
    }
    return true;
}

bool escribirResultados(const Config& cfg, const std::vector<Libro>& libros,
                        const Indice& indice, std::string& error) {
    fs::path mapaTmp, indiceTmp;
    const bool mapaBien = escribirTemporal(cfg.mapeoLibrosPath, [&](std::ostream& salida) {
        for (const Libro& libro : libros) salida << libro.id << ';' << libro.relativa << '\n';
    }, mapaTmp, error);
    if (!mapaBien) return false;
    const bool indiceBien = escribirTemporal(cfg.indiceInvertidoPath, [&](std::ostream& salida) {
        for (const auto& palabra : indice) {
            salida << palabra.first;
            for (const auto& libro : palabra.second)
                salida << ";(" << libro.first << ',' << libro.second << ')';
            salida << '\n';
        }
    }, indiceTmp, error);
    if (!indiceBien) { fs::remove(mapaTmp); return false; }
    return publicarArchivosIndice(mapaTmp.string(), cfg.mapeoLibrosPath,
                                 indiceTmp.string(), cfg.indiceInvertidoPath, error);
}
} // namespace

bool publicarArchivosIndice(const std::string& mapeoTemporal,
                           const std::string& mapeoDestino,
                           const std::string& indiceTemporal,
                           const std::string& indiceDestino,
                           std::string& error) {
    std::error_code ec;
    const auto limpiar = [&](const fs::path& ruta) {
        std::error_code fallo;
        fs::remove(ruta, fallo);
        if (fallo) error += " No se pudo limpiar " + ruta.string() + ": " + fallo.message();
    };
    const bool habiaMapeo = fs::exists(mapeoDestino, ec);
    if (ec) {
        error = "No se puede inspeccionar mapeo anterior: " + ec.message();
        limpiar(mapeoTemporal); limpiar(indiceTemporal); return false;
    }
    fs::path respaldo;
    if (habiaMapeo) {
        std::string plantilla = mapeoDestino + ".bak.XXXXXX";
        std::vector<char> nombre(plantilla.begin(), plantilla.end());
        nombre.push_back('\0');
        const int descriptor = mkstemp(nombre.data());
        if (descriptor < 0) {
            error = "No se puede reservar respaldo del mapeo: " + std::string(std::strerror(errno));
            limpiar(mapeoTemporal); limpiar(indiceTemporal); return false;
        }
        respaldo = nombre.data();
        if (close(descriptor) != 0) {
            error = "No se puede cerrar respaldo del mapeo: " + std::string(std::strerror(errno));
            limpiar(respaldo); limpiar(mapeoTemporal); limpiar(indiceTemporal); return false;
        }
        fs::remove(respaldo, ec);
        if (ec) {
            error = "No se puede preparar respaldo del mapeo: " + ec.message();
            limpiar(mapeoTemporal); limpiar(indiceTemporal); return false;
        }
        fs::create_hard_link(mapeoDestino, respaldo, ec);
        if (ec) {
            error = "No se puede respaldar mapeo anterior: " + ec.message();
            limpiar(mapeoTemporal); limpiar(indiceTemporal); return false;
        }
    }
    fs::rename(mapeoTemporal, mapeoDestino, ec);
    if (ec) {
        error = "No se puede reemplazar mapeo: " + ec.message();
        limpiar(mapeoTemporal); limpiar(indiceTemporal);
        if (habiaMapeo) limpiar(respaldo);
        return false;
    }
    fs::rename(indiceTemporal, indiceDestino, ec);
    if (ec) {
        error = "No se puede reemplazar indice: " + ec.message();
        std::error_code rollback;
        if (habiaMapeo) fs::rename(respaldo, mapeoDestino, rollback);
        else fs::remove(mapeoDestino, rollback);
        if (rollback) error += " Rollback del mapeo fallo: " + rollback.message() +
                               (habiaMapeo ? "; respaldo: " + respaldo.string() : "");
        limpiar(indiceTemporal);
        return false;
    }
    if (habiaMapeo) {
        fs::remove(respaldo, ec);
        if (ec) {
            error = "Indice publicado, pero no se pudo limpiar respaldo: " + ec.message();
            return false;
        }
    }
    return true;
}

void crearIndiceInvertido(const Config& cfg) {
    limpiarPantalla();
    std::cout << "======================================\n"
                 "      CREACION DE INDICE INVERTIDO\n"
                 "======================================\n";
    std::string error;
    int numeroThreads = 0;
    if (!validarConfigIndice(cfg, error, &numeroThreads)) {
        std::cout << "[ERROR] Configuracion del indice: " << error << '\n';
        pausar(); return;
    }
    try {
        std::vector<Libro> libros;
        if (!descubrirLibros(cfg.librosDir, libros, error)) {
            std::cout << "[ERROR] " << error << '\n'; pausar(); return;
        }
        const size_t limite = std::min<size_t>(256, std::max<size_t>(4, libros.size() * 4));
        if (static_cast<size_t>(numeroThreads) > limite) {
            std::cout << "[ERROR] N_THREADS=" << numeroThreads << " excede limite " << limite
                      << " para " << libros.size() << " libros.\n";
            pausar(); return;
        }
        Indice indice;
        if (!ejecutarWorkers(libros, numeroThreads, indice, error) ||
            !escribirResultados(cfg, libros, indice, error)) {
            std::cout << "[ERROR] " << error << '\n'; pausar(); return;
        }
        std::cout << "Utilizando " << numeroThreads << " threads; " << libros.size()
                  << " libros procesados.\nTarea finalizada con éxito.\n";
    } catch (const std::exception& e) {
        std::cout << "[ERROR] No se pudo crear indice: " << e.what() << '\n';
    }
    pausar("Presione ENTER para volver...");
}
