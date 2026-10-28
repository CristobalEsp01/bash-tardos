#include "config.h"
#include "juego.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

constexpr size_t MAX_LINEA = 128;
constexpr size_t MAX_SALIDA = 256 * 1024;
constexpr int MAX_ACCEPT_POR_CICLO = 16;
constexpr size_t MAX_CLIENTES_EN_MEMORIA = 16;
constexpr auto ESPERA_NOMBRE = std::chrono::seconds(5);
constexpr const char* FIN_PANTALLA = "<<<FIN>>>\n";

const char* estado_nombre(EstadoJuego e) {
    return e == ESPERANDO ? "ESPERANDO" : e == ACTIVO ? "ACTIVO" : "TERMINADO";
}

std::string equipo_nombre(int e) {
    return e < 0 ? "-" : e == 0 ? "EquipoA" : "EquipoB";
}

class Log {
public:
    explicit Log(const std::string& ruta) : out_(ruta, std::ios::app) {}
    bool valido() const { return out_.is_open(); }
    bool correcto() const { return out_.is_open() && !fallido_; }

    void escribir(const std::string& jugador, const std::string& equipo,
                  const std::string& evento, const std::string& info = "") {
        if (fallido_) return;
        std::time_t ahora = std::time(nullptr);
        std::tm local{};
        localtime_r(&ahora, &local);
        char fecha[32];
        std::strftime(fecha, sizeof(fecha), "%Y-%m-%d %H:%M:%S", &local);
        out_ << fecha << ';' << jugador << ';' << equipo << ';' << evento;
        if (!info.empty()) out_ << ';' << info;
        out_ << '\n';
        out_.flush();
        if (!out_) fallido_ = true;
    }

private:
    std::ofstream out_;
    bool fallido_ = false;
};

struct Cliente {
    int fd = -1;
    int id = -1;
    bool registrado = false;
    bool cerrar_al_enviar = false;
    std::chrono::steady_clock::time_point aceptado;
    std::string entrada;
    std::string salida;
};

class Servidor {
public:
    Servidor(int escucha, Log& log) : escucha_(escucha), log_(log) {}
    ~Servidor() {
        if (escucha_ >= 0) ::close(escucha_);
        for (auto& kv : clientes_) ::close(kv.first);
    }

