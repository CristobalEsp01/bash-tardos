#ifndef INDICE_INVERTIDO_MODULE_H
#define INDICE_INVERTIDO_MODULE_H

#include "config.h"

void crearIndiceInvertido(const Config& cfg);

// Publica dos temporales ya escritos. Conserva el mapeo anterior si falla
// la publicacion del indice; elimina los temporales consumidos o fallidos.
bool publicarArchivosIndice(const std::string& mapeoTemporal,
                           const std::string& mapeoDestino,
                           const std::string& indiceTemporal,
                           const std::string& indiceDestino,
                           std::string& error);

#endif
