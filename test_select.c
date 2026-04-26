#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

typedef struct {
    int id;
    char nombre[50];
    float salario;
    int activo;
} Empleado;

int main() {
    Empleado empleado_leido;

    // System Call: open (Solo lectura)
    int fd = open("empleados.dat", O_RDONLY);
    
    if (fd == -1) {
        perror("La base de datos no existe aún");
        return 1;
    }

    printf("--- REGISTROS EN LA BASE DE DATOS ---\n");
    
    // System Call: read
    // read() devuelve el número de bytes leídos. Si es 0, llegamos al final del archivo.
    while (read(fd, &empleado_leido, sizeof(Empleado)) > 0) {
        
        // Solo mostramos los que no han sido "borrados"
        if (empleado_leido.activo == 1) {
            printf("ID: %d | Nombre: %s | Salario: $%.2f\n", 
                   empleado_leido.id, 
                   empleado_leido.nombre, 
                   empleado_leido.salario);
        }
    }
    printf("-------------------------------------\n");

    close(fd);
    return 0;
}