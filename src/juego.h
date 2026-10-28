// =============================================================================
//  juego.hpp - Lógica del juego BAJO / MEDIO / ALTO (2 equipos). Sin red, I/O ni hilos.
//  Requiere utils.h.  Compilar con: g++ -std=c++17
//
//  `Juego` es un valor simple y copiable: una copia es un snapshot del estado.
//    Escriben:               agregar_jugador, remover_jugador, procesar_accion
//    Solo leen (const):      pantalla_para, ids_conectados, juego_terminado
//  La librería NO sincroniza hilos: eso lo decide el servidor.
//
//  USO EN EL SERVIDOR (un solo Juego; cada cliente tiene un id único):
//    cliente se conecta     -> agregar_jugador(juego, id, nombre)
//    cliente envía texto    -> procesar_accion(juego, id, "alto")
//                              (acepta 1/2/3, bajo/medio/alto y "RONDAS <n>" del host)
//    cliente se desconecta  -> remover_jugador(juego, id)
//
//  Las tres devuelven un Resultado:
//    ok == true   -> cambió el estado: enviar pantalla_para(snapshot, id) a cada id
//                    de ids_conectados(snapshot)
//    ok == false  -> enviar r.mensaje solo a quien hizo la llamada
//
//  Un solo hilo (poll/select): llamar y difundir directamente con `juego`.
//  Varios hilos: UN mutex del servidor alrededor de la llamada y difundir desde la
//  copia, fuera del lock (así cada difusión refleja exactamente esa acción):
//      Juego snap; Resultado r;
//      { std::lock_guard<std::mutex> lk(m); r = procesar_accion(juego, id, txt); snap = juego; }
//      if (r.ok) for (int i : ids_conectados(snap)) enviar(i, pantalla_para(snap, i));
//
//  Cuando juego_terminado(snapshot) es true se pueden cerrar los sockets; para una
//  sala nueva basta  juego = Juego();  Detectar desconexiones y timeouts es tarea
//  de la capa de red.
//
//  El juego empieza solo al conectarse el 4º jugador (el 1º es el host). Si alguien
//  se desconecta en medio, gana el equipo con más puntos (si empatan, el del host).
//
//  TURNOS (T1): cada ronda tiene 4 turnos. Las parejas (rivales) actúan una
//  después de la otra; la pareja que va primero se sortea una vez al empezar y
//  queda fija toda la partida. Dentro de cada pareja actúa primero el jugador de
//  menor id. El orden completo de los 4 turnos de la ronda se fija al empezar la
//  ronda (a partir de las parejas REALES, vía rival_id) y se guarda en
//  `orden_turnos`; `indice_turno` avanza por ese vector. Solo el jugador del
//  turno actual puede elegir; el resto ve "Esperando a X...". El servidor rechaza
//  acciones fuera de turno.
// =============================================================================
#pragma once

#include <algorithm>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "../include/utils.h"

constexpr int JUGADORES_POR_EQUIPO = 2;
constexpr int JUGADORES_NECESARIOS = 2 * JUGADORES_POR_EQUIPO;
constexpr int RONDAS_POR_DEFECTO   = 5;
constexpr int RONDAS_MAXIMAS       = 50;
constexpr int LARGO_MAX_NOMBRE     = 16;

enum EstadoJuego { ESPERANDO = 0, ACTIVO = 1, TERMINADO = 2 };
enum TipoJugada  { SIN_JUGADA = 0, BAJO = 1, MEDIO = 2, ALTO = 3 };

struct Resultado {
    bool ok = false;
    std::string mensaje;             // motivo del error si ok == false
};

struct Jugador {
    int id = -1;
    std::string nombre;
    bool conectado = true;
    int equipo = -1;                 // 0 ó 1
    int puntaje = 0;                 // aporte al equipo
    TipoJugada jugada = SIN_JUGADA;  // jugada de la ronda en curso
    int rival_id = -1;               // rival de la ronda en curso (nunca se muestra)
    std::string ultima_ronda;        // resultado de la ronda anterior, ya como texto
};

