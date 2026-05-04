#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

#include "../includes/config.h"
#include "../includes/types.h"
#include "../includes/errors.h"

extern CodigoError ultimo_error;

const char* mensaje_error(CodigoError codigo) {
    switch(codigo) {
        case OK: return "Operación exitosa";
        case ERR_ARCHIVO_NO_ENCONTRADO: return "Archivo no encontrado";
        case ERR_SINTAXIS: return "Error de sintaxis";
        case ERR_TABLA_NO_EXISTE: return "Tabla no existe";
        case ERR_ID_NO_ENCONTRADO: return "ID no encontrado";
        case ERR_BD_NO_EXISTE: return "Base de datos no existe";
        case ERR_DUPLICADO: return "Elemento duplicado";
        default: return "Error desconocido";
    }
}

void imprimir_error(void) {
    if (ultimo_error != OK) {
        fprintf(stderr, "Error: %s\n", mensaje_error(ultimo_error));
        ultimo_error = OK;
    }
}

CodigoError ultimo_error = OK;