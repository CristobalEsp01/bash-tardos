// =============================================================================
//  servidor.cpp - Servidor del juego BAJO/MEDIO/ALTO por sockets TCP.
//
//  - Un hilo por cliente. Estado global `Juego` protegido por mutex.
//  - Difusión desde snapshot (patrón mutex + copia, ver juego.h).
//  - LOG a archivo cuya ruta viene de LOG_FILE (getenv). Si falta, aborta (2.11).
//  - Detección de desconexión: recv() == 0 o error -> remover_jugador + LOG (2.13).
//  - Cierre controlado al terminar la partida: shutdown+close de sockets,
//    join de hilos, flush+close del log (2.14).
//
//  Compilar:
//    g++ -std=c++17 -Wall -Wextra -pthread servidor.cpp -o servidor
//
//  Ejecutar:
//    LOG_FILE=/ruta/juego.log PORT=5000 BIND_ADDR=0.0.0.0 ./servidor
//
//  Protocolo:
//    Cliente -> servidor: primer mensaje = nombre; luego líneas de acción.
//    Servidor -> cliente: <pantalla>\n<<<FIN>>>\n
// =============================================================================
#include "juego.h"
#include "../include/utils.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// ------------------------------- LOG -----------------------------------------
class Log {
public:
    explicit Log(const std::string& ruta) : out_(ruta, std::ios::app) {
        if (!out_) { std::cerr << "ERROR: no se pudo abrir LOG_FILE=" << ruta << "\n"; std::abort(); }
    }
    void escribir(const std::string& jugador, const std::string& equipo,
                  const std::string& accion, const std::string& detalle = "") {
        std::lock_guard<std::mutex> lk(m_);
        char fecha[32];
        std::time_t t = std::time(nullptr);
        std::tm tm{};
        localtime_r(&t, &tm);
        std::strftime(fecha, sizeof(fecha), "%Y-%m-%d %H:%M:%S", &tm);
        out_ << fecha << ';' << jugador << ';' << equipo << ';' << accion;
        if (!detalle.empty()) out_ << ';' << detalle;
        out_ << '\n';
        out_.flush();
    }
    void cerrar() { std::lock_guard<std::mutex> lk(m_); out_.flush(); out_.close(); }
private:
    std::ofstream out_;
    std::mutex m_;
};

// --------------------------- estado compartido -------------------------------
struct Estado {
    Juego juego;
    std::mutex m;
};

static Log* g_log = nullptr;
static std::mutex g_fds_m;                          // protege g_fds
static std::unordered_map<int, int> g_fds;          // id -> fd
static std::atomic<bool> g_terminado{false};

static std::string nombre_equipo(int e) { return e == 0 ? "EquipoA" : "EquipoB"; }

// ------------------------------ envío ----------------------------------------
static const char* FIN_PANTALLA = "\n<<<FIN>>>\n";

static void enviar_texto(int fd, const std::string& texto) {
    size_t off = 0;
    while (off < texto.size()) {
        ssize_t n = ::send(fd, texto.data() + off, texto.size() - off, MSG_NOSIGNAL);
        if (n < 0 && errno == EINTR) continue;       // reintentar
        if (n <= 0) return;
        off += static_cast<size_t>(n);
    }
}

static void enviar_pantalla(int fd, int id, const Juego& snap) {
    enviar_texto(fd, pantalla_para(snap, id) + FIN_PANTALLA);
}

static void enviar_mensaje(int fd, const std::string& texto) {
    enviar_texto(fd, texto + "\n" + FIN_PANTALLA);
}

// Difunde a todos los conectados. `g_fds` bajo lock SOLO para leer una copia.
static void difundir(const Juego& snap) {
    std::unordered_map<int, int> fds;
    { std::lock_guard<std::mutex> lk(g_fds_m); fds = g_fds; }
    for (int id : ids_conectados(snap)) {
        auto it = fds.find(id);
        if (it != fds.end()) enviar_pantalla(it->second, id, snap);
    }
}

// --------------------------- LOG helpers -------------------------------------
static std::string nombre_y_equipo_de(const Juego& snap, int id, std::string& eq) {
    for (const auto& p : snap.jugadores) {
        if (p.id == id) {
            eq = p.equipo >= 0 ? nombre_equipo(p.equipo) : "";
            return p.nombre;
        }
    }
    eq.clear();
    return "";
}

