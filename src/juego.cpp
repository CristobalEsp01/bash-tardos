#include <bits/stdc++.h>
using namespace std;

// TODO: Terminar

/*
El juego funciona de la siguiente manera:
Deben haber dos equipos de dos personas conectados (en el futuro pueden ser más personas por equipo, pero mínimo dos equipos y deben ser de tamaños iguales)
Dos rivales son emparejados aleatoriamente para competir entre sí cada ronda (no saben contra quien les tocó)
Si ambos rivales eligen un movimiento de valor ALTO, entonces ninguno gana puntos
Si uno escoge BAJO y el otro ALTO, entonces el que escogió BAJO gana 5 puntos
Si no ocurre alguno de esos casos, entonces los jugadores reciben puntos acorde a su eleccion:
1 punto para BAJO, 2 para MEDIO y 3 para ALTO
Los puntos que gane un jugador pertenecen a su equipo

Se escoge el número de rondas al principio del juego (por defecto 5)
En cuanto termine se decide el grupo ganador el que tenga mayor puntaje
También muestra cuanto aportó cada jugador

Si alguien se desconecta el juego termina y declara como ganador al grupo con mayor puntaje o al grupo del host si tienen lo mismo ambos grupos
*/

#define JUGADORES_NECESARIOS 4
enum EstadoJuego { ESPERANDO = 0, ACTIVO = 1, TERMINADO = 2 };
enum tipoJugada { BAJO = 1, MEDIO = 2, ALTO = 3 };

struct Jugador {
    int id;
    int puntaje;
    tipoJugada jugada;
    string nombre;
}

struct Juego {
    int host_id = -1;
    int MAX_RONDAS = 5;
    int ronda_cont = 0;
    vector<Jugador> jugadores;
    int jugadores_cont = 0;

    pair<int,int> grupo1_ids;
    pair<int,int> grupo2_ids;

    int ganador = -1;
    bool juego_activo = false;
    bool interrumpido = false; // si alguien se desconectó en medio del juego

    void imprimirBanner() {
        // muestra información sobre el juego
    }
    void imprimirJugadores() {
        // muestra jugadores conectados en lista de espera continuamente refrescando la pantalla cada 3 segundos
    }

    // al conectarse un jugador
    void agregar_jugador() {

    }

    // en caso de desconectarse durante la espera
    void remover_jugador() {

    }

    // cuando estén los 4 jugadores
    void empezar_juego() {

    }   

    // al desconectarse alguien en medio de un juego o alguien gana
    void terminar_juego() {

    }
    
    void procesarJuego(vector<int> &argumentos) {
        // a y b son los jugadores de un equipo
        // a1 es emparejado con a1 y b1 con b2 
        int a1, b1, a2, b2;
        pair<int,int> rivales1;
        pair<int,int> rivales2;
        rivales1.first = a1;
        rivales2.first = b1;

        if (rand() % 2) {
            rivales1.second = a2;
            rivales2.second = b2;
        }
        else {
            rivales1.second = b2;
            rivales2.second = a2;
        }

        while (juego_activo && ganador = -1) {
            for (auto jugador : jugadores) {

            }
        }

        if (interrumpido) {
            // muestra mensaje de que la conexión se interrumpió
        }

        // mensaje de equipo ganador
    }

    obtenerEstadoJuego() {

        // de qué consiste el estado del juego?
        /*
        en espera, jugando, terminado (ENUM)
        de quién es el turno
        puntaje de los jugadores
        */

    }
}

int main() {
    srand(time(NULL));
    Juego juego;
}

/*
class JuegoCarrera:
    def __init__(self):
        # Estado del juego
        self.jugadores = {}  # {player_id: puntuacion}
        self.ganador = None
        self.juego_activo = True

    def agregar_jugador(self, player_id):
        if player_id not in self.jugadores:
            self.jugadores[player_id] = 0

    def eliminar_jugador(self, player_id):
        if player_id in self.jugadores:
            del self.jugadores[player_id]

    def procesar_accion(self, player_id, accion):
        """
        Este es el único método que llamará tu compañero.
        """
        if not self.juego_activo:
            return self.obtener_estado()

        if accion == "CLICK":
            self.jugadores[player_id] += 1
            
            # Condición de victoria
            if self.jugadores[player_id] >= 10:
                self.ganador = player_id
                self.juego_activo = False

        return self.obtener_estado()

    def obtener_estado(self):
        """Devuelve una foto actual del juego."""
        return {
            "jugadores": self.jugadores,
            "ganador": self.ganador,
            "activo": self.juego_activo
        }
*/