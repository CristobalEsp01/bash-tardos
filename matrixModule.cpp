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
                // qué problemas puede tener pasar una ruta arbitraria?
                string path_string1, path_string2;
                cout << "Ingrese las rutas absolutas a los archivos con matrices: \n1) ";
                cin >> path_string1;
                path_string1 = "'" + path_string1 + "'";
                cout << "2) ";
                cin >> path_string2;
                path_string2 = "'" + path_string2 + "'";

                string sep_string;
                cout << "Ingrese el caracter separador: ";
                cin >> sep_string;
                sep_string = "'" + sep_string + "'";

                const string input_string = "./matmul " + path_string1 + " " + path_string2 + " " + sep_string;

                // podría añadirse una debug flag que decida si imprimir el exit code o no
                int raw_status = std::system(input_string.c_str());
                cout << "programa retorno con estatus " << raw_status << std::endl;
                break;
            }

            default:
                cout << "Opcion invalida.\n";
        }
    } while (opcion != 0);
}
