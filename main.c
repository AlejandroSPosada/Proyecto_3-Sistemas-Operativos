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
#include "OperationsFileManager/multiFeature.h"

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

static bool is_dir(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}

/*
ejemplo: ./app -c --comp-alg huffman -i File_Manager/input.txt -o salida.bin

Before of using OperationsFileManager, that helps us to interact with File_Manager,
let's find out if the input parameters from the user are valid.
*/
int main(int argc, char** argv) {
    bool op_c = false, op_d = false, op_e = false, op_u = false;
    char* compAlg = "huffman";
    char* encAlg  = "vigenere";
    char* inPath = NULL;
    char* outPath = NULL;
    char* key = NULL;

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
    ThreadArgs myargs = {op_c, op_d, op_e, op_u, compAlg, encAlg, inPath, outPath, key};
    initOperation(myargs);

    return 0;
    // No debería llegar aquí
    usage(argv[0]);
    return 1;
}