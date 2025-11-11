#include "posix_utils.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

// Abre un archivo para lectura
int posix_open_read(const char* path) {
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Cannot open '%s' for reading: %s\n", path, strerror(errno));
    }
    return fd;
}

// Abre un archivo para escritura (crea o trunca)
int posix_open_write(const char* path) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, FILE_MODE);
    if (fd == -1) {
        fprintf(stderr, "Cannot open '%s' for writing: %s\n", path, strerror(errno));
    }
    return fd;
}

// Lee exactamente 'count' bytes (maneja EINTR y lecturas parciales)
ssize_t posix_read_full(int fd, void* buf, size_t count) {
    size_t total = 0;
    unsigned char* ptr = (unsigned char*)buf;
    
    while (total < count) {
        ssize_t n = read(fd, ptr + total, count - total);
        
        if (n == -1) {
            // Interrumpido por señal, reintentar
            if (errno == EINTR) continue;
            // Error real
            fprintf(stderr, "Read error: %s\n", strerror(errno));
            return -1;
        }
        
        // EOF alcanzado antes de leer todo
        if (n == 0) break;
        
        total += n;
    }
    
    return (ssize_t)total;
}

// Escribe exactamente 'count' bytes (maneja EINTR y escrituras parciales)
ssize_t posix_write_full(int fd, const void* buf, size_t count) {
    size_t total = 0;
    const unsigned char* ptr = (const unsigned char*)buf;
    
    while (total < count) {
        ssize_t n = write(fd, ptr + total, count - total);
        
        if (n == -1) {
            // Interrumpido por señal, reintentar
            if (errno == EINTR) continue;
            // Error real
            fprintf(stderr, "Write error: %s\n", strerror(errno));
            return -1;
        }
        
        total += n;
    }
    
    return (ssize_t)total;
}

// Obtiene el tamaño del archivo usando fstat
off_t posix_get_file_size(int fd) {
    struct stat st;
    
    if (fstat(fd, &st) == -1) {
        fprintf(stderr, "fstat error: %s\n", strerror(errno));
        return -1;
    }
    
    return st.st_size;
}

// Verifica si es un archivo regular
int posix_is_regular_file(int fd) {
    struct stat st;
    
    if (fstat(fd, &st) == -1) {
        fprintf(stderr, "fstat error: %s\n", strerror(errno));
        return -1;
    }
    
    return S_ISREG(st.st_mode) ? 1 : 0;
}

// Cierra file descriptor de forma segura
int posix_close(int fd) {
    if (fd < 0) {
        return 0; // Ya está cerrado o no válido
    }
    
    if (close(fd) == -1) {
        fprintf(stderr, "Warning: close failed: %s\n", strerror(errno));
        return -1;
    }
    
    return 0;
}
