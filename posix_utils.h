#ifndef POSIX_UTILS_H
#define POSIX_UTILS_H

#include <sys/types.h>
#include <stddef.h>

// Permisos estándar para archivos nuevos: rw-r--r-- (0644)
#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

/**
 * Abre un archivo para lectura (O_RDONLY)
 * @param path Ruta del archivo
 * @return File descriptor o -1 en error
 */
int posix_open_read(const char* path);

/**
 * Abre un archivo para escritura (O_WRONLY | O_CREAT | O_TRUNC)
 * @param path Ruta del archivo
 * @return File descriptor o -1 en error
 */
int posix_open_write(const char* path);

/**
 * Lee exactamente 'count' bytes del fd (maneja lecturas parciales e EINTR)
 * @param fd File descriptor
 * @param buf Buffer de destino
 * @param count Número de bytes a leer
 * @return Número de bytes leídos, 0 en EOF, -1 en error
 */
ssize_t posix_read_full(int fd, void* buf, size_t count);

/**
 * Escribe exactamente 'count' bytes al fd (maneja escrituras parciales e EINTR)
 * @param fd File descriptor
 * @param buf Buffer de origen
 * @param count Número de bytes a escribir
 * @return Número de bytes escritos, -1 en error
 */
ssize_t posix_write_full(int fd, const void* buf, size_t count);

/**
 * Obtiene el tamaño de un archivo usando fstat
 * @param fd File descriptor
 * @return Tamaño del archivo o -1 en error
 */
off_t posix_get_file_size(int fd);

/**
 * Verifica si el path es un archivo regular
 * @param fd File descriptor
 * @return 1 si es regular, 0 si no, -1 en error
 */
int posix_is_regular_file(int fd);

/**
 * Cierra un file descriptor de forma segura (verifica >= 0)
 * @param fd File descriptor
 * @return 0 en éxito, -1 en error
 */
int posix_close(int fd);

#endif // POSIX_UTILS_H
