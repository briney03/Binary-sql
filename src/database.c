#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>

#include "../includes/config.h"
#include "../includes/types.h"
#include "../includes/errors.h"
#include "../includes/io.h"

static int mkdir_recursive(const char* path) {
    char tmp[256];
    snprintf(tmp, sizeof(tmp), "%s", path);
    size_t len = strlen(tmp);
    if (tmp[len - 1] == '/') tmp[len - 1] = '\0';

    for (char* p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
    return 0;
}

static DefinicionDB databases[MAX_DATABASES];
static int num_databases = 0;
static char db_actual[MAX_NOMBRE] = {0};

void init_databases(void) {
    num_databases = 0;
    mkdir("data", 0755); // ensures data dir exists
    
    DIR* d = opendir("data");
    if (!d) return;
    
    struct dirent* dir;
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_DIR) {
            if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) continue;
            
            if (num_databases < MAX_DATABASES) {
                DefinicionDB* db = &databases[num_databases];
                memset(db, 0, sizeof(DefinicionDB));
                strncpy(db->nombre, dir->d_name, MAX_NOMBRE - 1);
                db->nombre[MAX_NOMBRE - 1] = '\0';
                
                // Load tables
                char path[512];
                snprintf(path, sizeof(path), "data/%s", dir->d_name);
                DIR* td = opendir(path);
                if (td) {
                    struct dirent* tdir;
                    while ((tdir = readdir(td)) != NULL) {
                        char* ext = strstr(tdir->d_name, ".meta");
                        if (ext && strcmp(ext, ".meta") == 0) {
                            char tabla_nombre[MAX_NOMBRE];
                            size_t name_len = ext - tdir->d_name;
                            if (name_len >= MAX_NOMBRE) name_len = MAX_NOMBRE - 1;
                            strncpy(tabla_nombre, tdir->d_name, name_len);
                            tabla_nombre[name_len] = '\0';
                            
                            DefinicionTabla tdef;
                            if (cargar_metadatos_tabla(db->nombre, tabla_nombre, &tdef) == 0) {
                                if (db->num_tablas < MAX_TABLAS) {
                                    db->tablas[db->num_tablas] = tdef;
                                    db->num_tablas++;
                                }
                            }
                        }
                    }
                    closedir(td);
                }
                
                num_databases++;
            }
        }
    }
    closedir(d);
}

int crear_database(char* nombre) {
    if (num_databases >= MAX_DATABASES) {
        ultimo_error = ERR_DUPLICADO;
        return -1;
    }
    for (int i = 0; i < num_databases; i++) {
        if (strcmp(databases[i].nombre, nombre) == 0) {
            ultimo_error = ERR_DUPLICADO;
            return -1;
        }
    }

    memset(&databases[num_databases], 0, sizeof(DefinicionDB));
    strncpy(databases[num_databases].nombre, nombre, MAX_NOMBRE - 1);
    databases[num_databases].nombre[MAX_NOMBRE - 1] = '\0';
    databases[num_databases].num_tablas = 0;
    num_databases++;

    char dir[MAX_NOMBRE + 20];
    sprintf(dir, "data/%s", nombre);
    mkdir_recursive(dir);

    printf("=> Base de datos '%s' creada.\n", nombre);
    return 0;
}

int usar_database(char* nombre) {
    for (int i = 0; i < num_databases; i++) {
        if (strcmp(databases[i].nombre, nombre) == 0) {
            strcpy(db_actual, nombre);
            printf("=> Usando base de datos '%s'.\n", nombre);
            return 0;
        }
    }
    ultimo_error = ERR_BD_NO_EXISTE;
    return -1;
}

char* get_database_actual(void) {
    return db_actual[0] ? db_actual : NULL;
}

int crear_tabla(char* nombre_tabla, Campo* campos, int num_campos) {
    if (!db_actual[0]) {
        printf("Error: No hay base de datos seleccionada. Use USE <nombre>.\n");
        return -1;
    }

    DefinicionDB* db = NULL;
    for (int i = 0; i < num_databases; i++) {
        if (strcmp(databases[i].nombre, db_actual) == 0) {
            db = &databases[i];
            break;
        }
    }

    if (!db) {
        ultimo_error = ERR_BD_NO_EXISTE;
        return -1;
    }

    if (db->num_tablas >= MAX_TABLAS) {
        ultimo_error = ERR_DUPLICADO;
        return -1;
    }

    memset(&db->tablas[db->num_tablas], 0, sizeof(DefinicionTabla));
    strncpy(db->tablas[db->num_tablas].nombre, nombre_tabla, MAX_NOMBRE - 1);
    db->tablas[db->num_tablas].nombre[MAX_NOMBRE - 1] = '\0';
    for (int i = 0; i < num_campos && i < MAX_CAMPOS; i++) {
        db->tablas[db->num_tablas].campos[i] = campos[i];
    }
    db->tablas[db->num_tablas].num_campos = num_campos;
    db->tablas[db->num_tablas].modo = TABLA_DINAMICA;
    db->num_tablas++;

    DefinicionTabla* tabla_ptr = &db->tablas[db->num_tablas - 1];
    if (guardar_metadatos_tabla(db_actual, tabla_ptr) != 0) {
        printf("Warning: No se pudieron guardar los metadatos.\n");
    }

    printf("=> Tabla '%s' creada en '%s'.\n", nombre_tabla, db_actual);
    return 0;
}

