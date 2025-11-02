#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "lzw.h"

#define LZW_MAX_DICT 4096
#define LZW_MAX_STR 1024

// Diccionario: código -> cadena (valor)
typedef struct {
    int codigo;
    char valor[LZW_MAX_STR];
} EntradaDiccionario;

static int crearDiccionario(EntradaDiccionario *dic) {
    for (int i = 0; i < 256; i++) {
        dic[i].codigo = i;
        dic[i].valor[0] = (char)i;
        dic[i].valor[1] = '\0';
    }
    return 256;
}

static int HallarEnD(EntradaDiccionario *dic, int tam, const char *str) {
    for (int i = 0; i < tam; i++) {
        if (strcmp(dic[i].valor, str) == 0) return dic[i].codigo;
    }
    return -1;
}

void writeLZW(char inputFile[]) {
    FILE* in = fopen(inputFile, "rb");
    if (!in) { fprintf(stderr, "Cannot open input file '%s': ", inputFile); perror(""); return; }

    fseek(in, 0, SEEK_END);
    long fileSize = ftell(in);
    rewind(in);

    if (fileSize < 0) { fprintf(stderr, "Invalid file size for '%s'\n", inputFile); fclose(in); return; }

    unsigned char *buf = (unsigned char*)malloc(fileSize > 0 ? fileSize : 1);
    if (!buf) { perror("malloc"); fclose(in); return; }
    size_t read = fread(buf, 1, fileSize, in);
    fclose(in);

    // Prepare dictionary
    EntradaDiccionario dic[LZW_MAX_DICT];
    int tamDict = crearDiccionario(dic);

    // Output codes (use 16-bit for storage)
    uint16_t *codes = (uint16_t*)malloc(sizeof(uint16_t) * (read + 16));
    if (!codes) { perror("malloc codes"); free(buf); return; }
    size_t outCount = 0;

    char actual[LZW_MAX_STR] = "";
    char temporal[LZW_MAX_STR];

    for (size_t i = 0; i < read; i++) {
        size_t la = strlen(actual);
        // append byte
        actual[la] = (char)buf[i];
        actual[la+1] = '\0';

        if (HallarEnD(dic, tamDict, actual) == -1) {
            // remove last char
            actual[la] = '\0';
            int code = HallarEnD(dic, tamDict, actual);
            if (code == -1) {
                // should not happen
                code = (unsigned char)actual[0];
            }
            codes[outCount++] = (uint16_t)code;

            if (tamDict < LZW_MAX_DICT) {
                strcpy(temporal, actual);
                size_t lt = strlen(temporal);
                temporal[lt] = (char)buf[i];
                temporal[lt+1] = '\0';
                dic[tamDict].codigo = tamDict;
                strncpy(dic[tamDict].valor, temporal, LZW_MAX_STR-1);
                dic[tamDict].valor[LZW_MAX_STR-1] = '\0';
                tamDict++;
            }

            // new actual = last char
            actual[0] = (char)buf[i]; actual[1] = '\0';
        }
    }

    if (strlen(actual) > 0) {
        int code = HallarEnD(dic, tamDict, actual);
        if (code != -1) codes[outCount++] = (uint16_t)code;
    }

    // Write compressed file: [uint64 originalSize][uint32 count][uint16 codes...]
    FILE* out = fopen("File_Manager/output.lzw", "wb");
    if (!out) { perror("Cannot open File_Manager/output.lzw"); free(buf); free(codes); return; }

    uint64_t orig = (uint64_t)read;
    uint32_t count = (uint32_t)outCount;
    fwrite(&orig, sizeof(uint64_t), 1, out);
    fwrite(&count, sizeof(uint32_t), 1, out);
    fwrite(codes, sizeof(uint16_t), outCount, out);

    fclose(out);
    free(buf);
    free(codes);
}