struct Juego {
    EstadoJuego estado = ESPERANDO;
    int host_id = -1;
    int max_rondas = RONDAS_POR_DEFECTO;
    int ronda_actual = 0;
    std::vector<Jugador> jugadores;
    int ganador = -1;                // equipo ganador (0 ó 1); -1 = empate
    int desconectado_id = -1;        // != -1 si el juego se interrumpió
    std::mt19937 rng{std::random_device{}()};   // va dentro del estado: copiarlo copia también el azar

    // --- estado de turnos (T1) ---
    int turno_id = -1;               // id del jugador que debe actuar ahora (-1 si no hay turno)
    int pareja_primera = -1;         // equipo de la pareja que actúa primero en cada ronda (fijo por partida)
    std::vector<int> orden_turnos;   // ids en el orden de la ronda actual (4 elementos)
    int indice_turno = 0;            // posición actual en orden_turnos
};

// ------------------------------- internos ------------------------------------
namespace detalle {

constexpr const char* NOMBRE_JUGADA[] = {"-", "BAJO", "MEDIO", "ALTO"};

inline Jugador* buscar(Juego& j, int id) {
    for (auto& p : j.jugadores) if (p.id == id) return &p;
    return nullptr;
}
inline const Jugador* buscar(const Juego& j, int id) {
    for (const auto& p : j.jugadores) if (p.id == id) return &p;
    return nullptr;
}

// Minúsculas sin tildes, usando utils.h (conserva dígitos)
inline std::string normalizar(const std::string& s) {
    std::string r;
    for (size_t i = 0; i < s.size();) {
        unsigned int cp = siguienteCaracterUtf8(s, i);
        if (char c = letraBase(cp)) r += c;
        else if (cp < 128) r += static_cast<char>(cp);
    }
    return r;
}

// Nombre recibido por red: sin caracteres de control y de largo acotado
inline std::string limpiar_nombre(const std::string& s) {
    std::string r;
    int largo = 0;
    for (size_t i = 0, ini = 0; i < s.size() && largo < LARGO_MAX_NOMBRE; ini = i) {
        unsigned int cp = siguienteCaracterUtf8(s, i);
        if (cp < 32 || cp == 127 || cp == 0xFFFD) continue;
        r += s.substr(ini, i - ini);
        largo++;
    }
    return trim(r);
}

inline int calcular_puntos(TipoJugada propia, TipoJugada rival) {
    if (propia == BAJO && rival == ALTO) return 5;
    if (propia == ALTO && rival == BAJO) return 0;   // asumido: el ALTO no gana nada contra un BAJO
    return propia;                                   // BAJO=1, MEDIO=2, ALTO=3
}

inline int puntos_equipo(const Juego& j, int e) {
    int suma = 0;
    for (const auto& p : j.jugadores) if (p.equipo == e) suma += p.puntaje;
    return suma;
}

inline std::string listar_equipo(const Juego& j, int e, bool con_puntos) {
    std::string s;
    for (const auto& p : j.jugadores) {
        if (p.equipo != e) continue;
        if (!s.empty()) s += ", ";
        s += p.nombre;
        if (con_puntos) s += " " + std::to_string(p.puntaje);
    }
    return s;
}

// Reparte equipos al azar y sortea los emparejamientos (rival_id simétrico).
// NO fija turnos: eso lo hace fijar_turnos_ronda() después.
inline void sortear_emparejamientos(Juego& j) {
    std::vector<Jugador*> eq[2];
    for (auto& p : j.jugadores) eq[p.equipo].push_back(&p);
    std::shuffle(eq[1].begin(), eq[1].end(), j.rng);
    for (size_t i = 0; i < eq[0].size(); i++) {
        eq[0][i]->rival_id = eq[1][i]->id;
        eq[1][i]->rival_id = eq[0][i]->id;
    }
}

// --- turnos T1 ---------------------------------------------------------------

// Fija orden_turnos (4 ids) para la ronda actual a partir de las parejas REALES:
//   pareja_primera: menor id de esa pareja -> su rival
//   otra pareja:    menor id de esa pareja -> su rival
// Luego deja indice_turno = 0 y turno_id = orden_turnos[0].
// El orden se basa en rival_id (el emparejamiento real), no en "menor id por equipo",
// así no hay duplicados aunque el sorteo empareje al menor id de un equipo con el
// menor id del otro.
inline void fijar_turnos_ronda(Juego& j) {
    int e1 = j.pareja_primera;
    int e2 = 1 - e1;

    // Jugadores de cada equipo ordenados por id
    std::vector<const Jugador*> e1p, e2p;
    for (const auto& p : j.jugadores) {
        if (p.equipo == e1) e1p.push_back(&p);
        else if (p.equipo == e2) e2p.push_back(&p);
    }
    std::sort(e1p.begin(), e1p.end(), [](const Jugador* a, const Jugador* b){ return a->id < b->id; });
    std::sort(e2p.begin(), e2p.end(), [](const Jugador* a, const Jugador* b){ return a->id < b->id; });

    // Parejas reales: cada jugador de e1 con su rival (rival_id)
    // pareja A = {e1p[0], rival de e1p[0]}
    // pareja B = {e1p[1], rival de e1p[1]}
    int a = e1p[0]->id, b = e1p[0]->rival_id;
    int c = e1p[1]->id, d = e1p[1]->rival_id;

    // Dentro de cada pareja, el de menor id va primero
    if (a > b) std::swap(a, b);
    if (c > d) std::swap(c, d);

    // Pareja A (la de pareja_primera) va primero; luego la B.
    // Nota: pareja A y B se distinguen por ser las dos parejas del equipo e1.
    j.orden_turnos = {a, b, c, d};
    j.indice_turno = 0;
    j.turno_id = j.orden_turnos[0];
}

// Avanza al siguiente turno de la ronda. Si ya no hay más, deja turno_id = -1
// (señal para que procesar_accion resuelva la ronda).
inline void avanzar_turno(Juego& j) {
    j.indice_turno++;
    if (j.indice_turno >= static_cast<int>(j.orden_turnos.size())) {
        j.turno_id = -1;   // señal: resolver ronda
    } else {
        j.turno_id = j.orden_turnos[j.indice_turno];
    }
}

inline void empezar_juego(Juego& j) {
    std::shuffle(j.jugadores.begin(), j.jugadores.end(), j.rng);   // equipos al azar
    for (int i = 0; i < JUGADORES_NECESARIOS; i++)
        j.jugadores[i].equipo = (i < JUGADORES_POR_EQUIPO) ? 0 : 1;
    j.estado = ACTIVO;
    j.ronda_actual = 1;
    sortear_emparejamientos(j);
    // Pareja que va primero: sortear una vez (fija por partida).
    j.pareja_primera = (j.rng() % 2 == 0) ? 0 : 1;
    fijar_turnos_ronda(j);
}

inline void terminar_juego(Juego& j, int desconectado_id = -1) {
    j.estado = TERMINADO;
    j.desconectado_id = desconectado_id;
    j.turno_id = -1;
    int a = puntos_equipo(j, 0), b = puntos_equipo(j, 1);
    if (a != b)                     j.ganador = (a > b) ? 0 : 1;
    else if (desconectado_id != -1) j.ganador = buscar(j, j.host_id)->equipo;   // desempate: equipo del host
    else                            j.ganador = -1;                             // empate
}

// Se llama cuando los 4 ya eligieron (último turno resuelto)
inline void resolver_ronda(Juego& j) {
    for (auto& p : j.jugadores) {
        TipoJugada rival = buscar(j, p.rival_id)->jugada;
        int pts = calcular_puntos(p.jugada, rival);
        p.puntaje += pts;
        p.ultima_ronda = std::string("Ronda anterior: tú ") + NOMBRE_JUGADA[p.jugada] +
                         ", rival " + NOMBRE_JUGADA[rival] + " -> " + std::to_string(pts) + " pts";
    }
    for (auto& p : j.jugadores) p.jugada = SIN_JUGADA;

    if (j.ronda_actual >= j.max_rondas) {
        terminar_juego(j);
    } else {
        j.ronda_actual++;
        sortear_emparejamientos(j);
        fijar_turnos_ronda(j);   // pareja_primera se mantiene fija por partida
    }
}

}  // namespace detalle