// Registra en el log las transiciones entre dos snapshots (antes -> después).
static void log_transiciones(const Juego& antes, const Juego& despues) {
    // INICIO_PARTIDA
    if (antes.estado == ESPERANDO && despues.estado == ACTIVO)
        g_log->escribir("", "", "INICIO_PARTIDA",
                        "rondas=" + std::to_string(despues.max_rondas));

    // Cambio de turno (solo si sigue ACTIVO y cambió el turno)
    if (despues.estado == ACTIVO && antes.turno_id != despues.turno_id) {
        std::string eq;
        std::string nom = nombre_y_equipo_de(despues, despues.turno_id, eq);
        g_log->escribir(nom, eq, "TURNO", "ronda=" + std::to_string(despues.ronda_actual));
    }

    // Ronda resuelta: ronda_actual aumentó o estado pasó a TERMINADO habiendo jugado
    if (antes.ronda_actual != despues.ronda_actual ||
        (antes.estado == ACTIVO && despues.estado == TERMINADO)) {
        // Puntajes individuales de esta ronda (ronda_actual antes de incrementar)
        for (const auto& p : despues.jugadores) {
            const Jugador* pa = nullptr;
            for (const auto& q : antes.jugadores) if (q.id == p.id) { pa = &q; break; }
            if (!pa || pa->jugada == SIN_JUGADA) continue;
            std::string eq = p.equipo >= 0 ? nombre_equipo(p.equipo) : "";
            int ganados = p.puntaje - pa->puntaje;
            std::string det = std::string(detalle::NOMBRE_JUGADA[pa->jugada]) +
                              "->" + std::to_string(ganados) + "pts";
            g_log->escribir(p.nombre, eq, "RONDA_RESUELTA", det);
        }
    }

    // FIN_PARTIDA
    if (antes.estado != TERMINADO && despues.estado == TERMINADO) {
        std::string res = despues.ganador == -1 ? "EMPATE" : nombre_equipo(despues.ganador);
        g_log->escribir("", "", "FIN_PARTIDA", res);
    }
}

// --------------------------- manejadores -------------------------------------
static void manejar_conexion(Estado& est, int id, const std::string& nombre) {
    Resultado r; Juego antes, despues;
    {
        std::lock_guard<std::mutex> lk(est.m);
        antes = est.juego;
        r = agregar_jugador(est.juego, id, nombre);
        despues = est.juego;
    }
    if (!r.ok) {
        int fd; { std::lock_guard<std::mutex> lk(g_fds_m); fd = g_fds.count(id) ? g_fds[id] : -1; }
        if (fd >= 0) enviar_mensaje(fd, "ERROR: " + r.mensaje);
        g_log->escribir(nombre, "", "ERROR_CONEXION", r.mensaje);
        return;
    }
    {
        std::string eq;
        std::string nom = nombre_y_equipo_de(despues, id, eq);
        g_log->escribir(nom, eq, "CONECTA");
    }
    log_transiciones(antes, despues);
    difundir(despues);

    if (juego_terminado(despues)) g_terminado = true;
}

static void manejar_mensaje(Estado& est, int id, const std::string& texto) {
    Resultado r; Juego antes, despues;
    {
        std::lock_guard<std::mutex> lk(est.m);
        antes = est.juego;
        r = procesar_accion(est.juego, id, texto);
        despues = est.juego;
    }
    if (!r.ok) {
        int fd; { std::lock_guard<std::mutex> lk(g_fds_m); fd = g_fds.count(id) ? g_fds[id] : -1; }
        if (fd >= 0) enviar_mensaje(fd, "ERROR: " + r.mensaje);
        return;
    }

    // Acción aceptada: loguear según el tipo
    std::string eq;
    std::string nom = nombre_y_equipo_de(despues, id, eq);
    std::string norm = detalle::normalizar(texto);

    if (norm.rfind("rondas", 0) == 0) {
        g_log->escribir(nom, eq, "RONDAS", std::to_string(despues.max_rondas));
    } else {
        // Jugada: el snapshot ANTES tiene al jugador con jugada == SIN_JUGADA;
        // el DESPUÉS puede tenerlo con jugada o, si resolvió la ronda, ya reseteada.
        // Recuperamos lo que eligió el jugador del texto normalizado.
        TipoJugada t = (norm == "1" || norm == "bajo")  ? BAJO
                     : (norm == "2" || norm == "medio") ? MEDIO
                     : (norm == "3" || norm == "alto")  ? ALTO : SIN_JUGADA;
        g_log->escribir(nom, eq, "ELIGE", detalle::NOMBRE_JUGADA[t]);
    }

    log_transiciones(antes, despues);
    difundir(despues);

    if (juego_terminado(despues)) g_terminado = true;
}