    bool ejecutar() {
        bool correcto = true;
        while (escucha_ >= 0 || !clientes_.empty()) {
            if (!log_.correcto()) break;
            if (terminando_ && std::chrono::steady_clock::now() >= cierre_limite_) {
                std::vector<int> restantes;
                for (const auto& kv : clientes_) restantes.push_back(kv.first);
                for (int fd : restantes) desconectar(fd);
                break;
            }
            auto ahora = std::chrono::steady_clock::now();
            for (auto& kv : clientes_) {
                Cliente& c = kv.second;
                if (!c.registrado && !c.cerrar_al_enviar &&
                    c.entrada.find('\n') == std::string::npos &&
                    ahora - c.aceptado >= ESPERA_NOMBRE) {
                    error(c, "Tiempo de espera para nombre agotado.");
                    c.cerrar_al_enviar = true;
                }
            }
            std::vector<int> cerrar;
            for (const auto& kv : clientes_)
                if (kv.second.cerrar_al_enviar && kv.second.salida.empty()) cerrar.push_back(kv.first);
            for (int fd : cerrar) desconectar(fd);

            std::vector<pollfd> fds;
            if (escucha_ >= 0) fds.push_back({escucha_, POLLIN, 0});
            for (const auto& kv : clientes_) {
                short eventos = kv.second.cerrar_al_enviar ? 0 : POLLIN;
                if (!kv.second.salida.empty()) eventos |= POLLOUT;
                fds.push_back({kv.first, eventos, 0});
            }
            int n = ::poll(fds.data(), fds.size(), 100);
            if (n < 0) {
                if (errno == EINTR) continue;
                std::perror("poll");
                correcto = false;
                break;
            }
            if (n == 0) continue;

            for (const pollfd& p : fds) {
                if (p.revents == 0) continue;
                if (p.fd == escucha_) {
                    if (p.revents & POLLIN) aceptar();
                    continue;
                }
                auto it = clientes_.find(p.fd);
                if (it == clientes_.end()) continue;
                if (p.revents & (POLLERR | POLLNVAL)) {
                    desconectar(p.fd);
                    continue;
                }
                if ((p.revents & POLLIN) && !it->second.cerrar_al_enviar) recibir(p.fd);
                it = clientes_.find(p.fd);
                if (it == clientes_.end()) continue;
                if ((p.revents & POLLOUT) && !it->second.salida.empty()) enviar(p.fd);
                it = clientes_.find(p.fd);
                if (it == clientes_.end()) continue;
                if ((p.revents & POLLHUP) && !(p.revents & POLLIN) &&
                    !it->second.cerrar_al_enviar) {
                    desconectar(p.fd);
                    continue;
                }
                if (it->second.cerrar_al_enviar && it->second.salida.empty())
                    desconectar(p.fd);
            }
            std::vector<std::pair<int, int>> pendientes;
            for (const auto& kv : clientes_) pendientes.emplace_back(kv.second.id, kv.first);
            std::sort(pendientes.begin(), pendientes.end());
            for (const auto& p : pendientes) procesar_entrada(p.second);
        }
        std::vector<int> restantes;
        for (const auto& kv : clientes_) restantes.push_back(kv.first);
        for (int fd : restantes) desconectar(fd);
        if (!log_.correcto()) {
            std::cerr << "ERROR: fallo al escribir LOG_FILE\n";
            correcto = false;
        }
        return correcto;
    }

private:
    int escucha_;
    Log& log_;
    Juego juego_;
    std::map<int, Cliente> clientes_;
    int siguiente_id_ = 1;
    bool terminando_ = false;
    std::chrono::steady_clock::time_point cierre_limite_{};

    void iniciar_cierre() {
        if (terminando_) return;
        terminando_ = true;
        cierre_limite_ = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        if (escucha_ >= 0) {
            ::close(escucha_);
            escucha_ = -1;
        }
        for (auto& kv : clientes_) kv.second.cerrar_al_enviar = true;
    }

    static void poner_no_bloqueante(int fd) {
        int flags = ::fcntl(fd, F_GETFL, 0);
        if (flags >= 0) ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }

    void pantalla(Cliente& c, const std::string& texto) {
        if (c.salida.size() + texto.size() + 12 > MAX_SALIDA) {
            c.cerrar_al_enviar = true;
            c.salida.clear();
            return;
        }
        c.salida += texto;
        if (c.salida.empty() || c.salida.back() != '\n') c.salida += '\n';
        c.salida += FIN_PANTALLA;
    }

    void error(Cliente& c, const std::string& texto) {
        pantalla(c, "ERROR: " + texto);
    }

    void difundir() {
        for (auto& kv : clientes_) {
            Cliente& c = kv.second;
            if (c.registrado && !c.cerrar_al_enviar)
                pantalla(c, pantalla_para(juego_, c.id));
        }
    }