// =============================== API PÚBLICA =================================

// El primero en llegar es el host. Al llegar el último necesario, empieza el juego.
inline Resultado agregar_jugador(Juego& j, int id, const std::string& nombre) {
    if (j.estado != ESPERANDO)   return {false, "La partida ya empezó."};
    if (detalle::buscar(j, id))  return {false, "Ya estás conectado."};

    Jugador p;
    p.id = id;
    p.nombre = detalle::limpiar_nombre(nombre);
    if (p.nombre.empty()) p.nombre = "Jugador " + std::to_string(id);
    if (j.jugadores.empty()) j.host_id = id;
    j.jugadores.push_back(p);

    if (static_cast<int>(j.jugadores.size()) == JUGADORES_NECESARIOS) detalle::empezar_juego(j);
    return {true, ""};
}

// En la espera el jugador sale de la lista (si era el host, pasa al siguiente).
// En medio de la partida, esta termina (interrumpida).
inline Resultado remover_jugador(Juego& j, int id) {
    Jugador* p = detalle::buscar(j, id);
    if (!p) return {false, "No estás en la partida."};

    if (j.estado == ESPERANDO) {
        j.jugadores.erase(j.jugadores.begin() + (p - j.jugadores.data()));
        if (j.host_id == id) j.host_id = j.jugadores.empty() ? -1 : j.jugadores.front().id;
    } else {
        p->conectado = false;
        if (j.estado == ACTIVO) detalle::terminar_juego(j, id);
    }
    return {true, ""};
}

