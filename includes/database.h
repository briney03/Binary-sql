#ifndef DATABASE_H
#define DATABASE_H

#include "types.h"

void init_databases(void);
int crear_database(char* nombre);
int usar_database(char* nombre);
char* get_database_actual(void);
int crear_tabla(char* nombre_tabla, Campo* campos, int num_campos);
int crear_tabla_desde_parser(const char* nombre_tabla, Campo* campos, int num_campos);
DefinicionDB* obtener_db(char* nombre);
DefinicionTabla* obtener_tabla(const char* db_nombre, const char* tabla_nombre);
int es_tabla_dinamica(const char* db_nombre, const char* tabla_nombre);
int listar_databases(DefinicionDB** resultado, int* cantidad);
int listar_tablas(const char* db, DefinicionTabla* resultado, int* cantidad);
int eliminar_database(char* nombre);
int eliminar_tabla(const char* db_nombre, const char* tabla_nombre);
int table_exists(const char* db_nombre, const char* tabla_nombre);

#endif