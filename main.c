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

static void usage(const char* prog) {
    fprintf(stderr,
        "Uso: %s [ops] [opciones]\n\n"
        "Operaciones (combinables):\n"
        "  -c    Comprimir\n"
        "  -d    Descomprimir\n"
        "  -e    Encriptar (Vigenère)\n"
        "  -u    Desencriptar (Vigenère)\n"
        "  -ce   Comprimir y luego encriptar (limitado, ver nota)\n\n"
        "Opciones:\n"
    "  --comp-alg [nombre]   Algoritmo de compresión (huffman, rle, lzw)\n"
        "  --enc-alg  [nombre]   Algoritmo de encriptación (vigenere)\n"
        "  -i [ruta]             Archivo de entrada\n"
        "  -o [ruta]             Archivo de salida\n"
        "  -k [clave]            Clave para encriptar/desencriptar\n\n"
        "Notas:\n"
        "  - La encriptación Vigenère actual trabaja sobre texto (no binario).\n"
        "  - La combinación -ce sobre .bin no está soportada por esta implementación.\n",
        prog);
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

static char* read_text_file(const char* path, size_t* outLen) {
    FILE* f = fopen(path, "r");
    if (!f) { perror("fopen"); return NULL; }
    if (fseek(f, 0, SEEK_END) != 0) { perror("fseek"); fclose(f); return NULL; }
    long sz = ftell(f);
    if (sz < 0) { perror("ftell"); fclose(f); return NULL; }
    rewind(f);
    char* buf = (char*)malloc((size_t)sz + 1);
    if (!buf) { perror("malloc"); fclose(f); return NULL; }
    size_t rd = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[rd] = '\0';
    if (outLen) *outLen = rd;
    return buf;
}

static int write_bytes_file(const char* path, const char* data, size_t len) {
    FILE* f = fopen(path, "wb");
    if (!f) { perror("fopen"); return -1; }
    size_t wr = fwrite(data, 1, len, f);
    fclose(f);
    return (wr == len) ? 0 : -1;
}

// Extrae el nombre base de una ruta (sin directorios)
static const char* get_basename(const char* path) {
    const char* lastSlash = strrchr(path, '/');
    return lastSlash ? lastSlash + 1 : path;
}

int main(int argc, char** argv) {
    bool op_c = false, op_d = false, op_e = false, op_u = false;
    const char* compAlg = "huffman";
    const char* encAlg  = "vigenere";
    const char* inPath = NULL;
    const char* outPath = NULL;
    const char* key = NULL;

    if (argc <= 1) {
        usage(argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];
        if (arg[0] == '-' && arg[1] == '-' ) {
            if (strcmp(arg, "--comp-alg") == 0) {
                if (i + 1 >= argc) { fprintf(stderr, "Falta valor para --comp-alg\n"); return 1; }
                compAlg = argv[++i];
            } else if (strcmp(arg, "--enc-alg") == 0) {
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
            if (strcmp(arg, "-i") == 0) {
                if (i + 1 >= argc) { fprintf(stderr, "Falta ruta para -i\n"); return 1; }
                inPath = argv[++i];
            } else if (strcmp(arg, "-o") == 0) {
                if (i + 1 >= argc) { fprintf(stderr, "Falta ruta para -o\n"); return 1; }
                outPath = argv[++i];
            } else if (strcmp(arg, "-k") == 0) {
                if (i + 1 >= argc) { fprintf(stderr, "Falta clave para -k\n"); return 1; }
                key = argv[++i];
            } else {
                // Parsear banderas cortas combinadas
                for (int j = 1; arg[j] != '\0'; j++) {
                    switch (arg[j]) {
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

    // Manejo de combinaciones: soportamos -ce de forma declarativa, pero advertimos limitación
    if (opsCount > 1) {
        if (op_c && op_e && !op_d && !op_u) {
            fprintf(stderr, "Aviso: combinación -ce no implementada por limitación de Vigenère sobre binario.\n");
            return 1;
        } else {
            fprintf(stderr, "Combinación de operaciones no soportada en esta versión.\n");
            return 1;
        }
    }

    // Validación de encriptación (luego de manejar combinaciones)
    if (op_e || op_u) {
        if (!key) { fprintf(stderr, "-k [clave] es obligatorio para -e/-u\n"); return 1; }
        if (strcmp(encAlg, "vigenere") != 0) {
            fprintf(stderr, "Algoritmo de encriptación no soportado: %s\n", encAlg);
            return 1;
        }
    }

    // Ejecutar operación individual
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
        // Vigenère: solo texto (usa strlen internamente)
        size_t len = 0;
        char* text = read_text_file(inPath, &len);
        if (!text) return 1;
        char* outBuf = (char*)malloc(len + 1);
        if (!outBuf) { perror("malloc"); free(text); return 1; }
        vigenere_encrypt(text, (char*)key, outBuf);
        char dest[512];
        if (outPath) {
            const char* base = get_basename(outPath);
            snprintf(dest, sizeof(dest), "File_Manager/%s", base);
        } else {
            snprintf(dest, sizeof(dest), "File_Manager/output.txt");
        }
        if (write_bytes_file(dest, outBuf, strlen(outBuf)) != 0) {
            fprintf(stderr, "Error escribiendo salida: %s\n", dest);
            free(text); free(outBuf); return 1;
        }
        printf("Archivo encriptado guardado en: %s\n", dest);
        free(text);
        free(outBuf);
        return 0;
    }

    if (op_u) {
        size_t len = 0;
        char* cipher = read_text_file(inPath, &len);
        if (!cipher) return 1;
        char* outBuf = (char*)malloc(len + 1);
        if (!outBuf) { perror("malloc"); free(cipher); return 1; }
        vigenere_decrypt(cipher, (char*)key, outBuf);
        char dest[512];
        if (outPath) {
            const char* base = get_basename(outPath);
            snprintf(dest, sizeof(dest), "File_Manager/%s", base);
        } else {
            snprintf(dest, sizeof(dest), "File_Manager/output.txt");
        }
        if (write_bytes_file(dest, outBuf, strlen(outBuf)) != 0) {
            fprintf(stderr, "Error escribiendo salida: %s\n", dest);
            free(cipher); free(outBuf); return 1;
        }
        printf("Archivo desencriptado guardado en: %s\n", dest);
        free(cipher);
        free(outBuf);
        return 0;
    }

    // No debería llegar aquí
    usage(argv[0]);
    return 1;
}