int crear_tabla_desde_parser(const char* nombre_tabla, Campo* campos, int num_campos) {
    if (!db_actual[0]) {
        printf("Error: No hay base de datos seleccionada. Use USE <nombre>.\n");
        return -1;
    }

    DefinicionDB* db = NULL;
    for (int i = 0; i < num_databases; i++) {
        if (strcmp(databases[i].nombre, db_actual) == 0) {
            db = &databases[i];
            break;
        }
    }

    if (!db) {
        ultimo_error = ERR_BD_NO_EXISTE;
        return -1;
    }

    if (db->num_tablas >= MAX_TABLAS) {
        ultimo_error = ERR_DUPLICADO;
        return -1;
    }

    memset(&db->tablas[db->num_tablas], 0, sizeof(DefinicionTabla));
    strncpy(db->tablas[db->num_tablas].nombre, nombre_tabla, MAX_NOMBRE - 1);
    db->tablas[db->num_tablas].nombre[MAX_NOMBRE - 1] = '\0';
    for (int i = 0; i < num_campos && i < MAX_CAMPOS; i++) {
        db->tablas[db->num_tablas].campos[i] = campos[i];
    }
    db->tablas[db->num_tablas].num_campos = num_campos;
    db->tablas[db->num_tablas].modo = TABLA_DINAMICA;
    db->num_tablas++;

    DefinicionTabla* tabla_ptr = &db->tablas[db->num_tablas - 1];
    if (guardar_metadatos_tabla(db_actual, tabla_ptr) != 0) {
        printf("Warning: No se pudieron guardar los metadatos.\n");
    }

    printf("=> Tabla '%s' creada en '%s'.\n", nombre_tabla, db_actual);
    return 0;
}

DefinicionDB* obtener_db(char* nombre) {
    for (int i = 0; i < num_databases; i++) {
        if (strcmp(databases[i].nombre, nombre) == 0) {
            return &databases[i];
        }
    }
    return NULL;
}

DefinicionTabla* obtener_tabla(const char* db_nombre, const char* tabla_nombre) {
    DefinicionDB* db = obtener_db((char*)db_nombre);
    if (!db) return NULL;
    for (int i = 0; i < db->num_tablas; i++) {
        if (strcmp(db->tablas[i].nombre, tabla_nombre) == 0) {
            return &db->tablas[i];
        }
    }
    return NULL;
}

int es_tabla_dinamica(const char* db_nombre, const char* tabla_nombre) {
    DefinicionTabla* t = obtener_tabla(db_nombre, tabla_nombre);
    if (!t) return -1;
    return (t->modo == TABLA_DINAMICA) ? 1 : 0;
}

int listar_databases(DefinicionDB** resultado, int* cantidad) {
    *resultado = databases;
    *cantidad = num_databases;
    return 0;
}

int listar_tablas(const char* db, DefinicionTabla* resultado, int* cantidad) {
    DefinicionDB* d = obtener_db((char*)db);
    if (!d) {
        ultimo_error = ERR_BD_NO_EXISTE;
        return -1;
    }
    for (int i = 0; i < d->num_tablas && i < MAX_TABLAS; i++) {
        resultado[i] = d->tablas[i];
    }
    *cantidad = d->num_tablas;
    return 0;
}

int eliminar_database(char* nombre) {
    for (int i = 0; i < num_databases; i++) {
        if (strcmp(databases[i].nombre, nombre) == 0) {
            char cmd[100];
            sprintf(cmd, "rm -rf data/%s", nombre);
            system(cmd);

            memset(&databases[i], 0, sizeof(DefinicionDB));
            for (int j = i; j < num_databases - 1; j++) {
                databases[j] = databases[j + 1];
            }
            memset(&databases[num_databases - 1], 0, sizeof(DefinicionDB));
            num_databases--;
            if (strcmp(db_actual, nombre) == 0) {
                db_actual[0] = '\0';
            }
            printf("=> Base de datos '%s' eliminada.\n", nombre);
            return 0;
        }
    }
    ultimo_error = ERR_BD_NO_EXISTE;
    return -1;
}

int eliminar_tabla(const char* db_nombre, const char* tabla_nombre) {
    DefinicionDB* db = obtener_db((char*)db_nombre);
    if (!db) {
        ultimo_error = ERR_BD_NO_EXISTE;
        return -1;
    }
    for (int i = 0; i < db->num_tablas; i++) {
        if (strcmp(db->tablas[i].nombre, tabla_nombre) == 0) {
            char cmd[200];
            sprintf(cmd, "rm -f data/%s/%s.csv data/%s/%s.meta",
                    db_nombre, tabla_nombre, db_nombre, tabla_nombre);
            system(cmd);

            for (int j = i; j < db->num_tablas - 1; j++) {
                db->tablas[j] = db->tablas[j + 1];
            }
            db->num_tablas--;
            printf("=> Tabla '%s' eliminada de '%s'.\n", tabla_nombre, db_nombre);
            return 0;
        }
    }
    ultimo_error = ERR_TABLA_NO_EXISTE;
    return -1;
}

int table_exists(const char* db_nombre, const char* tabla_nombre) {
    return obtener_tabla(db_nombre, tabla_nombre) != NULL;
}