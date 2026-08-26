#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <limits>

// Utilidad compartida para limpiar buffer
inline void limpiarBuffer() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}
#endif