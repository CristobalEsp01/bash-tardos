#ifndef CONFIG_H
#define CONFIG_H

#include <string>

// ---------------------------------------------------------------------
// Configuracion: entorno del proceso > .env > defecto (solo opcionales).
// Campos vacios de tareas concretas se validan antes de ejecutarlas.
// ---------------------------------------------------------------------
struct Config {
    std::string userFile   = "data/USUARIOS.txt";  // USER_FILE
    std::string perfilFile = "data/PERFILES.txt";  // PERFIL_FILE
    std::string adminBin   = "bin/admin";          // ADMIN_BIN
    std::string multiBin   = "bin/multi";          // MULTI_BIN
    std::string servidorBin = "bin/servidor";       // JUEGO_SERVIDOR_BIN
    std::string clienteBin  = "bin/cliente";        // JUEGO_CLIENTE_BIN
    std::string librosDir  = "data/LIBROS";        // LIBROS_DIR
    std::string adminPerfil = "ADMIN";             // ADMIN_PERFIL

    std::string nThreads;                           // N_THREADS (requerido para indice)
    std::string indiceInvertidoPath;                 // INDICE_INVERTIDO_PATH
    std::string mapeoLibrosPath;                     // MAPEO_LIBROS_PATH
    std::string logFile;                             // LOG_FILE (requerido para servidor)
    std::string host = "127.0.0.1";                 // HOST, IPv4
    std::string bindAddr = "0.0.0.0";               // BIND_ADDR, IPv4
    int port = 5000;                                // PORT
    std::string portError;                          // error diferido hasta opcion 3

    bool huboAdvertencias = false;
};

// Carga la configuracion. Prioridad: variable de entorno del sistema
// (getenv) > valor en el archivo .env > valor por defecto.
// Un valor vacio explicito reemplaza al de .env; validadores lo rechazan
// cuando sea requerido por la tarea solicitada.
Config cargarConfiguracion(const std::string& rutaEnv = ".env");

bool validarConfigIndice(const Config& cfg, std::string& error, int* numeroThreads = nullptr);
bool validarConfigServidor(const Config& cfg, std::string& error);
bool validarConfigCliente(const Config& cfg, std::string& error);

#endif
