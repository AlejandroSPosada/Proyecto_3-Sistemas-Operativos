#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <errno.h>

#include "huffman.h"
#include "rle.h"
#include "vigenere.h"
#include "lzw.h"
#include "aes.h"
#include "common.h"
// Mensaje de ayuda en la consola para el uso del programa
static void usage(const char* prog) {
    fprintf(stderr,
        "Uso: %s [ops] [opciones]\n\n"
        "Operaciones (combinables):\n"
        "  -c    Comprimir\n"
        "  -d    Descomprimir\n"
        "  -e    Encriptar (Vigenère)\n"
        "  -u    Desencriptar (Vigenère)\n"
        "  -ce   Comprimir y luego encriptar\n"
        "  -ud   Desencriptar y luego descomprimir (inverso de -ce)\n\n"
        "Opciones:\n"
    "  --comp-alg [nombre]   Algoritmo de compresión (huffman, rle, lzw)\n"
        "  --enc-alg  [nombre]   Algoritmo de encriptación (vigenere, aes)\n"
        "  -i [ruta]             Archivo de entrada\n"
        "  -o [ruta]             Archivo de salida\n"
        "  -k [clave]            Clave para encriptar/desencriptar\n\n",
        prog);
}
// Verificar si la ruta es un directorio
static bool is_dir(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}
// Verificar si el archivo existe (no para directorios)
static bool file_exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}
// Mover resultados a carpeta File_Manager con el nombre dado
static int move_file(const char* src, const char* dst) {
    if (rename(src, dst) == 0) return 0;
    perror("rename");
    return -1;
}

