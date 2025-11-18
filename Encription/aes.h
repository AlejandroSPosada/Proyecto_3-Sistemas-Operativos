#ifndef AES_H
#define AES_H

#include <stddef.h>
#include <stdint.h>
#include "../common.h"


// Tamaño de clave: 256 bits (32 bytes)
// Tamaño de bloque: 128 bits (16 bytes)
// Modo: ECB (Electronic Codebook) - implementación sencilla
// Relleno: PKCS7
// Formato del archivo:
// FileMetadata (272 bytes)
// Bloques de datos cifrados (múltiplos de 16 bytes)

#define AES_BLOCK_SIZE 16
#define AES_KEY_SIZE 32    // 256 bits

/**
 * Cifra un archivo usando AES-256-ECB
 * 
 * @param inputPath Ruta del archivo de entrada
 * @param outputPath Ruta del archivo cifrado de salida
 * @param password Contraseña para el cifrado (se convertirá a una clave de 256 bits)
 * @return si tiene exito, -1 en caso de error
 */
int aes_encrypt_file(const char* inputPath, const char* outputPath, const char* password);

/**
 * Descifra un archivo usando AES-256-ECB
 * 
 * @param inputPath Ruta del archivo cifrado de entrada
 * @param outputPath Ruta del archivo descifrado de salida
 * @param password Contraseña para el descifrado
 * @return si tiene éxito, -1 en caso de error
 */
int aes_decrypt_file(const char* inputPath, const char* outputPath, const char* password);

#endif
