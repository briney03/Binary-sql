#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <sys/types.h>

#define MAX_NOMBRE 50
#define MAX_TABLAS 10
#define MAX_DATABASES 10
#define MAX_CAMPOS 10
#define MAX_LINEA 1024

typedef enum {
    TIPO_INT,
    TIPO_FLOAT,
    TIPO_STRING
} TipoCampo;

typedef enum {
    TABLA_DINAMICA = 1
} TipoTablaMode;

typedef enum {
    OK = 0,
    ERR_ARCHIVO_NO_ENCONTRADO,
    ERR_SINTAXIS,
    ERR_TABLA_NO_EXISTE,
    ERR_ID_NO_ENCONTRADO,
    ERR_BD_NO_EXISTE,
    ERR_DUPLICADO,
    ERR_CAMPO_NO_ENCONTRADO,
    ERR_TIPO_INVALIDO
} CodigoError;

typedef struct {
    int id;
    off_t offset;
} IndiceEntrada;

typedef struct {
    char tabla[50];
    int id;
    char operacion[10];
    char datos_previos[256];
} LogEntry;

typedef struct {
    char nombre[MAX_NOMBRE];
    TipoCampo tipo;
    int longitud;
} Campo;

typedef struct {
    char nombre[MAX_NOMBRE];
    Campo campos[MAX_CAMPOS];
    int num_campos;
    TipoTablaMode modo;
    char tabla_base[MAX_NOMBRE];
} DefinicionTabla;

typedef struct {
    char nombre[MAX_NOMBRE];
    DefinicionTabla tablas[MAX_TABLAS];
    int num_tablas;
} DefinicionDB;

typedef struct {
    char nombre[MAX_NOMBRE];
    TipoCampo tipo;
    int longitud;
    off_t offset_valor;
} CampoLayout;

typedef struct {
    char nombre_tabla[MAX_NOMBRE];
    CampoLayout campos[MAX_CAMPOS];
    int num_campos;
    int tamano_total;
} TableLayout;

#endif