int readLZW(char inputFile[]) {
    FILE* in = fopen(inputFile, "rb");
    if (!in) { fprintf(stderr, "Cannot open file '%s': ", inputFile); perror(""); return 1; }

    uint64_t origSize;
    if (fread(&origSize, sizeof(uint64_t), 1, in) != 1) { perror("fread origSize"); fclose(in); return 1; }
    uint32_t count;
    if (fread(&count, sizeof(uint32_t), 1, in) != 1) { perror("fread count"); fclose(in); return 1; }

    if (count == 0) { fprintf(stderr, "No codes in compressed file\n"); fclose(in); return 1; }

    uint16_t *codes = (uint16_t*)malloc(sizeof(uint16_t) * count);
    if (!codes) { perror("malloc codes"); fclose(in); return 1; }
    if (fread(codes, sizeof(uint16_t), count, in) != count) { perror("fread codes"); free(codes); fclose(in); return 1; }
    fclose(in);

    EntradaDiccionario dic[LZW_MAX_DICT];
    int tamDict = crearDiccionario(dic);

    // Reconstruct
    // Use dynamic buffer for output
    unsigned char *outBuf = (unsigned char*)malloc(origSize > 0 ? origSize : 1);
    if (!outBuf) { perror("malloc outBuf"); free(codes); return 1; }
    uint64_t outPos = 0;

    char previo[LZW_MAX_STR] = "";
    char nueva[LZW_MAX_STR];

    // first code
    if (codes[0] < (uint16_t)tamDict) {
        strncpy(previo, dic[codes[0]].valor, LZW_MAX_STR-1);
        previo[LZW_MAX_STR-1] = '\0';
    } else {
        fprintf(stderr, "Invalid first code\n"); free(codes); free(outBuf); return 1;
    }

    // write previo
    size_t lp = strlen(previo);
    if (outPos + lp > origSize) { fprintf(stderr, "Output overflow\n"); free(codes); free(outBuf); return 1; }
    memcpy(outBuf + outPos, previo, lp);
    outPos += lp;

    for (uint32_t i = 1; i < count; i++) {
        uint16_t code = codes[i];
        if (code < (uint16_t)tamDict) {
            strncpy(nueva, dic[code].valor, LZW_MAX_STR-1);
            nueva[LZW_MAX_STR-1] = '\0';
        } else {
            // special case: W+C
            strncpy(nueva, previo, LZW_MAX_STR-1);
            nueva[LZW_MAX_STR-1] = '\0';
            size_t ln = strlen(nueva);
            if (ln + 1 < LZW_MAX_STR) {
                nueva[ln] = previo[0];
                nueva[ln+1] = '\0';
            } else {
                fprintf(stderr, "String too long in special case\n"); free(codes); free(outBuf); return 1;
            }
        }

        size_t ln = strlen(nueva);
        if (outPos + ln > origSize) { fprintf(stderr, "Output overflow while writing\n"); free(codes); free(outBuf); return 1; }
        memcpy(outBuf + outPos, nueva, ln);
        outPos += ln;

        // add previo + primera letra de nueva to dictionary
        if (tamDict < LZW_MAX_DICT) {
            strncpy(dic[tamDict].valor, previo, LZW_MAX_STR-1);
            dic[tamDict].valor[LZW_MAX_STR-1] = '\0';
            size_t lpv = strlen(dic[tamDict].valor);
            if (lpv + 1 < LZW_MAX_STR) {
                dic[tamDict].valor[lpv] = nueva[0];
                dic[tamDict].valor[lpv+1] = '\0';
            }
            dic[tamDict].codigo = tamDict;
            tamDict++;
        }

        strncpy(previo, nueva, LZW_MAX_STR-1);
        previo[LZW_MAX_STR-1] = '\0';
    }

    // sanity check
    if (outPos != origSize) {
        // It may still be valid for text files: allow <= origSize but warn
        if (outPos > origSize) {
            fprintf(stderr, "Decompressed size mismatch: expected %llu got %llu\n", (unsigned long long)origSize, (unsigned long long)outPos);
            free(codes); free(outBuf); return 1;
        }
    }

    // write output
    FILE* out = fopen("File_Manager/output.txt", "wb");
    if (!out) { perror("Cannot open File_Manager/output.txt"); free(codes); free(outBuf); return 1; }
    size_t written = fwrite(outBuf, 1, outPos, out);
    fclose(out);

    free(codes);
    free(outBuf);
    return (written == outPos) ? 0 : 1;
}
