#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>    // Para open() y constantes O_WRONLY...
#include <unistd.h>   // Para write(), close()

// ¡Conectamos nuestro molde!
#include "esquema.h"

#define MAX_BUFFER 1024

void print_prompt() {
    printf("mibd> ");
}

void read_input(char* buffer) {
    if (fgets(buffer, MAX_BUFFER, stdin) == NULL) {
        printf("Error fatal al leer la entrada.\n");
        exit(EXIT_FAILURE);
    }
    buffer[strcspn(buffer, "\n")] = 0; 
}

// --- Función dedicada a insertar ---
void ejecutar_insert(char* input) {
    Empleado nuevo;
    nuevo.activo = 1; // Por defecto, el empleado nace "vivo"

    int datos_leidos = sscanf(input, "INSERT %d %49s %f", &nuevo.id, nuevo.nombre, &nuevo.salario);

    if (datos_leidos != 3) {
        printf("Error: Sintaxis incorrecta.\nUso esperado: INSERT <id> <nombre> <salario>\n");
        return; 
    }

    int fd = open("empleados.dat", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("Error de Kernel: No se pudo abrir la BD");
        return;
    }

    write(fd, &nuevo, sizeof(Empleado));
    close(fd);

    printf("=> Registro insertado con éxito: ID %d | %s\n", nuevo.id, nuevo.nombre);
}

// --- Función para actualizar un registro (UPDATE) ---
void ejecutar_update(char* input) {
    int target_id;
    float nuevo_salario;
    
    // Extraemos el ID y el nuevo salario del comando
    int datos_leidos = sscanf(input, "UPDATE %d %f", &target_id, &nuevo_salario);

    if (datos_leidos != 2) {
        printf("Error: Sintaxis incorrecta.\nUso esperado: UPDATE <id> <nuevo_salario>\n");
        return; 
    }

    int fd = open("empleados.dat", O_RDWR);
    if (fd == -1) {
        printf("Error: No se pudo abrir la BD.\n");
        return;
    }

    Empleado reg;
    bool encontrado = false;

    // Buscamos el registro leyendo bloque por bloque
    while (read(fd, &reg, sizeof(Empleado)) > 0) {
        
        if (reg.id == target_id && reg.activo == 1) {
            encontrado = true;
            
            // 1. Modificamos el dato en nuestra variable RAM
            reg.salario = nuevo_salario;

            // 2. MAGIA: Retrocedemos el puntero del Kernel 1 bloque
            off_t offset = -(off_t)sizeof(Empleado);
            lseek(fd, offset, SEEK_CUR);

            // 3. Sobrescribimos ese bloque exacto en el disco duro
            write(fd, &reg, sizeof(Empleado));
            
            printf("=> Salario actualizado con éxito para el ID %d.\n", target_id);
            break; 
        }
    }

    if (!encontrado) {
        printf("Error: No se encontró un empleado activo con el ID %d.\n", target_id);
    }

    close(fd);
}

// --- Función para realizar un borrado lógico (DELETE) ---
void ejecutar_delete(char* input) {
    int target_id;
    
    // Extraemos solo el ID que queremos borrar
    int datos_leidos = sscanf(input, "DELETE %d", &target_id);

    if (datos_leidos != 1) {
        printf("Error: Sintaxis incorrecta.\nUso esperado: DELETE <id>\n");
        return; 
    }

    // Abrimos con O_RDWR porque vamos a leer y luego sobrescribir
    int fd = open("empleados.dat", O_RDWR);
    if (fd == -1) {
        printf("Error: No se pudo abrir la BD.\n");
        return;
    }

    Empleado reg;
    bool encontrado = false;

    // Recorremos el archivo bloque por bloque
    while (read(fd, &reg, sizeof(Empleado)) > 0) {
        
        if (reg.id == target_id && reg.activo == 1) {
            encontrado = true;
            
            // 1. EL BORRADO LÓGICO: "Matamos" al registro en la RAM
            reg.activo = 0;

            // 2. Retrocedemos el puntero del Kernel 62 bytes exactos
            off_t offset = -(off_t)sizeof(Empleado);
            lseek(fd, offset, SEEK_CUR);

            // 3. Sobrescribimos el bloque en el disco duro
            write(fd, &reg, sizeof(Empleado));
            
            printf("=> Empleado con ID %d eliminado correctamente (Borrado Lógico).\n", target_id);
            break; 
        }
    }

    if (!encontrado) {
        printf("Error: No se encontró un empleado activo con el ID %d.\n", target_id);
    }

    close(fd);
}

