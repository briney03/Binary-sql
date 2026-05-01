#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

#include "motor.h"

static int en_transaccion = 0;
static LogEntry log_transaccion[MAX_LOG];
static int log_count = 0;
static char tabla_bloqueada[50] = {0};
static int fd_bloqueado = -1;

void iniciar_transaccion() {
    en_transaccion = 1;
    log_count = 0;
    printf("=> Transacción iniciada (START TRANSACTION)\n");
}

void registrar_en_log(char* tabla, int id, char* operacion, char* datos_previos) {
    if (log_count < MAX_LOG) {
        strcpy(log_transaccion[log_count].tabla, tabla);
        log_transaccion[log_count].id = id;
        strcpy(log_transaccion[log_count].operacion, operacion);
        memcpy(log_transaccion[log_count].datos_previos, datos_previos, 256);
        log_count++;
    }
}

void confirmar_transaccion() {
    if (!en_transaccion) {
        printf("Error: No hay transacción activa.\n");
        return;
    }

    char tablas_procesadas[3][50] = {"empleados", "clientes", "productos"};
    for (int i = 0; i < 3; i++) {
        char nombre_archivo[100];
        sprintf(nombre_archivo, "%s.dat", tablas_procesadas[i]);
        int fd = open(nombre_archivo, O_RDWR);
        if (fd != -1) {
            fsync(fd);
            close(fd);
        }
    }

    en_transaccion = 0;
    log_count = 0;
    printf("=> Transacción confirmada (COMMIT). Cambios persistidos en disco.\n");
}

void deshacer_transaccion() {
    if (!en_transaccion) {
        printf("Error: No hay transacción activa.\n");
        return;
    }
    for (int i = log_count - 1; i >= 0; i--) {
        char nombre_archivo[100];
        sprintf(nombre_archivo, "%s.dat", log_transaccion[i].tabla);
        int fd = open(nombre_archivo, O_RDWR);
        if (fd == -1) continue;

        TipoTabla tipo = get_tipo_tabla(log_transaccion[i].tabla);
        char* operacion = log_transaccion[i].operacion;

        if (strcmp(operacion, "INSERT") == 0) {
            off_t offset = buscar_en_indice(tipo, log_transaccion[i].id);
            if (offset >= 0) {
                if (strcmp(log_transaccion[i].tabla, "empleados") == 0) {
                    Empleado e = {0};
                    e.activo = 0;
                    lseek(fd, offset, SEEK_SET);
                    write(fd, &e, sizeof(Empleado));
                } else if (strcmp(log_transaccion[i].tabla, "clientes") == 0) {
                    Cliente c = {0};
                    c.activo = 0;
                    lseek(fd, offset, SEEK_SET);
                    write(fd, &c, sizeof(Cliente));
                } else if (strcmp(log_transaccion[i].tabla, "productos") == 0) {
                    Producto p = {0};
                    p.activo = 0;
                    lseek(fd, offset, SEEK_SET);
                    write(fd, &p, sizeof(Producto));
                }
            }
            eliminar_indice(tipo, log_transaccion[i].id);
            close(fd);
        }
        else if (strcmp(operacion, "UPDATE") == 0) {
            off_t offset = buscar_en_indice(tipo, log_transaccion[i].id);
            if (offset >= 0) {
                size_t tam = obtener_tamano_registro(log_transaccion[i].tabla);
                lseek(fd, offset, SEEK_SET);
                write(fd, log_transaccion[i].datos_previos, tam);
            }
            close(fd);
        }
        else if (strcmp(operacion, "DELETE") == 0) {
            if (strcmp(log_transaccion[i].tabla, "empleados") == 0) {
                Empleado* e = (Empleado*)log_transaccion[i].datos_previos;
                e->activo = 1;
                off_t offset = buscar_en_indice(tipo, log_transaccion[i].id);
                if (offset < 0) {
                    char nombre_idx[100];
                    sprintf(nombre_idx, "%s.idx", log_transaccion[i].tabla);
                    int fd_idx = open(nombre_idx, O_WRONLY | O_CREAT | O_APPEND, 0644);
                    if (fd_idx != -1) {
                        IndiceEntrada ent = { log_transaccion[i].id, 0 };
                        write(fd_idx, &ent, sizeof(IndiceEntrada));
                        close(fd_idx);
                    }
                }
            } else if (strcmp(log_transaccion[i].tabla, "clientes") == 0) {
                Cliente* c = (Cliente*)log_transaccion[i].datos_previos;
                c->activo = 1;
            } else if (strcmp(log_transaccion[i].tabla, "productos") == 0) {
                Producto* p = (Producto*)log_transaccion[i].datos_previos;
                p->activo = 1;
            }
            size_t tam = obtener_tamano_registro(log_transaccion[i].tabla);
            lseek(fd, 0, SEEK_SET);
            off_t offset = 0;
            char buffer[256];
            while (read(fd, buffer, tam) > 0) {
                int* id_ptr = (int*)buffer;
                if (*id_ptr == log_transaccion[i].id) {
                    lseek(fd, offset, SEEK_SET);
                    write(fd, log_transaccion[i].datos_previos, tam);
                    break;
                }
                offset += tam;
            }
            close(fd);
        }
        else if (strcmp(operacion, "RESTORE") == 0) {
            if (strcmp(log_transaccion[i].tabla, "empleados") == 0) {
                Empleado* e = (Empleado*)log_transaccion[i].datos_previos;
                e->activo = 0;
            } else if (strcmp(log_transaccion[i].tabla, "clientes") == 0) {
                Cliente* c = (Cliente*)log_transaccion[i].datos_previos;
                c->activo = 0;
            } else if (strcmp(log_transaccion[i].tabla, "productos") == 0) {
                Producto* p = (Producto*)log_transaccion[i].datos_previos;
                p->activo = 0;
            }
            size_t tam = obtener_tamano_registro(log_transaccion[i].tabla);
            lseek(fd, 0, SEEK_SET);
            off_t offset = 0;
            char buffer[256];
            while (read(fd, buffer, tam) > 0) {
                int* id_ptr = (int*)buffer;
                if (*id_ptr == log_transaccion[i].id) {
                    lseek(fd, offset, SEEK_SET);
                    write(fd, log_transaccion[i].datos_previos, tam);
                    break;
                }
                offset += tam;
            }
            eliminar_indice(tipo, log_transaccion[i].id);
            close(fd);
        }
        else {
            close(fd);
        }
    }
    en_transaccion = 0;
    log_count = 0;
    printf("=> Transacción cancelada (ROLLBACK). Cambios deshechos.\n");
}

