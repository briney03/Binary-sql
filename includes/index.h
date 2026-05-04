#ifndef INDEX_H
#define INDEX_H

#include "types.h"

off_t buscar_en_indice(const char* tabla, int id);
void actualizar_indice_insert(const char* tabla, int id, off_t offset);
void eliminar_indice(const char* tabla, int id);
void reconstruir_indice(const char* tabla);

#endif