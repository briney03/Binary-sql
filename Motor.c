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