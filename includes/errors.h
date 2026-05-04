#ifndef ERRORS_H
#define ERRORS_H

#include "types.h"

extern CodigoError ultimo_error;
const char* mensaje_error(CodigoError codigo);
void imprimir_error(void);

#endif