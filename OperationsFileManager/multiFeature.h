#ifndef MULTIFEATURE_H
#define MULTIFEATURE_H

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <errno.h>
#include "../common.h"
#include "huffman.h"
#include "rle.h"
#include "vigenere.h"
#include "lzw.h"
#include "aes.h"
#include <dirent.h>

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
} ThreadArgs;

// Funciones auxiliares (implementadas en multiFeature.c)
bool file_exists(const char* path);
void get_file_base(const char* filepath, char* base, size_t size);
int move_file(const char* src, const char* dst);
void* operationOneFile(void* arg);
void initOperation(ThreadArgs p);

#endif
