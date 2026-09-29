#ifndef CONTEOTEXTOMODULE_H
#define CONTEOTEXTOMODULE_H

#include <string>

struct ResultadoConteo {
    long long vocales = 0;
    long long consonantes = 0;
    long long especiales = 0;   // todo lo que no es letra, digito ni espacio
    long long palabras = 0;     // secuencias de letras/digitos
};

// Cuenta sobre un archivo. Devuelve false (y muestra el motivo) si el
// archivo no existe, no es un archivo regular o no se puede leer.
bool contarArchivo(const std::string& rutaArchivo, ResultadoConteo& resultado);

// Opcion 6: cuenta sobre el archivo recibido con -f y muestra el resumen
// con la opcion VOLVER.
void conteoSobreTexto(const std::string& rutaArchivo);

#endif
