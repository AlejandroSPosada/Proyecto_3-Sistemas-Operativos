#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#define MAX_FILENAME_LEN 256 // Nomvre de archivo maximo (256 caracteres)
#define METADATA_MAGIC 0x4D435046  // "MCPF" en hex (Magic Compressed/Protected File), funciona como firma de archivos creados por el programa.

// Estructura comun para metadatos de archivo
typedef struct {
    uint32_t magic;              // Guarda la firma para validar que el archivo fue creado por el programa
    uint64_t originalSize;       // Tamaño original del archivo
    char originalName[MAX_FILENAME_LEN];  // Nombre original completo
    uint32_t flags;             // Banderas para uso futuro (compresion, encriptacion, etc)
} FileMetadata;

// Funciones auxiliares para leer/escribir archivos binarios
static inline const char* get_extension(const char* filename) {
    const char* dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot + 1;
} // Tambien adquiere el tipo de archivo del especificado

static inline const char* get_basename(const char* path) { // Devuelve el nombre del archivo sin la ruta
    const char* lastSlash = strrchr(path, '/');
    return lastSlash ? lastSlash + 1 : path;
}

#endif