    void registrar_cambio(const Juego& antes) {
        if (antes.estado == ESPERANDO && juego_.estado == ACTIVO) {
            for (const Jugador& p : juego_.jugadores)
                log_.escribir(p.nombre, equipo_nombre(p.equipo), "ASIGNA_EQUIPO");
            log_.escribir("-", "-", "INICIO_PARTIDA",
                          "rondas=" + std::to_string(juego_.max_rondas));
        }

        bool ronda_resuelta = antes.estado == ACTIVO &&
            (antes.ronda_actual != juego_.ronda_actual || juego_.estado == TERMINADO);
        if (ronda_resuelta && juego_.desconectado_id == -1) {
            for (const Jugador& p : juego_.jugadores) {
                const Jugador* previo = detalle::buscar(antes, p.id);
                int ganados = p.puntaje - (previo ? previo->puntaje : 0);
                log_.escribir(p.nombre, equipo_nombre(p.equipo), "RONDA_RESUELTA",
                              "ronda=" + std::to_string(antes.ronda_actual) +
                              ",puntos=" + std::to_string(ganados) +
                              ",total=" + std::to_string(p.puntaje) +
                              "," + p.ultima_ronda);
            }
        }
        if (antes.estado != juego_.estado) {
            log_.escribir("-", "-", "ESTADO",
                          std::string(estado_nombre(antes.estado)) + "->" +
                          estado_nombre(juego_.estado));
        }
        if (juego_.estado == ACTIVO &&
            (antes.turno_id != juego_.turno_id || antes.ronda_actual != juego_.ronda_actual)) {
            const Jugador* p = detalle::buscar(juego_, juego_.turno_id);
            if (p) log_.escribir(p->nombre, equipo_nombre(p->equipo), "TURNO",
                                 "ronda=" + std::to_string(juego_.ronda_actual));
        }
        if (antes.estado != TERMINADO && juego_.estado == TERMINADO) {
            std::string ganador = juego_.ganador == -1 ? "EMPATE" : equipo_nombre(juego_.ganador);
            log_.escribir("-", "-", "FIN_PARTIDA",
                          "ganador=" + ganador +
                          ",EquipoA=" + std::to_string(detalle::puntos_equipo(juego_, 0)) +
                          ",EquipoB=" + std::to_string(detalle::puntos_equipo(juego_, 1)));
        }
    }

    void aceptar() {
        for (int atendidos = 0; atendidos < MAX_ACCEPT_POR_CICLO; ++atendidos) {
            int fd = ::accept(escucha_, nullptr, nullptr);
            if (fd < 0) {
                if (errno == EINTR) continue;
                if (errno != EAGAIN && errno != EWOULDBLOCK) std::perror("accept");
                return;
            }
            poner_no_bloqueante(fd);
            if (clientes_.size() >= MAX_CLIENTES_EN_MEMORIA) {
                ::close(fd);
                continue;
            }
            int plazas = 0;
            for (const auto& kv : clientes_)
                if (!kv.second.cerrar_al_enviar) ++plazas;
            Cliente c;
            c.fd = fd;
            c.id = siguiente_id_++;
            c.aceptado = std::chrono::steady_clock::now();
            if (plazas >= JUGADORES_NECESARIOS || juego_.estado != ESPERANDO) {
                error(c, "La partida está llena o ya empezó.");
                c.cerrar_al_enviar = true;
                log_.escribir("-", "-", "ERROR_CONEXION", "sala llena");
            }
            clientes_.emplace(fd, std::move(c));
        }
    }