int main(int argc, char** argv) {
    bool op_c = false, op_d = false, op_e = false, op_u = false; // Banderas para la accion a realizar (Comprimir, descomprimir...)
    const char* compAlg = "huffman";
    const char* encAlg  = "vigenere";
    const char* inPath = NULL;
    const char* outPath = NULL;
    const char* key = NULL;

    if (argc <= 1) {
        usage(argv[0]);
        return 1;
    }
    // Analisis de lo escrito en consola
    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];
        if (arg[0] == '-' && arg[1] == '-' ) { 
            if (strcmp(arg, "--comp-alg") == 0) { // Verificar el algoritmo de compresion especificado
                if (i + 1 >= argc) { fprintf(stderr, "Falta valor para --comp-alg\n"); return 1; }
                compAlg = argv[++i];
            } else if (strcmp(arg, "--enc-alg") == 0) { // Verificar el algoritmo de encriptacion especificado
                if (i + 1 >= argc) { fprintf(stderr, "Falta valor para --enc-alg\n"); return 1; }
                encAlg = argv[++i];
            } else if (strcmp(arg, "--help") == 0) {
                usage(argv[0]);
                return 0;
            } else {
                fprintf(stderr, "Opción desconocida: %s\n", arg);
                usage(argv[0]);
                return 1;
            }
        } else if (arg[0] == '-' && arg[1] != '\0') {
            // Soporta -c -d -e -u y combinaciones tipo -ce
            if (strcmp(arg, "-i") == 0) { // Verificar ruta de entrada
                if (i + 1 >= argc) { fprintf(stderr, "Falta ruta para -i\n"); return 1; }
                inPath = argv[++i];
            } else if (strcmp(arg, "-o") == 0) { // Verificar ruta de salida
                if (i + 1 >= argc) { fprintf(stderr, "Falta ruta para -o\n"); return 1; }
                outPath = argv[++i];
            } else if (strcmp(arg, "-k") == 0) { // Verificar clave para encriptacion/desencriptacion
                if (i + 1 >= argc) { fprintf(stderr, "Falta clave para -k\n"); return 1; }
                key = argv[++i];
            } else {
                // Parsear banderas cortas combinadas
                for (int j = 1; arg[j] != '\0'; j++) {
                    switch (arg[j]) { // Activar banderas segun lo especificado
                        case 'c': op_c = true; break;
                        case 'd': op_d = true; break;
                        case 'e': op_e = true; break;
                        case 'u': op_u = true; break;
                        default:
                            fprintf(stderr, "Opción corta desconocida: -%c\n", arg[j]);
                            usage(argv[0]);
                            return 1;
                    }
                }
            }
        } else {
            fprintf(stderr, "Argumento no reconocido: %s\n", arg);
            usage(argv[0]);
            return 1;
        }
    }

    // Validaciones básicas
    if (!inPath) { fprintf(stderr, "Falta -i [ruta_entrada]\n"); return 1; }
    if (is_dir(inPath)) {
        fprintf(stderr, "Directorio como entrada aún no implementado: %s\n", inPath);
        return 1;
    }
    if (op_c || op_d) {
        if (strcmp(compAlg, "huffman") != 0 && strcmp(compAlg, "rle") != 0 && strcmp(compAlg, "lzw") != 0) {
            fprintf(stderr, "Algoritmo de compresión no soportado: %s (use: huffman, rle o lzw)\n", compAlg);
            return 1;
        }
    }

    int opsCount = (op_c?1:0) + (op_d?1:0) + (op_e?1:0) + (op_u?1:0);
    if (opsCount == 0) {
        fprintf(stderr, "Debe especificar al menos una operación (-c/-d/-e/-u)\n");
        return 1;
    }

    // Manejo de combinaciones: soportamos -ce (comprimir + encriptar) y -ud (desencriptar + descomprimir)
    if (opsCount > 1) {
        if (op_c && op_e && !op_d && !op_u) {
            // -ce es válido: comprimir primero, luego encriptar
        } else if (op_u && op_d && !op_c && !op_e) {
            // -ud es válido: desencriptar primero, luego descomprimir (inverso de -ce)
        } else {
            fprintf(stderr, "Combinación de operaciones no soportada. Use -ce o -ud\n");
            return 1;
        }
    }

    // Validación de encriptación (luego de manejar combinaciones)
    if (op_e || op_u) {
        if (!key) { fprintf(stderr, "-k [clave] es obligatorio para -e/-u\n"); return 1; }
        if (strcmp(encAlg, "vigenere") != 0 && strcmp(encAlg, "aes") != 0) {
            fprintf(stderr, "Algoritmo de encriptación no soportado: %s (use: vigenere o aes)\n", encAlg);
            return 1;
        }
    }

    // Ejecutar operación individual o combinación
    if (op_c && op_e) {
        // Combinación -ce: comprimir primero, luego encriptar
        if (!key) { fprintf(stderr, "-k [clave] es obligatorio para -ce\n"); return 1; }
        
        const char* compressedFile;
        if (strcmp(compAlg, "huffman") == 0) {
            writeHuffman((char*)inPath);
            compressedFile = "File_Manager/output.bin";
        } else if (strcmp(compAlg, "rle") == 0) {
            writeRLE((char*)inPath);
            compressedFile = "File_Manager/output.rle";
        } else if (strcmp(compAlg, "lzw") == 0) {
            writeLZW((char*)inPath);
            compressedFile = "File_Manager/output.lzw";
        } else {
            fprintf(stderr, "Algoritmo desconocido: %s\n", compAlg);
            return 1;
        }
        
        if (!file_exists(compressedFile)) {
            fprintf(stderr, "Error en compresión: %s\n", compressedFile);
            return 1;
        }
        
        // Ahora encriptar el archivo comprimido
        char encryptedFile[512];
        if (outPath) {
            const char* base = get_basename(outPath);
            snprintf(encryptedFile, sizeof(encryptedFile), "File_Manager/%s", base);
        } else {
            snprintf(encryptedFile, sizeof(encryptedFile), "File_Manager/output.enc");
        }
        
        // Encriptar según el algoritmo seleccionado
        int encResult;
        if (strcmp(encAlg, "vigenere") == 0) {
            encResult = vigenere_encrypt_file(compressedFile, encryptedFile, key);
        } else if (strcmp(encAlg, "aes") == 0) {
            encResult = aes_encrypt_file(compressedFile, encryptedFile, key);
        } else {
            fprintf(stderr, "Algoritmo de encriptación desconocido: %s\n", encAlg);
            return 1;
        }
        
        if (encResult != 0) {
            fprintf(stderr, "Error encriptando archivo comprimido\n");
            return 1;
        }
        
        printf("Archivo comprimido y encriptado guardado en: %s\n", encryptedFile);
        return 0;
    }
    
    if (op_u && op_d) {
        // Combinación -ud: desencriptar primero, luego descomprimir (inverso de -ce)
        if (!key) { fprintf(stderr, "-k [clave] es obligatorio para -ud\n"); return 1; }
        if (!file_exists(inPath)) {
            fprintf(stderr, "Entrada no encontrada: %s\n", inPath);
            return 1;
        }
        
        // Paso 1: Desencriptar
        const char* decryptedFile = "File_Manager/temp_decrypted.tmp";
        
        int decResult;
        if (strcmp(encAlg, "vigenere") == 0) {
            decResult = vigenere_decrypt_file(inPath, decryptedFile, key);
        } else if (strcmp(encAlg, "aes") == 0) {
            decResult = aes_decrypt_file(inPath, decryptedFile, key);
        } else {
            fprintf(stderr, "Algoritmo de encriptación desconocido: %s\n", encAlg);
            return 1;
        }
        
        if (decResult != 0) {
            fprintf(stderr, "Error desencriptando archivo\n");
            return 1;
        }
        
        if (!file_exists(decryptedFile)) {
            fprintf(stderr, "Error en desencriptación: %s\n", decryptedFile);
            return 1;
        }
        
        // Paso 2: Descomprimir el archivo desencriptado
        int result;
        if (strcmp(compAlg, "huffman") == 0) {
            result = readHuffman((char*)decryptedFile);
        } else if (strcmp(compAlg, "rle") == 0) {
            result = readRLE((char*)decryptedFile);
        } else if (strcmp(compAlg, "lzw") == 0) {
            result = readLZW((char*)decryptedFile);
        } else {
            fprintf(stderr, "Algoritmo desconocido: %s\n", compAlg);
            remove(decryptedFile);
            return 1;
        }
        
        // Limpiar archivo temporal
        remove(decryptedFile);
        
        if (result != 0) {
            fprintf(stderr, "Fallo al descomprimir\n");
            return 1;
        }
        
        const char* produced = "File_Manager/output.txt";
        if (!file_exists(produced)) {
            fprintf(stderr, "No se encontró salida de descompresión: %s\n", produced);
            return 1;
        }
        
        if (outPath) {
            char dest[512];
            const char* base = get_basename(outPath);
            snprintf(dest, sizeof(dest), "File_Manager/%s", base);
            if (strcmp(dest, produced) != 0) {
                if (move_file(produced, dest) != 0) return 1;
                printf("Archivo desencriptado y descomprimido guardado en: %s\n", dest);
            } else {
                printf("Archivo desencriptado y descomprimido guardado en: %s\n", produced);
            }
        } else {
            printf("Archivo desencriptado y descomprimido guardado en: %s\n", produced);
        }
        return 0;
    }
    
    if (op_c) {
        // Compression: entrada -> File_Manager/output.[ext]
        const char* produced;
        
        if (strcmp(compAlg, "huffman") == 0) {
            writeHuffman((char*)inPath);
            produced = "File_Manager/output.bin";
        } else if (strcmp(compAlg, "rle") == 0) {
            writeRLE((char*)inPath);
            produced = "File_Manager/output.rle";
        } else if (strcmp(compAlg, "lzw") == 0) {
            writeLZW((char*)inPath);
            produced = "File_Manager/output.lzw";
        } else {
            fprintf(stderr, "Algoritmo desconocido: %s\n", compAlg);
            return 1;
        }
        
        // Siempre dejar en File_Manager: usar nombre dado por -o (si existe) pero dentro de File_Manager
        if (!file_exists(produced)) {
            fprintf(stderr, "No se encontró salida de compresión: %s\n", produced);
            return 1;
        }
        if (outPath) {
            char dest[512];
            const char* base = get_basename(outPath);
            snprintf(dest, sizeof(dest), "File_Manager/%s", base);
            if (strcmp(dest, produced) != 0) {
                if (move_file(produced, dest) != 0) return 1;
                printf("Archivo comprimido guardado en: %s\n", dest);
            } else {
                printf("Archivo comprimido guardado en: %s\n", produced);
            }
        } else {
            printf("Archivo comprimido guardado en: %s\n", produced);
        }
        return 0;
    }

    if (op_d) {
        // Decompression: entrada -> File_Manager/output.txt
        if (!file_exists(inPath)) {
            fprintf(stderr, "Entrada no encontrada: %s\n", inPath);
            return 1;
        }
        
        int result;
        if (strcmp(compAlg, "huffman") == 0) {
            result = readHuffman((char*)inPath);
        } else if (strcmp(compAlg, "rle") == 0) {
            result = readRLE((char*)inPath);
        } else if (strcmp(compAlg, "lzw") == 0) {
            result = readLZW((char*)inPath);
        } else {
            fprintf(stderr, "Algoritmo desconocido: %s\n", compAlg);
            return 1;
        }
        
        if (result != 0) {
            fprintf(stderr, "Fallo al descomprimir %s\n", inPath);
            return 1;
        }
        const char* produced = "File_Manager/output.txt";
        if (!file_exists(produced)) {
            fprintf(stderr, "No se encontró salida de descompresión: %s\n", produced);
            return 1;
        }
        if (outPath) {
            char dest[512];
            const char* base = get_basename(outPath);
            snprintf(dest, sizeof(dest), "File_Manager/%s", base);
            if (strcmp(dest, produced) != 0) {
                if (move_file(produced, dest) != 0) return 1;
                printf("Archivo descomprimido guardado en: %s\n", dest);
            } else {
                printf("Archivo descomprimido guardado en: %s\n", produced);
            }
        } else {
            printf("Archivo descomprimido guardado en: %s\n", produced);
        }
        return 0;
    }

    if (op_e) {
        // Encriptar archivo (funciona con cualquier tipo de archivo: texto o binario)
        if (!file_exists(inPath)) {
            fprintf(stderr, "Entrada no encontrada: %s\n", inPath);
            return 1;
        }
        
        char dest[512];
        if (outPath) {
            const char* base = get_basename(outPath);
            snprintf(dest, sizeof(dest), "File_Manager/%s", base);
        } else {
            snprintf(dest, sizeof(dest), "File_Manager/output.enc");
        }
        
        int encResult;
        if (strcmp(encAlg, "vigenere") == 0) {
            encResult = vigenere_encrypt_file(inPath, dest, key);
        } else if (strcmp(encAlg, "aes") == 0) {
            encResult = aes_encrypt_file(inPath, dest, key);
        } else {
            fprintf(stderr, "Algoritmo de encriptación desconocido: %s\n", encAlg);
            return 1;
        }
        
        if (encResult != 0) {
            fprintf(stderr, "Error encriptando archivo\n");
            return 1;
        }
        
        printf("Archivo encriptado guardado en: %s\n", dest);
        return 0;
    }

    if (op_u) {
        // Desencriptar archivo
        if (!file_exists(inPath)) {
            fprintf(stderr, "Entrada no encontrada: %s\n", inPath);
            return 1;
        }
        
        char dest[512];
        if (outPath) {
            const char* base = get_basename(outPath);
            snprintf(dest, sizeof(dest), "File_Manager/%s", base);
        } else {
            snprintf(dest, sizeof(dest), "File_Manager/output.txt");
        }
        
        int decResult;
        if (strcmp(encAlg, "vigenere") == 0) {
            decResult = vigenere_decrypt_file(inPath, dest, key);
        } else if (strcmp(encAlg, "aes") == 0) {
            decResult = aes_decrypt_file(inPath, dest, key);
        } else {
            fprintf(stderr, "Algoritmo de encriptación desconocido: %s\n", encAlg);
            return 1;
        }
        
        if (decResult != 0) {
            fprintf(stderr, "Error desencriptando archivo\n");
            return 1;
        }
        
        printf("Archivo desencriptado guardado en: %s\n", dest);
        return 0;
    }

    // No debería llegar aquí
    usage(argv[0]);
    return 1;
}
