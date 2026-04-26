#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

// Definimos la estructura
typedef struct {
    int id;
    char nombre[50];
    float salario;
    int activo;
} Empleado;

int main() {
    // 1. Llenamos el molde con datos
    Empleado nuevo_empleado;
    nuevo_empleado.id = 1;
    strcpy(nuevo_empleado.nombre, "Hector"); // Usamos strcpy para cadenas en C
    nuevo_empleado.salario = 25000.50;
    nuevo_empleado.activo = 1;               // 1 significa que está "vivo" en la BD

    // 2. System Call: open (Binario, Append, Crear)
    // O_APPEND asegura que siempre escribamos al final, sin borrar a los anteriores
    int fd = open("empleados.dat", O_WRONLY | O_CREAT | O_APPEND, 0644);
    
    if (fd == -1) {
        perror("Error al abrir el archivo de la BD");
        return 1;
    }

    // 3. System Call: write (¡La Magia Binaria!)
    // En lugar de escribir letra por letra, mandamos TODO el struct de un solo golpe
    write(fd, &nuevo_empleado, sizeof(Empleado));

    printf("Registro guardado exitosamente en empleados.dat\n");

    // 4. System Call: close
    close(fd);

    return 0;
}