// --- Función para recuperar un registro (RESTORE / UNDELETE) ---
void ejecutar_restore(char* input) {
    int target_id;
    
    // Extraemos el ID que queremos recuperar
    int datos_leidos = sscanf(input, "RESTORE %d", &target_id);

    if (datos_leidos != 1) {
        printf("Error: Sintaxis incorrecta.\nUso esperado: RESTORE <id>\n");
        return; 
    }

    int fd = open("empleados.dat", O_RDWR);
    if (fd == -1) {
        printf("Error: No se pudo abrir la BD.\n");
        return;
    }

    Empleado reg;
    bool encontrado = false;

    // Recorremos el archivo
    while (read(fd, &reg, sizeof(Empleado)) > 0) {
        
        // OJO: Aquí buscamos que el ID coincida y que esté INACTIVO (0)
        if (reg.id == target_id && reg.activo == 0) {
            encontrado = true;
            
            // ¡LA RESURRECCIÓN! Lo volvemos a la vida
            reg.activo = 1;

            // Retrocedemos y sobrescribimos
            off_t offset = -(off_t)sizeof(Empleado);
            lseek(fd, offset, SEEK_CUR);
            write(fd, &reg, sizeof(Empleado));
            
            printf("=> ¡Milagro! Empleado con ID %d ha sido restaurado y está activo de nuevo.\n", target_id);
            break; 
        }
    }

    if (!encontrado) {
        // Puede que no exista, o que exista pero ya esté activo
        printf("Error: No se encontró un registro inactivo con el ID %d.\n", target_id);
    }

    close(fd);
}

// --- Función para leer y mostrar todos los registros ---
void ejecutar_select() {
    Empleado reg;
    int fd = open("empleados.dat", O_RDONLY);

    if (fd == -1) {
        printf("Info: La base de datos está vacía o no existe.\n");
        return;
    }

    printf("\nID  | Nombre                               | Salario    | Estado\n");
    printf("------------------------------------------------------------------\n");

    while (read(fd, &reg, sizeof(Empleado)) > 0) {
        if (reg.activo == 1) {
            printf("%-3d | %-36s | $%-10.2f | Activo\n", 
                   reg.id, reg.nombre, reg.salario);
        } else {
            printf("%-3d | [REGISTRO ELIMINADO]                 | ---------- | Inactivo\n", reg.id);
        }
    }

    printf("------------------------------------------------------------------\n");
    close(fd);
}

// --- EL ÚNICO Analizador (Parser) ---
void parse_and_execute(char* input) {
    if (strcmp(input, "exit") == 0) {
        printf("Cerrando el motor de base de datos... Adiós.\n");
        exit(EXIT_SUCCESS);
    } 
    else if (strncmp(input, "INSERT", 6) == 0) {
        ejecutar_insert(input);
    }
    else if (strcmp(input, "SELECT") == 0) {
        ejecutar_select();
    }
    else if (strncmp(input, "UPDATE", 6) == 0) {
        ejecutar_update(input);
    }
    // ¡NUESTRO NUEVO CÓDIGO AQUÍ!
    else if (strncmp(input, "DELETE", 6) == 0) {
        ejecutar_delete(input);
    }
    // ¡NUEVO COMANDO
    else if (strncmp(input, "RESTORE", 7) == 0) {
        ejecutar_restore(input);
    }
    else if (strncmp(input, "START TRANSACTION", 17) == 0) {
        printf("[DEBUG] Preparando sección crítica para transacciones...\n");
    }
    else {
        printf("Error sintáctico: Comando '%s' no reconocido.\n", input);
    }
}

int main() {
    char input_buffer[MAX_BUFFER];
    
    printf("=========================================\n");
    printf("   Motor de Base de Datos v0.2 Iniciado  \n");
    printf("   Escribe 'exit' para salir del motor.  \n");
    printf("=========================================\n\n");

    while (true) {
        print_prompt();
        read_input(input_buffer);

        if (strlen(input_buffer) == 0) continue;

        parse_and_execute(input_buffer);
    }

    return 0;
}