// Todo lo que escribe un cliente: jugadas ("1", "alto", ...) o "RONDAS <n>" (solo el host, en la espera)
inline Resultado procesar_accion(Juego& j, int id, const std::string& accion) {
    std::istringstream ss(accion);
    std::string cmd;
    int n = 0;
    ss >> cmd >> n;                    // n solo se usa con "RONDAS <n>"
    cmd = detalle::normalizar(cmd);

    Jugador* p = detalle::buscar(j, id);
    if (!p) return {false, "No estás en la partida."};

    if (cmd == "rondas") {
        if (j.estado != ESPERANDO || id != j.host_id)
            return {false, "Solo el host puede cambiar las rondas antes de empezar."};
        if (n < 1 || n > RONDAS_MAXIMAS)
            return {false, "Rondas: de 1 a " + std::to_string(RONDAS_MAXIMAS) + "."};
        j.max_rondas = n;
        return {true, ""};
    }

    TipoJugada t = (cmd == "1" || cmd == "bajo")  ? BAJO
                 : (cmd == "2" || cmd == "medio") ? MEDIO
                 : (cmd == "3" || cmd == "alto")  ? ALTO : SIN_JUGADA;
    if (t == SIN_JUGADA)         return {false, "Usa 1/BAJO, 2/MEDIO o 3/ALTO."};
    if (j.estado != ACTIVO)      return {false, "No hay una ronda en curso."};

    if (j.turno_id != id) {
        const Jugador* t_ = detalle::buscar(j, j.turno_id);
        return {false, "No es tu turno. Espera a " + (t_ ? t_->nombre : std::string("otro jugador")) + "."};
    }
    if (p->jugada != SIN_JUGADA) return {false, "Ya elegiste en esta ronda."};

    p->jugada = t;

    // ¿Terminó la ronda? Si todos eligieron, resolver. Si no, avanzar turno.
    bool todos = std::all_of(j.jugadores.begin(), j.jugadores.end(),
                             [](const Jugador& q) { return q.jugada != SIN_JUGADA; });
    if (todos) {
        detalle::resolver_ronda(j);
    } else {
        detalle::avanzar_turno(j);
        if (j.turno_id == -1) detalle::resolver_ronda(j);  // último turno de la ronda
    }
    return {true, ""};
}

