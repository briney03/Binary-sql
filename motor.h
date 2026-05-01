#ifndef MOTOR_H
#define MOTOR_H

#include "esquema.h"

#define MAX_BUFFER 1024
#define MAX_LOG 100

typedef struct {
    char tabla[50];
    int id;
    char operacion[10];
    char datos_previos[256];
} LogEntry;

void iniciar_transaccion();
void confirmar_transaccion();
void deshacer_transaccion();
void registrar_en_log(char* tabla, int id, char* operacion, char* datos_previos);
int bloquear_tabla(char* tabla);
void desbloquear_tabla(char* tabla);

void print_prompt();
void read_input(char* buffer);
void parse_and_execute(char* input);

size_t obtener_tamano_registro(char* tabla);
TipoTabla get_tipo_tabla(char* tabla);
char* get_nombre_tabla(TipoTabla tipo);

off_t buscar_en_indice(TipoTabla tipo, int id);
void actualizar_indice_insert(TipoTabla tipo, int id, off_t offset);
void eliminar_indice(TipoTabla tipo, int id);
void reconstruir_indice(TipoTabla tipo);

void ejecutar_insert(char* tabla, char* input);
void ejecutar_update(char* tabla, char* input);
void ejecutar_cambio_estado(char* tabla, char* input, int activar);
void ejecutar_delete(char* input);
void ejecutar_restore(char* input);
void ejecutar_select(char* tabla);

#endif