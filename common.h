#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#define MAX_FILENAME_LEN 256
#define METADATA_MAGIC 0x4D435046  // "MCPF" en hex (Magic Compressed/Protected File)

// Estructura común para metadatos de archivo
typedef struct {
    uint32_t magic;              // Número mágico para validación
    uint64_t originalSize;       // Tamaño original del archivo
    char originalName[MAX_FILENAME_LEN];  // Nombre original completo
    uint32_t flags;             // Banderas para uso futuro (compresión, encriptación, etc)
} FileMetadata;

// Funciones auxiliares para leer/escribir archivos binarios
static inline const char* get_extension(const char* filename) {
    const char* dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot + 1;
}

static inline const char* get_basename(const char* path) {
    const char* lastSlash = strrchr(path, '/');
    return lastSlash ? lastSlash + 1 : path;
}

#endif