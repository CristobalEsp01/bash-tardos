// =============================================================================
//  cliente.cpp - Cliente del juego BAJO/MEDIO/ALTO por sockets TCP.
//
//  - Se conecta a HOST:PORT (entorno del proceso > .env > valores por defecto).
//  - Pide el nombre por stdin, lo envía como primera línea.
//  - Bucle con select(): lee de stdin y del socket.
//      * Línea del usuario -> se envía al servidor.
//      * Pantalla del servidor (hasta "<<<FIN>>>") -> limpiarPantalla + imprimir.
//  - Cuando el servidor cierra la conexión, el cliente termina.
//
//  Compilar:
//    g++ -std=c++17 -Wall -Wextra -pthread cliente.cpp -o cliente
//
//  Ejecutar:
//    HOST=127.0.0.1 PORT=5000 ./bin/cliente
//
//  Protocolo (recordatorio):
//    Cliente -> servidor: primer mensaje = nombre; luego líneas de acción.
//    Servidor -> cliente: <pantalla>\n<<<FIN>>>\n
// =============================================================================
#include "config.h"
#include "utils.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

static const char* FIN_PANTALLA = "\n<<<FIN>>>\n";

// Conecta a host:puerto. Devuelve fd o -1.
static int conectar(const std::string& host, int puerto) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { std::perror("socket"); return -1; }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(puerto));
    if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
        std::cerr << "ERROR: HOST inválida: " << host << "\n";
        ::close(fd);
        return -1;
    }
    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::perror("connect");
        ::close(fd);
        return -1;
    }
    return fd;
}

// Envía un texto + '\n' por el socket.
static bool enviar_linea(int fd, const std::string& linea) {
    std::string m = linea + "\n";
    size_t off = 0;
    while (off < m.size()) {
        ssize_t n = ::send(fd, m.data() + off, m.size() - off, MSG_NOSIGNAL);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        off += static_cast<size_t>(n);
    }
    return true;
}

int main() {
    Config cfg = cargarConfiguracion();
    std::string error;
    if (!validarConfigCliente(cfg, error)) {
        std::cerr << "ERROR: " << error << "\n";
        return 1;
    }
    const std::string& host = cfg.host;
    int puerto = cfg.port;

    int fd = conectar(host, puerto);
    if (fd < 0) return 1;

    // Entrada por descriptor: nombre y acciones comparten el mismo buffer.
    limpiarPantalla();
    std::cout << "Conectado a " << host << ":" << puerto << "\n"
              << "Ingresa tu nombre: " << std::flush;

    // Acumulador de la pantalla actual
    std::string buf;

    // Imprime todo lo que haya hasta FIN_PANTALLA. Si llega una pantalla completa,
    // limpia la terminal y la muestra. Devuelve false si el socket se cerró.
    auto procesar_buffer = [&]() -> bool {
        while (true) {
            auto pos = buf.find(FIN_PANTALLA);
            if (pos == std::string::npos) return true;     // falta más
            std::string pantalla = buf.substr(0, pos);
            buf.erase(0, pos + std::strlen(FIN_PANTALLA));
            limpiarPantalla();
            std::cout << pantalla << '\n' << std::flush;
        }
    };

    char tmp[1024];
    bool fin = false;
    bool entrada_activa = true;
    bool nombre_enviado = false;
    std::string entrada;
    while (!fin) {
        fd_set rfds;
        FD_ZERO(&rfds);
        if (entrada_activa) FD_SET(STDIN_FILENO, &rfds);
        FD_SET(fd, &rfds);
        int maxfd = entrada_activa ? std::max(STDIN_FILENO, fd) : fd;

        int sel = ::select(maxfd + 1, &rfds, nullptr, nullptr, nullptr);
        if (sel < 0) {
            if (errno == EINTR) continue;
            std::perror("select");
            break;
        }

        // Datos del servidor
        if (FD_ISSET(fd, &rfds)) {
            ssize_t n = ::recv(fd, tmp, sizeof(tmp), 0);
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) {
                fin = true;
            } else {
                buf.append(tmp, static_cast<size_t>(n));
                if (buf.size() > 1024 * 1024) {
                    std::cerr << "ERROR: pantalla demasiado larga\n";
                    fin = true;
                } else if (!procesar_buffer()) fin = true;
            }
        }

        // Entrada del usuario
        if (!fin && entrada_activa && FD_ISSET(STDIN_FILENO, &rfds)) {
            ssize_t n = ::read(STDIN_FILENO, tmp, sizeof(tmp));
            if (n > 0) {
                entrada.append(tmp, static_cast<size_t>(n));
                if (entrada.size() > 4096) {
                    std::cerr << "ERROR: entrada demasiado larga\n";
                    fin = true;
                }
            } else if (n == 0) {
                entrada_activa = false;
            } else if (errno != EINTR && errno != EAGAIN) {
                std::perror("read");
                fin = true;
            }
            while (!fin) {
                size_t pos = entrada.find('\n');
                if (pos == std::string::npos && (entrada_activa || entrada.empty())) break;
                std::string linea = entrada.substr(0, pos);
                entrada.erase(0, pos == std::string::npos ? entrada.size() : pos + 1);
                if (!linea.empty() && linea.back() == '\r') linea.pop_back();
                if (!enviar_linea(fd, linea)) fin = true;
                nombre_enviado = true;
            }
            if (!entrada_activa && !nombre_enviado) fin = true;
        }
    }

    ::shutdown(fd, SHUT_RDWR);
    ::close(fd);

    std::cout << "\n[Desconectado del servidor]\n";
    return 0;
}
