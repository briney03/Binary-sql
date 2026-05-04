#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "types.h"

void iniciar_transaccion(void);
void confirmar_transaccion(void);
void deshacer_transaccion(void);
void registrar_en_log(char* tabla, int id, char* operacion, char* datos_previos);
int esta_en_transaccion(void);

#endif