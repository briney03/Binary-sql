// Las siguientes 2 líneas son "Guardas de Inclusión".
// Evitan que el compilador se confunda si incluyes este archivo varias veces.
#ifndef ESQUEMA_H
#define ESQUEMA_H

// Aquí vive el molde de tu base de datos
typedef struct {
    int id;
    char nombre[50];
    float salario;
    int activo;
} Empleado;

#endif