static void manejar_desconexion(Estado& est, int id) {
    Resultado r; Juego antes, despues;
    {
        std::lock_guard<std::mutex> lk(est.m);
        antes = est.juego;
        r = remover_jugador(est.juego, id);
        despues = est.juego;
    }
    if (!r.ok) return;

    std::string eq;
    std::string nom = nombre_y_equipo_de(antes, id, eq);
    g_log->escribir(nom, eq, "DESCONECTA");

    log_transiciones(antes, despues);
    difundir(despues);

    if (juego_terminado(despues)) g_terminado = true;
}

// --------------------------- hilo por cliente --------------------------------
static void atender_cliente(int fd, int id, Estado& est) {
    {
        std::lock_guard<std::mutex> lk(g_fds_m);
        g_fds[id] = fd;
    }

    std::string buf;
    char tmp[512];

    auto leer_linea = [&](std::string& out) -> bool {
        while (true) {
            auto pos = buf.find('\n');
            if (pos != std::string::npos) {
                out = buf.substr(0, pos);
                buf.erase(0, pos + 1);
                if (!out.empty() && out.back() == '\r') out.pop_back();
                return true;
            }
            ssize_t n = ::recv(fd, tmp, sizeof(tmp), 0);
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) return false;
            buf.append(tmp, static_cast<size_t>(n));
        }
    };

    std::string nombre;
    if (!leer_linea(nombre)) {
        std::lock_guard<std::mutex> lk(g_fds_m);
        g_fds.erase(id);
        ::shutdown(fd, SHUT_RDWR);
        ::close(fd);
        return;
    }

    manejar_conexion(est, id, nombre);

    std::string linea;
    while (!g_terminado && leer_linea(linea)) {
        manejar_mensaje(est, id, linea);
    }

    // Si terminó la partida, el cliente sigue leyendo el FIN hasta que cerremos.
    // Damos un margen para que llegue el último mensaje; no hace falta más.
    {
        std::lock_guard<std::mutex> lk(g_fds_m);
        g_fds.erase(id);
    }
    manejar_desconexion(est, id);

    ::shutdown(fd, SHUT_RDWR);
    ::close(fd);
}

// -------------------------------- main ---------------------------------------
int main() {
    const char* logfile = std::getenv("LOG_FILE");
    if (!logfile || !*logfile) {
        std::cerr << "ERROR: falta la variable de entorno LOG_FILE\n";
        return 1;
    }
    Log log(logfile);
    g_log = &log;

    const char* port_env = std::getenv("PORT");
    int puerto = port_env ? std::atoi(port_env) : 5000;
    const char* bind_env = std::getenv("BIND_ADDR");
    std::string bind_addr = bind_env ? bind_env : "0.0.0.0";

    int srv = ::socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) { std::perror("socket"); return 1; }
    int opt = 1;
    ::setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(puerto));
    if (::inet_pton(AF_INET, bind_addr.c_str(), &addr.sin_addr) != 1) {
        std::cerr << "ERROR: BIND_ADDR inválida: " << bind_addr << "\n";
        return 1;
    }
    if (::bind(srv, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::perror("bind");
        ::close(srv);
        return 1;
    }
    if (::listen(srv, 16) < 0) {
        std::perror("listen");
        ::close(srv);
        return 1;
    }

    std::cout << "[servidor] escuchando en " << bind_addr << ":" << puerto
              << " (LOG_FILE=" << logfile << ")\n" << std::flush;

    Estado est;
    std::vector<std::thread> hilos;
    std::atomic<int> next_id{1};

    // Bucle de aceptación: mientras no haya terminado la partida.
    while (!g_terminado) {
        sockaddr_in cli{};
        socklen_t cli_len = sizeof(cli);
        int fd = ::accept(srv, reinterpret_cast<sockaddr*>(&cli), &cli_len);
        if (fd < 0) {
            if (errno == EINTR) continue;
            std::perror("accept");
            break;
        }
        int id = next_id++;
        hilos.emplace_back(atender_cliente, fd, id, std::ref(est));
    }

    // Cierre controlado (2.14): al terminar, cerramos el socket de escucha
    // y hacemos shutdown de todos los clientes para desbloquear sus recv().
    ::close(srv);
    {
        std::lock_guard<std::mutex> lk(g_fds_m);
        for (auto& kv : g_fds) ::shutdown(kv.second, SHUT_RDWR);
    }
    for (auto& h : hilos) if (h.joinable()) h.join();
    log.cerrar();

    std::cout << "[servidor] cerrado.\n";
    return 0;
}