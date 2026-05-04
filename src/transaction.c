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

static int en_transaccion = 0;

void iniciar_transaccion(void) {
    en_transaccion = 1;
    printf("=> Transaccion iniciada (START TRANSACTION)\n");
}

void confirmar_transaccion(void) {
    if (!en_transaccion) {
        printf("Error: No hay transaccion activa.\n");
        return;
    }
    en_transaccion = 0;
    printf("=> Transaccion confirmada (COMMIT).\n");
}

void registrar_en_log(char* tabla, int id, char* operacion, char* datos_previos) {
    (void)tabla;
    (void)id;
    (void)operacion;
    (void)datos_previos;
}

void deshacer_transaccion(void) {
    if (!en_transaccion) {
        printf("Error: No hay transaccion activa.\n");
        return;
    }
    en_transaccion = 0;
    printf("=> Transaccion cancelada (ROLLBACK).\n");
}

int esta_en_transaccion(void) {
    return en_transaccion;
}