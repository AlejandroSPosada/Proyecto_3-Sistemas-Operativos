#include "multiFeature.h"

void* operationOneFile(void* arg) {
    struct ThreadArgs* args = (struct ThreadArgs*)arg;
    bool op_c = args->op_c;
    bool op_d = args->op_d;
    bool op_e = args->op_e; 
    bool op_u = args->op_u;
    const char* compAlg = args->compAlg;
    const char* encAlg  = args->encAlg;
    const char* inPath = args->inPath;
    const char* outPath = args->outPath;
    const char* key = args->key;

    // Ejecutar operación individual o combinación
    if (op_c && op_e) {
        // Combinación -ce: comprimir primero, luego encriptar
        if (!key) { fprintf(stderr, "-k [clave] es obligatorio para -ce\n"); return NULL; }
        
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
            return NULL;
        }
        
        if (!file_exists(compressedFile)) {
            fprintf(stderr, "Error en compresión: %s\n", compressedFile);
            return NULL;
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
            return NULL;
        }
        
        if (encResult != 0) {
            fprintf(stderr, "Error encriptando archivo comprimido\n");
            return NULL;
        }
        
        printf("Archivo comprimido y encriptado guardado en: %s\n", encryptedFile);
        return NULL;
    }
    
    if (op_u && op_d) {
        // Combinación -ud: desencriptar primero, luego descomprimir (inverso de -ce)
        if (!key) { fprintf(stderr, "-k [clave] es obligatorio para -ud\n"); return NULL; }
        if (!file_exists(inPath)) {
            fprintf(stderr, "Entrada no encontrada: %s\n", inPath);
            return NULL;
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
            return NULL;
        }
        
        if (decResult != 0) {
            fprintf(stderr, "Error desencriptando archivo\n");
            return NULL;
        }
        
        if (!file_exists(decryptedFile)) {
            fprintf(stderr, "Error en desencriptación: %s\n", decryptedFile);
            return NULL;
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
            return NULL;
        }
        
        // Limpiar archivo temporal
        remove(decryptedFile);
        
        if (result != 0) {
            fprintf(stderr, "Fallo al descomprimir\n");
            return NULL;
        }
        
        const char* produced = "File_Manager/output.txt";
        if (!file_exists(produced)) {
            fprintf(stderr, "No se encontró salida de descompresión: %s\n", produced);
            return NULL;
        }
        
        if (outPath) {
            char dest[512];
            const char* base = get_basename(outPath);
            snprintf(dest, sizeof(dest), "File_Manager/%s", base);
            if (strcmp(dest, produced) != 0) {
                if (move_file(produced, dest) != 0) return NULL;
                printf("Archivo desencriptado y descomprimido guardado en: %s\n", dest);
            } else {
                printf("Archivo desencriptado y descomprimido guardado en: %s\n", produced);
            }
        } else {
            printf("Archivo desencriptado y descomprimido guardado en: %s\n", produced);
        }
        return NULL;
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
            return NULL;
        }
        
        // Siempre dejar en File_Manager: usar nombre dado por -o (si existe) pero dentro de File_Manager
        if (!file_exists(produced)) {
            fprintf(stderr, "No se encontró salida de compresión: %s\n", produced);
            return NULL;
        }
        if (outPath) {
            char dest[512];
            const char* base = get_basename(outPath);
            snprintf(dest, sizeof(dest), "File_Manager/%s", base);
            if (strcmp(dest, produced) != 0) {
                if (move_file(produced, dest) != 0) return NULL;
                printf("Archivo comprimido guardado en: %s\n", dest);
            } else {
                printf("Archivo comprimido guardado en: %s\n", produced);
            }
        } else {
            printf("Archivo comprimido guardado en: %s\n", produced);
        }
        return NULL;
    }

    if (op_d) {
        // Decompression: entrada -> File_Manager/output.txt
        if (!file_exists(inPath)) {
            fprintf(stderr, "Entrada no encontrada: %s\n", inPath);
            return NULL;
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
            return NULL;
        }
        
        if (result != 0) {
            fprintf(stderr, "Fallo al descomprimir %s\n", inPath);
            return NULL;
        }
        const char* produced = "File_Manager/output.txt";
        if (!file_exists(produced)) {
            fprintf(stderr, "No se encontró salida de descompresión: %s\n", produced);
            return NULL;
        }
        if (outPath) {
            char dest[512];
            const char* base = get_basename(outPath);
            snprintf(dest, sizeof(dest), "File_Manager/%s", base);
            if (strcmp(dest, produced) != 0) {
                if (move_file(produced, dest) != 0) return NULL;
                printf("Archivo descomprimido guardado en: %s\n", dest);
            } else {
                printf("Archivo descomprimido guardado en: %s\n", produced);
            }
        } else {
            printf("Archivo descomprimido guardado en: %s\n", produced);
        }
        return NULL;
    }

    if (op_e) {
        // Encriptar archivo (funciona con cualquier tipo de archivo: texto o binario)
        if (!file_exists(inPath)) {
            fprintf(stderr, "Entrada no encontrada: %s\n", inPath);
            return NULL;
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
            return NULL;
        }
        
        if (encResult != 0) {
            fprintf(stderr, "Error encriptando archivo\n");
            return NULL;
        }
        
        printf("Archivo encriptado guardado en: %s\n", dest);
        return NULL;
    }

    if (op_u) {
        // Desencriptar archivo
        if (!file_exists(inPath)) {
            fprintf(stderr, "Entrada no encontrada: %s\n", inPath);
            return NULL;
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
            return NULL;
        }
        
        if (decResult != 0) {
            fprintf(stderr, "Error desencriptando archivo\n");
            return NULL;
        }
        
        printf("Archivo desencriptado guardado en: %s\n", dest);
        return NULL;
    }

    return NULL;
}

static bool is_dir(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}

static bool file_exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

static int move_file(const char* src, const char* dst) {
    if (rename(src, dst) == 0) return 0;
    perror("rename");
    return -1;
}

void initOperation(ThreadArgs myargs){
    pthread_t thread1;
    pthread_create(&thread1, NULL, operationOneFile, &myargs);

    // Wait for thread to finish
    pthread_join(thread1, NULL);
}