int bloquear_tabla(char* tabla) {
    if (tabla_bloqueada[0] != '\0' && strcmp(tabla_bloqueada, tabla) != 0) {
        return 0;
    }
    char nombre_archivo[100];
    sprintf(nombre_archivo, "%s.dat", tabla);
    int fd = open(nombre_archivo, O_RDWR);
    if (fd == -1) return -1;

    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();

    if (fcntl(fd, F_SETLK, &lock) == -1) {
        close(fd);
        return -1;
    }
    strcpy(tabla_bloqueada, tabla);
    fd_bloqueado = fd;
    return fd;
}

void desbloquear_tabla(char* tabla) {
    if (strcmp(tabla_bloqueada, tabla) == 0 && fd_bloqueado != -1) {
        struct flock lock;
        lock.l_type = F_UNLCK;
        lock.l_whence = SEEK_SET;
        lock.l_start = 0;
        lock.l_len = 0;
        fcntl(fd_bloqueado, F_SETLK, &lock);
        close(fd_bloqueado);
        fd_bloqueado = -1;
        tabla_bloqueada[0] = '\0';
    }
}

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

size_t obtener_tamano_registro(char* tabla) {
    if (strcmp(tabla, "empleados") == 0) return sizeof(Empleado);
    if (strcmp(tabla, "clientes") == 0) return sizeof(Cliente);
    if (strcmp(tabla, "productos") == 0) return sizeof(Producto);
    return 0;
}

TipoTabla get_tipo_tabla(char* tabla) {
    if (strcmp(tabla, "empleados") == 0) return Empleados;
    if (strcmp(tabla, "clientes") == 0) return Clientes;
    if (strcmp(tabla, "productos") == 0) return Productos;
    return Ninguna;
}

