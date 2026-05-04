#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "includes/config.h"
#include "includes/types.h"
#include "includes/errors.h"
#include "includes/parser.h"
#include "includes/transaction.h"
#include "includes/index.h"
#include "includes/database.h"

void print_prompt(void) {
    char* db = get_database_actual();
    if (db && db[0]) {
        printf("mibd:%s> ", db);
    } else {
        printf("mibd> ");
    }
}

void read_input(char* buffer) {
    if (fgets(buffer, MAX_BUFFER, stdin) == NULL) {
        printf("Error fatal al leer la entrada.\n");
        exit(EXIT_FAILURE);
    }
    buffer[strcspn(buffer, "\n")] = 0;
}

int main(void) {
    char input_buffer[MAX_BUFFER];

    printf("=========================================\n");
    printf("   Motor de Base de Datos v0.3 Iniciado  \n");
    printf("   Escribe 'exit' para salir del motor.  \n");
    printf("=========================================\n\n");

    while (1) {
        print_prompt();
        read_input(input_buffer);

        if (strlen(input_buffer) == 0) continue;

        parse_and_execute(input_buffer);
    }

    return 0;
}