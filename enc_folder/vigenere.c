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

int main() {
    char text[256], key[256];
    char encrypted[256], decrypted[256];

    printf("Texto a cifrar: ");
    fgets(text, sizeof(text), stdin);
    text[strcspn(text, "\n")] = 0; // eliminar salto de línea

    printf("Clave: ");
    fgets(key, sizeof(key), stdin);
    key[strcspn(key, "\n")] = 0;

    vigenere_encrypt(text, key, encrypted);
    vigenere_decrypt(encrypted, key, decrypted);

    printf("\nTexto original: %s\n", text);
    printf("Texto cifrado (en bytes): ");
    for (int i = 0; i < strlen(encrypted); i++)
        printf("%d ", (unsigned char)encrypted[i]);
    printf("\n");

    printf("Texto descifrado: %s\n", decrypted);

    return 0;
}
