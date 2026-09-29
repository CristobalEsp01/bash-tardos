#ifndef CONFIG_H
#define CONFIG_H

#include <string>

// ---------------------------------------------------------------------
// Configuracion del sistema leida desde el archivo .env
// (todas las variables relevantes viven ahi, ver README)
// ---------------------------------------------------------------------
struct Config {
    std::string userFile   = "data/USUARIOS.txt";  // USER_FILE
    std::string perfilFile = "data/PERFILES.txt";  // PERFIL_FILE
    std::string adminBin   = "bin/admin";          // ADMIN_BIN
    std::string multiBin   = "bin/multi";          // MULTI_BIN
    std::string librosDir  = "data/LIBROS";        // LIBROS_DIR
    std::string adminPerfil = "ADMIN";             // ADMIN_PERFIL
};

// Carga la configuracion. Prioridad: variable de entorno del sistema
// (getenv) > valor en el archivo .env > valor por defecto.
// Si alguna variable no esta definida muestra una advertencia.
Config cargarConfiguracion(const std::string& rutaEnv = ".env");

#endif