// Texto listo para mostrar al jugador `id` (el cliente solo hace limpiarPantalla() y lo imprime)
inline std::string pantalla_para(const Juego& j, int id) {
    const Jugador* yo = detalle::buscar(j, id);
    if (!yo) return "No estás en la partida.\n";
    std::ostringstream o;

    if (j.estado == ESPERANDO) {
        o << "=== BAJO / MEDIO / ALTO ===\n"
          << "Elige 1=BAJO, 2=MEDIO o 3=ALTO: ganas 1, 2 o 3 puntos para tu equipo.\n"
          << "BAJO contra ALTO: el BAJO gana 5 y ALTO no gana nada.\n\n"
          << "Jugadores (" << j.jugadores.size() << "/" << JUGADORES_NECESARIOS << "): ";
        for (size_t i = 0; i < j.jugadores.size(); i++)
            o << (i ? ", " : "") << j.jugadores[i].nombre << (j.jugadores[i].id == j.host_id ? " (host)" : "");
        o << "\nRondas: " << j.max_rondas;
        if (id == j.host_id) o << "  (cambiar: RONDAS <n>)";
        o << "\n";
        return o.str();
    }

    int mi = yo->equipo, rival = 1 - mi;
    int a = detalle::puntos_equipo(j, mi), b = detalle::puntos_equipo(j, rival);
    if (j.estado == ACTIVO) {
        o << "Ronda " << j.ronda_actual << "/" << j.max_rondas
          << " | Puntos: tu equipo " << a << ", rival " << b << "\n"
          << "Tu equipo: " << detalle::listar_equipo(j, mi, false) << "\n";
        if (!yo->ultima_ronda.empty()) o << yo->ultima_ronda << "\n";
        if (j.turno_id == id) {
            if (yo->jugada == SIN_JUGADA) o << "Es tu turno. Elige: 1=BAJO 2=MEDIO 3=ALTO\n";
            else                          o << "Elegiste " << detalle::NOMBRE_JUGADA[yo->jugada] << ". Esperando a los demás...\n";
        } else {
            const Jugador* t_ = detalle::buscar(j, j.turno_id);
            o << "Esperando a " << (t_ ? t_->nombre : std::string("otro jugador")) << "...\n";
        }
    } else {
        o << "=== FIN ===\n";
        if (j.desconectado_id != -1)
            o << detalle::buscar(j, j.desconectado_id)->nombre << " se desconectó "
              << "(gana el equipo con más puntos; si empatan, el del host).\n";
        o << (j.ganador == -1 ? "Empate" : j.ganador == mi ? "¡Ganó tu equipo!" : "Perdió tu equipo") << "\n"
          << "Tu equipo: " << a << " pts (" << detalle::listar_equipo(j, mi, true) << ")\n"
          << "Rival: " << b << " pts (" << detalle::listar_equipo(j, rival, true) << ")\n";
    }
    return o.str();
}

// Jugadores que siguen conectados (destinatarios de cada difusión)
inline std::vector<int> ids_conectados(const Juego& j) {
    std::vector<int> ids;
    for (const auto& p : j.jugadores) if (p.conectado) ids.push_back(p.id);
    return ids;
}

inline bool juego_terminado(const Juego& j) { return j.estado == TERMINADO; }