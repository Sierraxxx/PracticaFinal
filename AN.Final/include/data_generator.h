#pragma once
// include/data_generator.h

#include "types.h"
#include <string>

/// Genera un vector de N claves según la distribución y universo dados.
/// Si U <= 0 se usa rango int32 completo [0, INT_MAX].
KeyVec generate(size_t N, Distribution dist, int U,
                unsigned seed = 20260321);

/// Escribe el dataset a un archivo CSV (una clave por línea).
void write_dataset(const KeyVec& v, const std::string& path);

/// Carga un dataset desde CSV.
KeyVec read_dataset(const std::string& path);
