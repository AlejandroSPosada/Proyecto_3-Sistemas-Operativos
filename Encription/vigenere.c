#include <stdio.h>
#include <string.h>

#include "vigenere.h"

// Cifra usando Vigenère (para todo el ASCII)
void vigenere_encrypt(char *text, char *key, char *result) {
    int textLen = strlen(text);
    int keyLen = strlen(key);

    for (int i = 0; i < textLen; i++) {
        // Cifrado: (texto + clave) mod 256
        result[i] = (text[i] + key[i % keyLen]) % 256;
    }
    result[textLen] = '\0';
}

// Descifra usando Vigenère (para todo el ASCII)
void vigenere_decrypt(char *cipher, char *key, char *result) {
    int textLen = strlen(cipher);
    int keyLen = strlen(key);

    for (int i = 0; i < textLen; i++) {
        // Descifrado: (texto - clave + 256) mod 256
        result[i] = (cipher[i] - key[i % keyLen] + 256) % 256;
    }
    result[textLen] = '\0';
}

