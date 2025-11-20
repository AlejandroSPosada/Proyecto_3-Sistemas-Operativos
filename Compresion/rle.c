#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "rle.h"
#include "../common.h"
#include "../posix_utils.h"

#define MAX_RUN_LENGTH 0xFFFFFFFF  // Longitud de ejecución máxima para uint32_t

/**
 * Codificación de Longitud de Ejecución Compresión (VERSIÓN POSIX)
 * Codifica ejecuciones consecutivas del mismo byte como pares [count][byte]
 * Usa uint32_t para contar para manejar ejecuciones largas de manera eficiente
 */
void writeRLE(char inputFile[], char outputFile[]) {
    // Abrir archivo de entrada con POSIX
    int fd_input = posix_open_read(inputFile);
    if (fd_input == -1) {
        return;
    }

    // Obtener tamaño del archivo con fstat
    off_t fileSize = posix_get_file_size(fd_input);
    if (fileSize <= 0) {
        fprintf(stderr, "Archivo '%s' está vacío o es inválido\n", inputFile);
        posix_close(fd_input);
        return;
    }

    // Verificar que es archivo regular
    if (posix_is_regular_file(fd_input) != 1) {
        fprintf(stderr, "'%s' no es un archivo regular\n", inputFile);
        posix_close(fd_input);
        return;
    }

    // Allocar buffer para todo el archivo
    unsigned char* data = (unsigned char*)malloc(fileSize);
    if (!data) {
        fprintf(stderr, "Falló asignación de memoria: %s\n", strerror(errno));
        posix_close(fd_input);
        return;
    }
    
    // Leer archivo completo con posix_read_full
    ssize_t bytes_read = posix_read_full(fd_input, data, fileSize);
    posix_close(fd_input);

    if (bytes_read != fileSize) {
        fprintf(stderr, "Falló lectura de archivo completo: esperado %ld, obtenido %ld bytes\n",
                (long)fileSize, (long)bytes_read);
        free(data);
        return;
    }

    // Abrir archivo de salida con POSIX
    int fd_output = posix_open_write(outputFile);
    if (fd_output == -1) {
        free(data);
        return;
    }

    // Escribir metadata header
    FileMetadata meta = {0};
    meta.magic = METADATA_MAGIC;
    meta.originalSize = (uint64_t)bytes_read;
    meta.flags = 0;
    strncpy(meta.originalName, get_basename(inputFile), MAX_FILENAME_LEN-1);
    meta.originalName[MAX_FILENAME_LEN-1] = '\0';
    
    if (posix_write_full(fd_output, &meta, sizeof(meta)) != sizeof(meta)) {
        fprintf(stderr, "Falló escritura de metadatos\n");
        posix_close(fd_output);
        free(data);
        return;
    }

    // RLE encoding
    size_t i = 0;
    while (i < (size_t)bytes_read) {
        unsigned char currentByte = data[i];
        uint32_t count = 1;
        
        // Contar ocurrencias consecutivas del mismo byte
        while (i + count < (size_t)bytes_read && 
               data[i + count] == currentByte && 
               count < MAX_RUN_LENGTH) {
            count++;
        }
        
        // Escribir par [count][byte] usando POSIX
        if (posix_write_full(fd_output, &count, sizeof(uint32_t)) != sizeof(uint32_t)) {
            fprintf(stderr, "Falló escritura de conteo\n");
            posix_close(fd_output);
            free(data);
            return;
        }
        
        if (posix_write_full(fd_output, &currentByte, sizeof(unsigned char)) != sizeof(unsigned char)) {
            fprintf(stderr, "Falló escritura de byte\n");
            posix_close(fd_output);
            free(data);
            return;
        }
        
        i += count;
    }

    posix_close(fd_output);
    free(data);
}

/**
 * Codificación de Longitud de Ejecución Descompresión (VERSIÓN POSIX)
 * Lee pares [count][byte] y los expande a datos originales
 */
int readRLE(char inputFile[], char outputFile[]) {
    // Abrir archivo comprimido con POSIX
    int fd_input = posix_open_read(inputFile);
    if (fd_input == -1) {
        return 1;
    }

    // Leer metadata header
    FileMetadata meta;
    if (posix_read_full(fd_input, &meta, sizeof(meta)) != sizeof(meta) || 
        meta.magic != METADATA_MAGIC) {
        fprintf(stderr, "Metadatos faltantes o inválidos en archivo comprimido\n");
        posix_close(fd_input);
        return 1;
    }

    uint64_t originalSize = meta.originalSize;
    if (originalSize == 0 || originalSize > 1024ULL * 1024 * 1024 * 10) { // Max 10GB sanity check
        fprintf(stderr, "Tamaño original inválido o demasiado grande: %llu\n", 
                (unsigned long long)originalSize);
        posix_close(fd_input);
        return 1;
    }

    // Allocar buffer de salida
    unsigned char* output = (unsigned char*)malloc(originalSize);
    if (!output) {
        fprintf(stderr, "Falló asignación de memoria: %s\n", strerror(errno));
        posix_close(fd_input);
        return 1;
    }

    // Descomprimir datos RLE
    size_t outputPos = 0;
    while (outputPos < originalSize) {
        uint32_t count;
        unsigned char byte;
        
        // Leer count
        ssize_t n = posix_read_full(fd_input, &count, sizeof(uint32_t));
        if (n == 0) break;  // EOF alcanzado
        if (n != sizeof(uint32_t)) {
            fprintf(stderr, "Falló lectura de conteo\n");
            free(output);
            posix_close(fd_input);
            return 1;
        }
        
        // Leer byte
        if (posix_read_full(fd_input, &byte, sizeof(unsigned char)) != sizeof(unsigned char)) {
            fprintf(stderr, "Falló lectura de byte\n");
            free(output);
            posix_close(fd_input);
            return 1;
        }
        
        // Validar que count no exceda el espacio restante
        if (outputPos + count > originalSize) {
            fprintf(stderr, "Corrupción de datos RLE: ejecución excede tamaño original\n");
            free(output);
            posix_close(fd_input);
            return 1;
        }
        
        // Expandir el run
        for (uint32_t i = 0; i < count; i++) {
            output[outputPos++] = byte;
        }
    }
    
    posix_close(fd_input);

    // Verificar que obtuvimos la cantidad esperada de datos
    if (outputPos != originalSize) {
        fprintf(stderr, "Discrepancia de tamaño: esperado %llu, obtenido %zu bytes\n", 
                (unsigned long long)originalSize, outputPos);
        free(output);
        return 1;
    }

    // Escribir datos descomprimidos al archivo de salida con POSIX
    int fd_output = posix_open_write(outputFile);
    if (fd_output == -1) {
        free(output);
        return 1;
    }
    
    ssize_t written = posix_write_full(fd_output, output, originalSize);
    posix_close(fd_output);
    
    if (written != (ssize_t)originalSize) {
        fprintf(stderr, "Falló escritura de salida completa\n");
        free(output);
        return 1;
    }

    free(output);
    return 0;
}
