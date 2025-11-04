#ifndef VIGENERE_H
#define VIGENERE_H

#include <stddef.h>
#include "../common.h"

// Nuevas funciones que trabajan con bytes en lugar de texto
int vigenere_encrypt_file(const char* inputPath, const char* outputPath, const char* key);
int vigenere_decrypt_file(const char* inputPath, const char* outputPath, const char* key);

#endif