// =============================================================================
//  cliente.cpp - Cliente del juego BAJO/MEDIO/ALTO por sockets TCP.
//
//  - Se conecta a HOST:PORT (env vars, defaults 127.0.0.1:5000).
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
//    HOST=127.0.0.1 PORT=5000 ./cliente
//
//  Protocolo (recordatorio):
//    Cliente -> servidor: primer mensaje = nombre; luego líneas de acción.
//    Servidor -> cliente: <pantalla>\n<<<FIN>>>\n
// =============================================================================
#include "../include/utils.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

static const char* FIN_PANTALLA = "<<<FIN>>>";

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
    const char* host_env = std::getenv("HOST");
    std::string host = host_env ? host_env : "127.0.0.1";
    const char* port_env = std::getenv("PORT");
    int puerto = port_env ? std::atoi(port_env) : 5000;

    int fd = conectar(host, puerto);
    if (fd < 0) return 1;

    // Nombre del jugador
    limpiarPantalla();
    std::cout << "Conectado a " << host << ":" << puerto << "\n"
              << "Ingresa tu nombre: " << std::flush;
    std::string nombre;
    if (!std::getline(std::cin, nombre)) { ::close(fd); return 0; }
    if (!enviar_linea(fd, nombre)) { ::close(fd); return 0; }

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
            // recortar el '\n' inmediatamente anterior al delimitador
            if (!pantalla.empty() && pantalla.back() == '\n') pantalla.pop_back();
            limpiarPantalla();
            std::cout << pantalla << std::flush;
        }
    };

    char tmp[1024];
    bool fin = false;
    while (!fin) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds);
        FD_SET(fd, &rfds);
        int maxfd = std::max(STDIN_FILENO, fd);

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
                if (!procesar_buffer()) fin = true;
            }
        }

        // Entrada del usuario
        if (!fin && FD_ISSET(STDIN_FILENO, &rfds)) {
            std::string linea;
            if (!std::getline(std::cin, linea)) {
                // EOF en stdin: cerramos
                fin = true;
            } else {
                if (!enviar_linea(fd, linea)) fin = true;
            }
        }
    }

    ::shutdown(fd, SHUT_RDWR);
    ::close(fd);

    std::cout << "\n[Desconectado del servidor]\n";
    return 0;
}