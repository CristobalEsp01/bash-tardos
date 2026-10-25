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

using namespace std;
namespace fs = std::filesystem;


queue<int> colaLibros; // Memoria compartida para los libros 
sem_t sem_elementos; // Contador de libros disponibles
sem_t sem_mutex;     // Candado para proteger la cola

void procesarLibro(int idLibro) {
    // TERMINAR IMPLEMENTACION: Abrir el archivo correspondiente al idLibro, ver su contenido, y actualizar el indice invertido.
}

void* threadTrabajador(void* arg) { // Funcion threads trabajadores
    while (true) {

        sem_wait(&sem_elementos); // Se duerme si la cola esta sin libros 

        sem_wait(&sem_mutex);     // Entra a seccion critica para sacar un libro de la cola
        int idLibro = colaLibros.front();
        colaLibros.pop();
        sem_post(&sem_mutex);     // Sale de la seccion critica

        // Condición de término limpia
        if (idLibro == -1) break; 

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

    // Iniciamos los semaforos
    sem_init(&sem_elementos, 0, 0); // Inicia en 0 (aqui no hay nada en la cola)
    sem_init(&sem_mutex, 0, 1);     // Inicia en 1 (desbloqueado)

    ofstream archivoMapeo(mapeoPath);
    int idLibro = 1;

    // Agregamos todos los libros a la cola y creamos el archivo de mapeo
    for (const auto& entry : fs::directory_iterator(librosDir)) {
        if (entry.is_regular_file()) {
            archivoMapeo << idLibro << ";" << entry.path().filename().string() << "\n";
            colaLibros.push(idLibro);
            sem_post(&sem_elementos); // Sube el contador de elementos
            idLibro++;
        }
    }
    archivoMapeo.close();

    // For para que los threads sepan cuándo salir
    for (int i = 0; i < numThreads; i++) {
        colaLibros.push(-1);
        sem_post(&sem_elementos);
    }

    cout << "[INFO] Lanzando " << numThreads << " threads...\n";
    vector<pthread_t> threads(numThreads);

    // Crear los threads
    for (int i = 0; i < numThreads; i++) {
        pthread_create(&threads[i], nullptr, threadTrabajador, nullptr);
    }

    // Esperar a que todos terminen su trabajo
    for (int i = 0; i < numThreads; i++) {
        pthread_join(threads[i], nullptr);
    }

    // Limpiamos la memoria y los semaforos
    sem_destroy(&sem_elementos);
    sem_destroy(&sem_mutex);

    cout << "[OK] Construcción concurrente finalizada con éxito.\n";
    pausar("Presione ENTER para volver...");
}