char* get_nombre_tabla(TipoTabla tipo) {
    switch(tipo) {
        case Empleados: return "empleados";
        case Clientes: return "clientes";
        case Productos: return "productos";
        default: return "";
    }
}

void reconstruir_indice(TipoTabla tipo) {
    char tabla[50];
    strcpy(tabla, get_nombre_tabla(tipo));
    char nombre_idx[100], nombre_dat[100];
    sprintf(nombre_idx, "%s.idx", tabla);
    sprintf(nombre_dat, "%s.dat", tabla);

    int fd_dat = open(nombre_dat, O_RDONLY);
    if (fd_dat == -1) return;

    int fd_idx = open(nombre_idx, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_idx == -1) { close(fd_dat); return; }

    size_t tam = obtener_tamano_registro(tabla);
    char buffer[256];
    off_t offset = 0;

    while (read(fd_dat, buffer, tam) > 0) {
        int* id_ptr = (int*)buffer;
        IndiceEntrada entrada = { *id_ptr, offset };
        write(fd_idx, &entrada, sizeof(IndiceEntrada));
        offset += tam;
    }

    close(fd_idx);
    close(fd_dat);
}

off_t buscar_en_indice(TipoTabla tipo, int id) {
    char tabla[50];
    strcpy(tabla, get_nombre_tabla(tipo));
    char nombre_idx[100];
    sprintf(nombre_idx, "%s.idx", tabla);

    int fd = open(nombre_idx, O_RDONLY);
    if (fd == -1) return -1;

    IndiceEntrada entrada;
    while (read(fd, &entrada, sizeof(IndiceEntrada)) > 0) {
        if (entrada.id == id) {
            close(fd);
            return entrada.offset;
        }
    }

    close(fd);
    return -1;
}

void actualizar_indice_insert(TipoTabla tipo, int id, off_t offset) {
    char tabla[50];
    strcpy(tabla, get_nombre_tabla(tipo));
    char nombre_idx[100];
    sprintf(nombre_idx, "%s.idx", tabla);

    int fd = open(nombre_idx, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) return;

    IndiceEntrada entrada = { id, offset };
    write(fd, &entrada, sizeof(IndiceEntrada));
    close(fd);
}

void eliminar_indice(TipoTabla tipo, int id) {
    char tabla[50];
    strcpy(tabla, get_nombre_tabla(tipo));
    char nombre_idx[100], nombre_tmp[100];
    sprintf(nombre_idx, "%s.idx", tabla);
    sprintf(nombre_tmp, "%s.idx.tmp", tabla);

    int fd_orig = open(nombre_idx, O_RDONLY);
    if (fd_orig == -1) return;

    int fd_tmp = open(nombre_tmp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_tmp == -1) { close(fd_orig); return; }

    IndiceEntrada entrada;
    while (read(fd_orig, &entrada, sizeof(IndiceEntrada)) > 0) {
        if (entrada.id != id) {
            write(fd_tmp, &entrada, sizeof(IndiceEntrada));
        }
    }

    close(fd_orig);
    close(fd_tmp);
    rename(nombre_tmp, nombre_idx);
}

