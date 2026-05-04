#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "../includes/config.h"
#include "../includes/types.h"
#include "../includes/index.h"

off_t buscar_en_indice(const char* tabla, int id) {
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

void actualizar_indice_insert(const char* tabla, int id, off_t offset) {
    char nombre_idx[100];
    sprintf(nombre_idx, "%s.idx", tabla);

    int fd = open(nombre_idx, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) return;

    IndiceEntrada entrada = { id, offset };
    write(fd, &entrada, sizeof(IndiceEntrada));
    close(fd);
}

void eliminar_indice(const char* tabla, int id) {
    char nombre_idx[100];
    char nombre_tmp[100];
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

void reconstruir_indice(const char* tabla) {
    char nombre_idx[100];
    char nombre_dat[100];
    sprintf(nombre_idx, "%s.idx", tabla);
    sprintf(nombre_dat, "%s.dat", tabla);

    int fd_dat = open(nombre_dat, O_RDONLY);
    if (fd_dat == -1) return;

    int fd_idx = open(nombre_idx, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_idx == -1) { close(fd_dat); return; }

    char meta_path[200];
    sprintf(meta_path, "data/%s.meta", tabla);
    FILE* meta_fd = fopen(meta_path, "r");
    if (meta_fd) {
        fclose(meta_fd);
    }

    IndiceEntrada entrada;
    char buffer[1024];
    off_t offset = 0;

    while (read(fd_dat, buffer, sizeof(buffer)) > 0) {
        int* id_ptr = (int*)buffer;
        entrada.id = *id_ptr;
        entrada.offset = offset;
        write(fd_idx, &entrada, sizeof(IndiceEntrada));
        offset += sizeof(buffer);
    }

    close(fd_idx);
    close(fd_dat);
}