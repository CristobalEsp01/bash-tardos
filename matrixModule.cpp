#include <cstdlib>
#include "utils.h"
using namespace std;

void menuMultiplicacionMatrices() {
    int opcion = -1;

    do {
        cout << "\n--- Modulo - Multiplicacion de Matrices ---\n";
        cout << "0) Salir\n";
        cout << "1) Elegir Matrices\n";
        cout << "Opcion: ";
        cin >> opcion;
        limpiarBuffer();

        switch (opcion) {
            case 0:
                cout << "Saliendo del modulo de multiplicacion de matrices...\n";
                break;

            case 1: {
                // podría añadirse una debug flag que decida si imprimir el exit code o no
                cout << endl;
                int raw_status = std::system(
                    "./matmul /home/felipe/bash-tardos/data/test_matrices/A.txt /home/felipe/bash-tardos/data/test_matrices/B.txt '#'"
                );
                cout << "programa retorno con codigo " << raw_status << std::endl;
                break;
            }

            default:
                cout << "Opcion invalida.\n";
        }
    } while (opcion != 0);
}