// --- Función dedicada a insertar Multi-Tabla ---
void ejecutar_insert(char* tabla, char* input) {
    char nombre_archivo[100];
    sprintf(nombre_archivo, "%s.dat", tabla);

    // Abrimos el archivo correspondiente a la tabla
    int fd = open(nombre_archivo, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("Error de Kernel: No se pudo abrir la BD");
        return;
    }

    // Truco de C: '%*s' significa "lee esta palabra pero ignórala"
    // Lo usamos para saltarnos la palabra "INSERT" y el nombre de la tabla
    
    TipoTabla tipo = get_tipo_tabla(tabla);

    if (strcmp(tabla, "empleados") == 0) {
        Empleado nuevo; nuevo.activo = 1;
        if (sscanf(input, "INSERT %*s %d %49s %f", &nuevo.id, nuevo.nombre, &nuevo.salario) == 3) {
            off_t offset = lseek(fd, 0, SEEK_END);
            write(fd, &nuevo, sizeof(Empleado));
            actualizar_indice_insert(tipo, nuevo.id, offset);
            if (en_transaccion) {
                registrar_en_log(tabla, nuevo.id, "INSERT", (char*)&nuevo);
            }
            printf("=> Empleado insertado en '%s.dat'.\n", tabla);
        } else {
            printf("Error sintaxis. Uso: INSERT empleados <id> <nombre> <salario>\n");
        }
    } 
    else if (strcmp(tabla, "clientes") == 0) {
        Cliente nuevo; nuevo.activo = 1;
        if (sscanf(input, "INSERT %*s %d %49s %14s", &nuevo.id, nuevo.nombre, nuevo.telefono) == 3) {
            off_t offset = lseek(fd, 0, SEEK_END);
            write(fd, &nuevo, sizeof(Cliente));
            actualizar_indice_insert(tipo, nuevo.id, offset);
            if (en_transaccion) {
                registrar_en_log(tabla, nuevo.id, "INSERT", (char*)&nuevo);
            }
            printf("=> Cliente insertado en '%s.dat'.\n", tabla);
        } else {
            printf("Error sintaxis. Uso: INSERT clientes <id> <nombre> <telefono>\n");
        }
    } 
    else if (strcmp(tabla, "productos") == 0) {
        Producto nuevo; nuevo.activo = 1;
        if (sscanf(input, "INSERT %*s %d %49s %f %d", &nuevo.id, nuevo.nombre, &nuevo.precio, &nuevo.stock) == 4) {
            off_t offset = lseek(fd, 0, SEEK_END);
            write(fd, &nuevo, sizeof(Producto));
            actualizar_indice_insert(tipo, nuevo.id, offset);
            if (en_transaccion) {
                registrar_en_log(tabla, nuevo.id, "INSERT", (char*)&nuevo);
            }
            printf("=> Producto insertado en '%s.dat'.\n", tabla);
        } else {
            printf("Error sintaxis. Uso: INSERT productos <id> <nombre> <precio> <stock>\n");
        }
    } 
    else {
        printf("Error: La tabla '%s' no existe en el esquema.\n", tabla);
    }

    close(fd);
}

// --- Función UPDATE Multi-Tabla ---
void ejecutar_update(char* tabla, char* input) {
    char nombre_archivo[100];
    sprintf(nombre_archivo, "%s.dat", tabla);

    int fd = open(nombre_archivo, O_RDWR);
    if (fd == -1) {
        printf("Error: No se pudo abrir la tabla '%s'.\n", tabla);
        return;
    }

    int target_id = 0;
    float nuevo_salario_f = 0;
    char nuevo_telefono[15] = {0};
    float nuevo_precio_f = 0;
    int nuevo_stock_i = 0;
    int parsed = 0;

    if (strcmp(tabla, "empleados") == 0) {
        parsed = sscanf(input, "UPDATE %*s %d %f", &target_id, &nuevo_salario_f);
    } else if (strcmp(tabla, "clientes") == 0) {
        parsed = sscanf(input, "UPDATE %*s %d %14s", &target_id, nuevo_telefono);
    } else if (strcmp(tabla, "productos") == 0) {
        parsed = sscanf(input, "UPDATE %*s %d %f %d", &target_id, &nuevo_precio_f, &nuevo_stock_i);
    }

    if (parsed < 2) {
        printf("Error: Sintaxis incorrecta para UPDATE.\n");
        close(fd);
        return;
    }

    bool encontrado = false;
    char buffer[256]; 
    size_t tam = obtener_tamano_registro(tabla);
    TipoTabla tipo = get_tipo_tabla(tabla);

    off_t offset_idx = buscar_en_indice(tipo, target_id);
    if (offset_idx >= 0) {
        lseek(fd, offset_idx, SEEK_SET);
        read(fd, buffer, tam);

        if (en_transaccion) {
            registrar_en_log(tabla, target_id, "UPDATE", buffer);
        }

        if (strcmp(tabla, "empleados") == 0) {
            Empleado* e = (Empleado*)buffer;
            if (e->activo == 1) {
                e->salario = nuevo_salario_f;
                lseek(fd, offset_idx, SEEK_SET);
                write(fd, e, tam);
                encontrado = true;
            }
        }
        else if (strcmp(tabla, "clientes") == 0) {
            Cliente* c = (Cliente*)buffer;
            if (c->activo == 1) {
                strcpy(c->telefono, nuevo_telefono);
                lseek(fd, offset_idx, SEEK_SET);
                write(fd, c, tam);
                encontrado = true;
            }
        }
        else if (strcmp(tabla, "productos") == 0) {
            Producto* p = (Producto*)buffer;
            if (p->activo == 1) {
                p->precio = nuevo_precio_f;
                p->stock = nuevo_stock_i;
                lseek(fd, offset_idx, SEEK_SET);
                write(fd, p, tam);
                encontrado = true;
            }
        }
    }

    if (encontrado) printf("=> Registro actualizado con éxito en '%s'.\n", tabla);
    else if (tam > 0) printf("Error: ID %d no encontrado o no activo en '%s'.\n", target_id, tabla);

    close(fd);
}

