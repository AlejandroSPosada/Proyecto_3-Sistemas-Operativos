#ifndef MULTIFEATURE_H
#define MULTIFEATURE_H

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include "../common.h"
#include "../Compresion/huffman.h"
#include "../Compresion/rle.h"
#include "../Compresion/lzw.h"
#include "../Encription/vigenere.h"
#include "../Encription/aes.h"
#include <dirent.h>
#include <time.h>

// this header is in charge of the interaction of the user with the File_Manager, and also the multithread feature.

typedef struct ThreadArgs{
    bool op_c;
    bool op_d;
    bool op_e;
    bool op_u;
    char* compAlg;
    char* encAlg;
    char* inPath;
    char* outPath;
    char* key;
    int thread_index;           // Número del hilo para impresión
    char* thread_file_name;     // Nombre del archivo siendo procesado
    struct timespec start_time; // Tiempo de inicio
    double elapsed_time;        // Tiempo transcurrido en segundos
} ThreadArgs;

// Funciones auxiliares (implementadas en multiFeature.c)
bool file_exists(const char* path);
void get_file_base(const char* filepath, char* base, size_t size);
int move_file(const char* src, const char* dst);
void ensure_directory_exists(const char* dir_path);
void* operationOneFile(void* arg);
void initOperation(ThreadArgs p);

#endif