    void recibir(int fd) {
        char buf[1024];
        ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
        if (n < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) return;
            desconectar(fd);
            return;
        }
        if (n == 0) {
            desconectar(fd);
            return;
        }
        auto it = clientes_.find(fd);
        if (it == clientes_.end()) return;
        Cliente& c = it->second;
        c.entrada.append(buf, static_cast<size_t>(n));
        if (c.entrada.size() > 4096) {
            error(c, "Demasiados datos pendientes.");
            c.cerrar_al_enviar = true;
        }
    }

    void procesar_entrada(int fd) {
        auto it = clientes_.find(fd);
        if (it == clientes_.end() || it->second.cerrar_al_enviar) return;
        Cliente& c = it->second;
        while (!terminando_) {
            if (!c.registrado) {
                bool anterior_pendiente = false;
                for (const auto& kv : clientes_)
                    if (kv.second.id < c.id && !kv.second.registrado &&
                        !kv.second.cerrar_al_enviar) anterior_pendiente = true;
                if (anterior_pendiente) break;
            }
            size_t pos = c.entrada.find('\n');
            if (pos == std::string::npos) {
                if (c.entrada.size() > MAX_LINEA) {
                    error(c, "Línea demasiado larga.");
                    c.cerrar_al_enviar = true;
                }
                break;
            }
            if (pos > MAX_LINEA) {
                error(c, "Línea demasiado larga.");
                c.cerrar_al_enviar = true;
                break;
            }
            std::string linea = c.entrada.substr(0, pos);
            c.entrada.erase(0, pos + 1);
            if (!linea.empty() && linea.back() == '\r') linea.pop_back();
            if (!c.registrado) {
                Juego antes = juego_;
                Resultado r = agregar_jugador(juego_, c.id, linea);
                if (!r.ok) {
                    error(c, r.mensaje);
                    c.cerrar_al_enviar = true;
                    break;
                }
                c.registrado = true;
                const Jugador* p = detalle::buscar(juego_, c.id);
                log_.escribir(p->nombre, equipo_nombre(p->equipo), "CONECTA");
                registrar_cambio(antes);
                difundir();
            } else {
                Juego antes = juego_;
                Resultado r = procesar_accion(juego_, c.id, linea);
                if (!r.ok) {
                    error(c, r.mensaje);
                } else {
                    const Jugador* p = detalle::buscar(juego_, c.id);
                    std::string cmd = detalle::normalizar(linea);
                    if (cmd.rfind("rondas", 0) == 0) {
                        log_.escribir(p->nombre, equipo_nombre(p->equipo), "RONDAS",
                                      std::to_string(juego_.max_rondas));
                    } else {
                        TipoJugada t = (cmd == "1" || cmd == "bajo") ? BAJO :
                                       (cmd == "2" || cmd == "medio") ? MEDIO : ALTO;
                        log_.escribir(p->nombre, equipo_nombre(p->equipo), "ELIGE",
                                      std::string(detalle::NOMBRE_JUGADA[t]) +
                                      ",ronda=" + std::to_string(antes.ronda_actual));
                    }
                    registrar_cambio(antes);
                    difundir();
                }
            }
            if (juego_terminado(juego_)) {
                iniciar_cierre();
                break;
            }
            if (c.cerrar_al_enviar) break;
        }
    }

    void enviar(int fd) {
        auto it = clientes_.find(fd);
        if (it == clientes_.end() || it->second.salida.empty()) return;
        std::string& s = it->second.salida;
        ssize_t n = ::send(fd, s.data(), s.size(), MSG_NOSIGNAL | MSG_DONTWAIT);
        if (n > 0) s.erase(0, static_cast<size_t>(n));
        else if (n < 0 && errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)
            desconectar(fd);
    }

    void desconectar(int fd) {
        auto it = clientes_.find(fd);
        if (it == clientes_.end()) return;
        Cliente c = std::move(it->second);
        clientes_.erase(it);
        ::close(fd);
        if (c.registrado) {
            Juego antes = juego_;
            const Jugador* p = detalle::buscar(juego_, c.id);
            std::string nombre = p ? p->nombre : "-";
            std::string equipo = p ? equipo_nombre(p->equipo) : "-";
            if (remover_jugador(juego_, c.id).ok) {
                log_.escribir(nombre, equipo, "DESCONECTA");
                if (antes.estado != TERMINADO) {
                    registrar_cambio(antes);
                    difundir();
                    if (juego_terminado(juego_)) iniciar_cierre();
                }
            }
        }
    }
};

}  // namespace

int main() {
    std::signal(SIGXFSZ, SIG_IGN);
    Config cfg = cargarConfiguracion();
    std::string error;
    if (!validarConfigServidor(cfg, error)) {
        std::cerr << "ERROR: " << error << '\n';
        return 1;
    }
    Log log(cfg.logFile);
    if (!log.valido()) {
        std::cerr << "ERROR: no se pudo abrir LOG_FILE=" << cfg.logFile << '\n';
        return 1;
    }

    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { std::perror("socket"); return 1; }
    int uno = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &uno, sizeof(uno));
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(cfg.port));
    if (::inet_pton(AF_INET, cfg.bindAddr.c_str(), &addr.sin_addr) != 1 ||
        ::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0 ||
        ::listen(fd, 16) != 0) {
        std::perror("bind/listen");
        ::close(fd);
        return 1;
    }
    int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags >= 0) ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    std::cout << "[servidor] escuchando en " << cfg.bindAddr << ':' << cfg.port << '\n'
              << std::flush;
    Servidor servidor(fd, log);
    if (!servidor.ejecutar()) return 1;
    std::cout << "[servidor] cerrado.\n";
    return 0;
}