// --- Función DELETE y RESTORE combinada lógicamente ---
void ejecutar_cambio_estado(char* tabla, char* input, int activar) {
    int target_id;
    // %*s ignora la palabra DELETE/RESTORE, el otro %*s ignora el nombre de la tabla
    if (sscanf(input, "%*s %*s %d", &target_id) != 1) {
        printf("Uso incorrecto. Asegúrate de mandar: COMANDO tabla <id>\n");
        return;
    }

    char nombre_archivo[100];
    sprintf(nombre_archivo, "%s.dat", tabla);
    int fd = open(nombre_archivo, O_RDWR);
    if (fd == -1) { printf("Error al abrir BD.\n"); return; }

    char buffer[256];
    size_t tam = obtener_tamano_registro(tabla);
    bool encontrado = false;
    TipoTabla tipo = get_tipo_tabla(tabla);

    off_t offset_idx = buscar_en_indice(tipo, target_id);
    if (offset_idx >= 0) {
        lseek(fd, offset_idx, SEEK_SET);
        read(fd, buffer, tam);
    } else {
        off_t offset = 0;
        while (read(fd, buffer, tam) > 0) {
            int* id_ptr = (int*)buffer;
            if (*id_ptr == target_id) {
                offset_idx = offset;
                break;
            }
            offset += tam;
        }
        if (offset_idx >= 0) {
            lseek(fd, offset_idx, SEEK_SET);
            read(fd, buffer, tam);
        }
    }

    if (offset_idx >= 0) {
        if (strcmp(tabla, "empleados") == 0) {
            Empleado* e = (Empleado*)buffer;
            if (e->activo == !activar) {
                if (en_transaccion) {
                    registrar_en_log(tabla, target_id, activar ? "RESTORE" : "DELETE", buffer);
                }
                e->activo = activar;
                lseek(fd, offset_idx, SEEK_SET);
                write(fd, e, tam);
                if (!activar) eliminar_indice(tipo, target_id);
                else actualizar_indice_insert(tipo, target_id, offset_idx);
                encontrado = true;
            }
        }
        else if (strcmp(tabla, "clientes") == 0) {
            Cliente* c = (Cliente*)buffer;
            if (c->activo == !activar) {
                if (en_transaccion) {
                    registrar_en_log(tabla, target_id, activar ? "RESTORE" : "DELETE", buffer);
                }
                c->activo = activar;
                lseek(fd, offset_idx, SEEK_SET);
                write(fd, c, tam);
                if (!activar) eliminar_indice(tipo, target_id);
                else actualizar_indice_insert(tipo, target_id, offset_idx);
                encontrado = true;
            }
        }
        else if (strcmp(tabla, "productos") == 0) {
            Producto* p = (Producto*)buffer;
            if (p->activo == !activar) {
                if (en_transaccion) {
                    registrar_en_log(tabla, target_id, activar ? "RESTORE" : "DELETE", buffer);
                }
                p->activo = activar;
                lseek(fd, offset_idx, SEEK_SET);
                write(fd, p, tam);
                if (!activar) eliminar_indice(tipo, target_id);
                else actualizar_indice_insert(tipo, target_id, offset_idx);
                encontrado = true;
            }
        }
    }

    if (encontrado) printf("=> ID %d %s exitosamente en '%s'.\n", target_id, activar ? "restaurado" : "eliminado", tabla);
    else printf("Error: ID %d no aplicable en '%s'.\n", target_id, tabla);

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
        
        // Aquí buscamos que el ID coincida y que esté INACTIVO (0)
        if (reg.id == target_id && reg.activo == 0) {
            encontrado = true;
            
            // Lo volvemos a la vida
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
void ejecutar_select(char* tabla) {
    size_t tam = obtener_tamano_registro(tabla);
    if (tam == 0) {
        printf("Error: La tabla '%s' no existe.\n", tabla);
        return;
    }

    char nombre_archivo[100];
    sprintf(nombre_archivo, "%s.dat", tabla);

    int fd = open(nombre_archivo, O_RDONLY);
    if (fd == -1) {
        printf("Info: La tabla '%s' está vacía.\n", tabla);
        return;
    }

    printf("\n--- CONTENIDO DE LA TABLA: %s ---\n", tabla);
    
    // Usamos un buffer de bytes genérico para leer cualquier struct
    char buffer[256]; 
    while (read(fd, buffer, tam) > 0) {
        // Para mostrar los datos, necesitamos "castear" el buffer al tipo correcto
        if (strcmp(tabla, "empleados") == 0) {
            Empleado* e = (Empleado*)buffer;
            if (e->activo) printf("ID: %d | Nombre: %s | Salario: %.2f\n", e->id, e->nombre, e->salario);
        }
        else if (strcmp(tabla, "clientes") == 0) {
            Cliente* c = (Cliente*)buffer;
            if (c->activo) printf("ID: %d | Nombre: %s | Tel: %s\n", c->id, c->nombre, c->telefono);
        }
        else if (strcmp(tabla, "productos") == 0) {
            Producto* p = (Producto*)buffer;
            if (p->activo) printf("ID: %d | Prod: %s | Precio: %.2f | Stock: %d\n", p->id, p->nombre, p->precio, p->stock);
        }
    }
    close(fd);
}

// --- EL ÚNICO Analizador (Parser) ---
void parse_and_execute(char* input) {
    char comando[20];
    char tabla[50];
    
    // Leemos las primeras dos palabras (Ej. "SELECT" y "empleados")
    int leidos = sscanf(input, "%19s %49s", comando, tabla);

    // Si el usuario presiona Enter sin escribir nada
    if (leidos == 0) return;

    if (strcmp(comando, "exit") == 0) {
        printf("Cerrando el motor de base de datos... Adiós.\n");
        exit(EXIT_SUCCESS);
    } 
    else if (strcmp(comando, "START") == 0) {
        if (strcmp(tabla, "TRANSACTION") == 0 || strcmp(tabla, "TRANSACTION;") == 0) {
            iniciar_transaccion();
        } else {
            printf("Error: Use 'START TRANSACTION' para iniciar una transacción.\n");
        }
        return;
    }
    else if (strcmp(comando, "COMMIT") == 0) {
        confirmar_transaccion();
        return;
    }
    else if (strcmp(comando, "ROLLBACK") == 0) {
        deshacer_transaccion();
        return;
    }
    else if (strcmp(comando, "REINDEX") == 0) {
        TipoTabla tipo = get_tipo_tabla(tabla);
        if (tipo != Ninguna) {
            reconstruir_indice(tipo);
            printf("=> Índice de '%s' reconstruido.\n", tabla);
        } else {
            printf("Error: Tabla '%s' no válida.\n", tabla);
        }
        return;
    }

    // Si escribió un comando pero olvidó poner la tabla
    if (leidos < 2) {
        printf("Error: Falta especificar el nombre de la tabla.\n");
        return;
    }

    // Rutas de ejecución pasando la tabla detectada
    if (strcmp(comando, "SELECT") == 0) {
        ejecutar_select(tabla); 
    }
    else if (strcmp(comando, "INSERT") == 0) {
        // Al INSERT le pasamos la tabla y el texto completo, porque tiene más datos
        ejecutar_insert(tabla, input);
    }
    else if (strcmp(comando, "UPDATE") == 0) {
        ejecutar_update(tabla, input);
    }
    else if (strcmp(comando, "DELETE") == 0) {
        ejecutar_cambio_estado(tabla, input, 0); // 0 significa "apagar" o borrar
    }
    else if (strcmp(comando, "RESTORE") == 0) {
        ejecutar_cambio_estado(tabla, input, 1); // 1 significa "encender" o restaurar
    }
    else {
        printf("Error sintáctico: Comando '%s' no reconocido.\n", comando);
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