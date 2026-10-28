#include "juego.h"
#include "utils.h"

#include <algorithm>
#include <sstream>

Juego::Juego() : rng(std::random_device{}()) {}

// ------------------------------- internos ------------------------------------
namespace detalle {

const char* const NOMBRE_JUGADA[4] = {"-", "BAJO", "MEDIO", "ALTO"};

Jugador* buscar(Juego& j, int id) {
    for (auto& p : j.jugadores) if (p.id == id) return &p;
    return nullptr;
}
const Jugador* buscar(const Juego& j, int id) {
    for (const auto& p : j.jugadores) if (p.id == id) return &p;
    return nullptr;
}

// Minúsculas sin tildes, usando utils.h (conserva dígitos)
std::string normalizar(const std::string& s) {
    std::string r;
    for (size_t i = 0; i < s.size();) {
        unsigned int cp = siguienteCaracterUtf8(s, i);
        if (char c = letraBase(cp)) r += c;
        else if (cp < 128) r += static_cast<char>(cp);
    }
    return r;
}

// Nombre recibido por red: sin caracteres de control y de largo acotado
std::string limpiar_nombre(const std::string& s) {
    std::string r;
    int largo = 0;
    for (size_t i = 0, ini = 0; i < s.size() && largo < LARGO_MAX_NOMBRE; ini = i) {
        unsigned int cp = siguienteCaracterUtf8(s, i);
        if (cp < 32 || cp == 127 || cp == 0xFFFD || cp == '<' || cp == '>' || cp == ';') continue;
        r += s.substr(ini, i - ini);
        largo++;
    }
    return trim(r);
}

int calcular_puntos(TipoJugada propia, TipoJugada rival) {
    if (propia == BAJO && rival == ALTO) return 5;
    if (propia == ALTO && rival == BAJO) return 0;   // asumido: el ALTO no gana nada contra un BAJO
    return propia;                                   // BAJO=1, MEDIO=2, ALTO=3
}

int puntos_equipo(const Juego& j, int e) {
    int suma = 0;
    for (const auto& p : j.jugadores) if (p.equipo == e) suma += p.puntaje;
    return suma;
}

std::string listar_equipo(const Juego& j, int e, bool con_puntos) {
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
void sortear_emparejamientos(Juego& j) {
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
void fijar_turnos_ronda(Juego& j) {
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
void avanzar_turno(Juego& j) {
    j.indice_turno++;
    if (j.indice_turno >= static_cast<int>(j.orden_turnos.size())) {
        j.turno_id = -1;   // señal: resolver ronda
    } else {
        j.turno_id = j.orden_turnos[j.indice_turno];
    }
}

void empezar_juego(Juego& j) {
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

void terminar_juego(Juego& j, int desconectado_id = -1) {
    j.estado = TERMINADO;
    j.desconectado_id = desconectado_id;
    j.turno_id = -1;
    int a = puntos_equipo(j, 0), b = puntos_equipo(j, 1);
    if (a != b)                     j.ganador = (a > b) ? 0 : 1;
    else if (desconectado_id != -1) j.ganador = buscar(j, j.host_id)->equipo;   // desempate: equipo del host
    else                            j.ganador = -1;                             // empate
}

// Se llama cuando los 4 ya eligieron (último turno resuelto)
void resolver_ronda(Juego& j) {
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
Resultado agregar_jugador(Juego& j, int id, const std::string& nombre) {
    if (j.estado != ESPERANDO)   return {false, "La partida ya empezó."};
    if (detalle::buscar(j, id))  return {false, "Ya estás conectado."};
    if (j.jugadores.size() >= JUGADORES_NECESARIOS) return {false, "La sala está llena."};

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
Resultado remover_jugador(Juego& j, int id) {
    Jugador* p = detalle::buscar(j, id);
    if (!p || !p->conectado) return {false, "No estás en la partida."};

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
Resultado procesar_accion(Juego& j, int id, const std::string& accion) {
    std::istringstream ss(accion);
    std::string cmd, argumento, extra;
    ss >> cmd;
    cmd = detalle::normalizar(cmd);

    Jugador* p = detalle::buscar(j, id);
    if (!p || !p->conectado) return {false, "No estás en la partida."};

    if (cmd == "rondas") {
        if (j.estado != ESPERANDO || id != j.host_id)
            return {false, "Solo el host puede cambiar las rondas antes de empezar."};
        if (!(ss >> argumento) || (ss >> extra) || argumento.empty() ||
            !std::all_of(argumento.begin(), argumento.end(), [](unsigned char c) { return c >= '0' && c <= '9'; }) ||
            argumento.size() > 2)
            return {false, "Rondas: de 1 a " + std::to_string(RONDAS_MAXIMAS) + "."};
        int n = std::stoi(argumento);
        if (n < 1 || n > RONDAS_MAXIMAS)
            return {false, "Rondas: de 1 a " + std::to_string(RONDAS_MAXIMAS) + "."};
        j.max_rondas = n;
        return {true, ""};
    }

    TipoJugada t = (cmd == "1" || cmd == "bajo")  ? BAJO
                 : (cmd == "2" || cmd == "medio") ? MEDIO
                 : (cmd == "3" || cmd == "alto")  ? ALTO : SIN_JUGADA;
    if (ss >> extra)            return {false, "Usa 1/BAJO, 2/MEDIO o 3/ALTO."};
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
std::string pantalla_para(const Juego& j, int id) {
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
std::vector<int> ids_conectados(const Juego& j) {
    std::vector<int> ids;
    for (const auto& p : j.jugadores) if (p.conectado) ids.push_back(p.id);
    return ids;
}

bool juego_terminado(const Juego& j) { return j.estado == TERMINADO; }
