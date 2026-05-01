#ifndef ESQUEMA_H
#define ESQUEMA_H

typedef struct {
    int id;
    off_t offset;
} IndiceEntrada;

typedef struct {
    int id;
    char nombre[50];
    float salario;
    int activo;
} Empleado;

typedef struct {
    int id;
    char nombre[50];
    char telefono[15];
    int activo;
} Cliente;

typedef struct {
    int id;
    char nombre[50];
    float precio;
    int stock;
    int activo;
} Producto;

typedef enum { Ninguna, Empleados, Clientes, Productos } TipoTabla;

#endif