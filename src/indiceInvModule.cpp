#include "indiceInvModule.h"
#include "utils.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <queue>
#include <cstdlib>
#include <pthread.h>
#include <semaphore.h>
#include <vector>
#include <map>
#include <sstream>
#include <cctype>

using namespace std;
namespace fs = std::filesystem;

queue<int> colaLibros;
sem_t sem_elementos;
sem_t sem_mutex;

map<int, string> mapaRutasGlobales;
// Ahora guarda: palabra -> {idLibro -> cantidad}
map<string, map<int, int>> indiceInvertidoGlobal;

string limpiarPalabra(string palabra) {
    string limpia = "";
    for (char c : palabra) {
        if (isalnum(c)) {
            limpia += tolower(c);
        }
    }
    return limpia;
}

void procesarLibro(int idLibro) {
    string ruta = mapaRutasGlobales[idLibro];
    ifstream archivo(ruta);
    if (!archivo.is_open()) return;

    string linea, palabra;
    map<string, int> conteoLocal;

    // Conteo de palabras local para este libro
    while (getline(archivo, linea)) {
        stringstream ss(linea);
        while (ss >> palabra) {
            string limpia = limpiarPalabra(palabra);
            if (!limpia.empty()) {
                conteoLocal[limpia]++;
            }
        }
    }
    archivo.close();

    // Actualización del índice global (sección crítica)
    sem_wait(&sem_mutex);
    for (const auto& par : conteoLocal) {
        indiceInvertidoGlobal[par.first][idLibro] += par.second;
    }
    sem_post(&sem_mutex);
}

void* threadTrabajador(void* arg) {
    while (true) {
        sem_wait(&sem_elementos);

        sem_wait(&sem_mutex);
        if (colaLibros.empty()) {
            sem_post(&sem_mutex);
            continue;
        }
        int idLibro = colaLibros.front();
        colaLibros.pop();
        sem_post(&sem_mutex);

        if (idLibro == -1) break; // Píldora de veneno (Punto 1.11)

        procesarLibro(idLibro);
    }
    return nullptr;
}

void crearIndiceInvertido(const string& librosDir) {
    limpiarPantalla();
    cout << "======================================\n";
    cout << "      CREACION DE INDICE INVERTIDO\n";
    cout << "======================================\n";

    const char* nThreadsStr = getenv("N_THREADS");
    const char* idxPath = getenv("INDICE_INVERTIDO_PATH");
    const char* mapeoPath = getenv("MAPEO_LIBROS_PATH");

    if (!nThreadsStr || !idxPath || !mapeoPath) {
        cout << "[ERROR] Faltan variables de entorno.\n";
        pausar();
        return;
    }

    int numThreads = stoi(nThreadsStr);
    if (numThreads <= 0) return;

    error_code ec;
    if (!fs::exists(librosDir, ec) || !fs::is_directory(librosDir, ec)) {
        cout << "[ERROR] La carpeta de libros '" << librosDir << "' no existe o no se puede leer.\n";
        pausar();
        return;
    }

    sem_init(&sem_elementos, 0, 0);
    sem_init(&sem_mutex, 0, 1);

    ofstream archivoMapeo(mapeoPath);
    int idLibro = 1;

    // Busca en todas las subcarpetas (ej. drama/)
    for (const auto& entry : fs::recursive_directory_iterator(librosDir)) {
        if (entry.is_regular_file()) {
            string rutaArchivo = entry.path().string();
            mapaRutasGlobales[idLibro] = rutaArchivo;
            
            archivoMapeo << idLibro << ";" << entry.path().filename().string() << "\n";
            colaLibros.push(idLibro);
            sem_post(&sem_elementos);
            idLibro++;
        }
    }
    archivoMapeo.close();

    // Liberamos semaforos para que los threads terminen
    for (int i = 0; i < numThreads; i++) {
        colaLibros.push(-1);
        sem_post(&sem_elementos);
    }

    cout << "Utilizando " << numThreads << " threads...\n";
    vector<pthread_t> threads(numThreads);

    for (int i = 0; i < numThreads; i++) {
        pthread_create(&threads[i], nullptr, threadTrabajador, nullptr);
    }

    for (int i = 0; i < numThreads; i++) {
        pthread_join(threads[i], nullptr);
    }

    // Escritura final con el formato pedido: palabra;(ID,cant);(ID,cant)
    ofstream archivoIndice(idxPath);
    for (const auto& par : indiceInvertidoGlobal) {
        archivoIndice << par.first;
        for (const auto& doc : par.second) {
            archivoIndice << ";(" << doc.first << "," << doc.second << ")";
        }
        archivoIndice << "\n";
    }
    archivoIndice.close();

    indiceInvertidoGlobal.clear();
    mapaRutasGlobales.clear();

    sem_destroy(&sem_elementos);
    sem_destroy(&sem_mutex);

    cout << "Tarea finalizada con éxito.\n";
    pausar("Presione ENTER para volver...");
}