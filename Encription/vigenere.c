#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "vigenere.h"
#include "../posix_utils.h"

#define BUFFER_SIZE 8192

static void process_bytes(unsigned char* data, size_t length, const unsigned char* key, size_t keyLen, int encrypt) {
    for (size_t i = 0; i < length; i++) {
        unsigned char k = key[i % keyLen];
        if (encrypt) {
            // Cifrado: (byte + clave) mod 256
            data[i] = (data[i] + k) % 256;
        } else {
            // Descifrado: (byte - clave + 256) mod 256
            data[i] = (data[i] - k + 256) % 256;
        }
    }
}

static int process_file(const char* inputPath, const char* outputPath, const char* key, int encrypt) {
    // Abrir archivo de entrada con POSIX
    int fd_input = posix_open_read(inputPath);
    if (fd_input == -1) return 1;

    // Obtener tamaño del archivo
    off_t fileSize = posix_get_file_size(fd_input);
    if (fileSize < 0) {
        posix_close(fd_input);
        return 1;
    }

    // Abrir archivo de salida con POSIX
    int fd_output = posix_open_write(outputPath);
    if (fd_output == -1) {
        posix_close(fd_input);
        return 1;
    }

    if (encrypt) {
        // Escribir metadata para archivos encriptados
        FileMetadata meta = {
            .magic = METADATA_MAGIC,
            .originalSize = fileSize,
            .flags = 0  // Podría agregar flags para método de encriptación
        };
        strncpy(meta.originalName, get_basename(inputPath), MAX_FILENAME_LEN-1);
        meta.originalName[MAX_FILENAME_LEN-1] = '\0';
        
        if (posix_write_full(fd_output, &meta, sizeof(meta)) != sizeof(meta)) {
            fprintf(stderr, "Falló escritura de metadatos\n");
            posix_close(fd_input);
            posix_close(fd_output);
            return 1;
        }
    } else {
        // Leer y verificar metadata para desencriptación
        FileMetadata meta;
        if (posix_read_full(fd_input, &meta, sizeof(meta)) != sizeof(meta) ||
            meta.magic != METADATA_MAGIC) {
            fprintf(stderr, "Archivo encriptado inválido o corrupto\n");
            posix_close(fd_input);
            posix_close(fd_output);
            return 1;
        }
        fileSize -= sizeof(meta);  // Ajustar por tamaño de metadata
    }

    // Procesar archivo en bloques
    unsigned char buffer[BUFFER_SIZE];
    size_t keyLen = strlen(key);
    
    while (1) {
        ssize_t bytes_read = read(fd_input, buffer, BUFFER_SIZE);
        if (bytes_read == -1) {
            if (errno == EINTR) continue;
            fprintf(stderr, "Error de lectura: %s\n", strerror(errno));
            posix_close(fd_input);
            posix_close(fd_output);
            return 1;
        }
        if (bytes_read == 0) break;  // EOF
        
        process_bytes(buffer, bytes_read, (const unsigned char*)key, keyLen, encrypt);
        
        if (posix_write_full(fd_output, buffer, bytes_read) != bytes_read) {
            fprintf(stderr, "Error de escritura\n");
            posix_close(fd_input);
            posix_close(fd_output);
            return 1;
        }
    }

    posix_close(fd_input);
    posix_close(fd_output);
    return 0;
}

int vigenere_encrypt_file(const char* inputPath, const char* outputPath, const char* key) {
    return process_file(inputPath, outputPath, key, 1);
}

int vigenere_decrypt_file(const char* inputPath, const char* outputPath, const char* key) {
    return process_file(inputPath, outputPath, key, 0);
}
