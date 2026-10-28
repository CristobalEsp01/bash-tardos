// =============================================================================
//  juego.h - Tipos y API del juego BAJO / MEDIO / ALTO (2 equipos).
//  Declaraciones; implementación en src/juego.cpp.
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

#include <random>
#include <string>
#include <vector>


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
    Juego();

    EstadoJuego estado = ESPERANDO;
    int host_id = -1;
    int max_rondas = RONDAS_POR_DEFECTO;
    int ronda_actual = 0;
    std::vector<Jugador> jugadores;
    int ganador = -1;                // equipo ganador (0 ó 1); -1 = empate
    int desconectado_id = -1;        // != -1 si el juego se interrumpió
    std::mt19937 rng;                // va dentro del estado: copiarlo copia también el azar

    // --- estado de turnos (T1) ---
    int turno_id = -1;               // id del jugador que debe actuar ahora (-1 si no hay turno)
    int pareja_primera = -1;         // equipo de la pareja que actúa primero en cada ronda (fijo por partida)
    std::vector<int> orden_turnos;   // ids en el orden de la ronda actual (4 elementos)
    int indice_turno = 0;            // posición actual en orden_turnos
};

// Helpers usados por servidor y pruebas.
namespace detalle {
extern const char* const NOMBRE_JUGADA[4];
Jugador* buscar(Juego& j, int id);
const Jugador* buscar(const Juego& j, int id);
std::string normalizar(const std::string& s);
std::string limpiar_nombre(const std::string& s);
int calcular_puntos(TipoJugada propia, TipoJugada rival);
int puntos_equipo(const Juego& j, int equipo);
std::string listar_equipo(const Juego& j, int equipo, bool con_puntos);
}

Resultado agregar_jugador(Juego& j, int id, const std::string& nombre);
Resultado remover_jugador(Juego& j, int id);
Resultado procesar_accion(Juego& j, int id, const std::string& accion);
std::string pantalla_para(const Juego& j, int id);
std::vector<int> ids_conectados(const Juego& j);
bool juego_terminado(const Juego& j);
