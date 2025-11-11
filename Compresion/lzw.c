#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "lzw.h"
#include "../common.h"
#include "../posix_utils.h"

#define LZW_MAX_DICT 4096

typedef struct {
    int code;
    unsigned char* data;
    size_t len;
} LZWEntry;

static int crearDiccionario(LZWEntry *dic) {
    for (int i = 0; i < 256; i++) {
        dic[i].code = i;
        dic[i].data = (unsigned char*)malloc(1);
        if (!dic[i].data) {
                        for (int j = 0; j < i; j++) free(dic[j].data);
            return -1;
        }
        dic[i].data[0] = (unsigned char)i;
        dic[i].len = 1;
    }
    return 256;
}

static void freeDictionary(LZWEntry *dic, int size) {
    for (int i = 0; i < size; i++) {
        if (dic[i].data) free(dic[i].data);
    }
}

static int findInDict(LZWEntry *dic, int dictSize, const unsigned char* data, size_t len) {
    for (int i = 0; i < dictSize; i++) {
        if (dic[i].len == len && memcmp(dic[i].data, data, len) == 0) return dic[i].code;
    }
    return -1;
}

void writeLZW(char inputFile[]) {
    // Abrir archivo con POSIX
    int fd_input = posix_open_read(inputFile);
    if (fd_input == -1) return;

    // Obtener tamaño con fstat
    off_t fileSize = posix_get_file_size(fd_input);
    if (fileSize < 0) {
        fprintf(stderr, "Invalid file size for '%s'\n", inputFile);
        posix_close(fd_input);
        return;
    }

    unsigned char *buf = (unsigned char*)malloc(fileSize > 0 ? fileSize : 1);
    if (!buf) {
        fprintf(stderr, "Memory allocation failed: %s\n", strerror(errno));
        posix_close(fd_input);
        return;
    }
    
    ssize_t bytes_read = posix_read_full(fd_input, buf, fileSize);
    posix_close(fd_input);
    
    if (bytes_read != fileSize) {
        fprintf(stderr, "Failed to read complete file\n");
        free(buf);
        return;
    }

    LZWEntry dict[LZW_MAX_DICT];
    int dictSize = crearDiccionario(dict);
    if (dictSize < 0) { free(buf); fprintf(stderr, "Failed to init dictionary\n"); return; }

    uint16_t *codes = (uint16_t*)malloc(sizeof(uint16_t) * (bytes_read + 16));
    if (!codes) { perror("malloc codes"); free(buf); freeDictionary(dict, dictSize); return; }
    size_t outCount = 0;

    unsigned char *w = NULL;
    size_t wlen = 0;

    for (size_t i = 0; i < (size_t)bytes_read; i++) {
        unsigned char k = buf[i];

        unsigned char *wk = (unsigned char*)malloc(wlen + 1);
        if (!wk) { perror("malloc wk"); free(buf); free(codes); freeDictionary(dict, dictSize); return; }
        if (wlen > 0) memcpy(wk, w, wlen);
        wk[wlen] = k;

        int idx = findInDict(dict, dictSize, wk, wlen + 1);
        if (idx != -1) {
            if (w) free(w);
            w = wk;
            wlen = wlen + 1;
        } else {
            if (wlen == 0) {
                codes[outCount++] = (uint16_t)k;
            } else {
                int codeW = findInDict(dict, dictSize, w, wlen);
                if (codeW == -1) {
                    codeW = (unsigned char)w[0];
                }
                codes[outCount++] = (uint16_t)codeW;
            }

            if (dictSize < LZW_MAX_DICT) {
                dict[dictSize].data = wk;
                dict[dictSize].len = wlen + 1;
                dict[dictSize].code = dictSize;
                dictSize++;
            } else {
                free(wk);
            }
            if (w) free(w);
            w = (unsigned char*)malloc(1);
            if (!w) { perror("malloc w"); free(buf); free(codes); freeDictionary(dict, dictSize); return; }
            w[0] = k; wlen = 1;
        }
    }

    if (wlen > 0) {
        int codeW = findInDict(dict, dictSize, w, wlen);
        if (codeW != -1) codes[outCount++] = (uint16_t)codeW;
    }

    // Abrir archivo de salida con POSIX
    int fd_output = posix_open_write("File_Manager/output.lzw");
    if (fd_output == -1) {
        free(buf); free(codes); freeDictionary(dict, dictSize);
        if (w) free(w);
        return;
    }

    FileMetadata meta = {0};
    meta.magic = METADATA_MAGIC;
    meta.originalSize = (uint64_t)bytes_read;
    meta.flags = 0;
    strncpy(meta.originalName, get_basename(inputFile), MAX_FILENAME_LEN-1);
    meta.originalName[MAX_FILENAME_LEN-1] = '\0';
    
    if (posix_write_full(fd_output, &meta, sizeof(meta)) != sizeof(meta)) {
        fprintf(stderr, "Failed to write metadata\n");
        posix_close(fd_output);
        free(buf); free(codes); freeDictionary(dict, dictSize);
        if (w) free(w);
        return;
    }

    uint32_t count = (uint32_t)outCount;
    if (posix_write_full(fd_output, &count, sizeof(uint32_t)) != sizeof(uint32_t)) {
        fprintf(stderr, "Failed to write count\n");
        posix_close(fd_output);
        free(buf); free(codes); freeDictionary(dict, dictSize);
        if (w) free(w);
        return;
    }
    
    size_t codes_bytes = sizeof(uint16_t) * outCount;
    if (posix_write_full(fd_output, codes, codes_bytes) != (ssize_t)codes_bytes) {
        fprintf(stderr, "Failed to write codes\n");
        posix_close(fd_output);
        free(buf); free(codes); freeDictionary(dict, dictSize);
        if (w) free(w);
        return;
    }

    posix_close(fd_output);

    free(buf);
    free(codes);
    if (w) free(w);
    freeDictionary(dict, dictSize);
}

int readLZW(char inputFile[]) {
    // Abrir archivo con POSIX
    int fd_input = posix_open_read(inputFile);
    if (fd_input == -1) return 1;

    // Leer metadata header
    FileMetadata meta;
    if (posix_read_full(fd_input, &meta, sizeof(meta)) != sizeof(meta) ||
        meta.magic != METADATA_MAGIC) {
        fprintf(stderr, "Invalid or missing metadata in compressed file\n");
        posix_close(fd_input);
        return 1;
    }
    uint64_t origSize = meta.originalSize;

    uint32_t count;
    if (posix_read_full(fd_input, &count, sizeof(uint32_t)) != sizeof(uint32_t)) {
        fprintf(stderr, "Failed to read count\n");
        posix_close(fd_input);
        return 1;
    }

    if (count == 0) {
        fprintf(stderr, "No codes in compressed file\n");
        posix_close(fd_input);
        return 1;
    }

    uint16_t *codes = (uint16_t*)malloc(sizeof(uint16_t) * count);
    if (!codes) {
        fprintf(stderr, "Memory allocation failed: %s\n", strerror(errno));
        posix_close(fd_input);
        return 1;
    }
    
    size_t codes_bytes = sizeof(uint16_t) * count;
    if (posix_read_full(fd_input, codes, codes_bytes) != (ssize_t)codes_bytes) {
        fprintf(stderr, "Failed to read codes\n");
        free(codes);
        posix_close(fd_input);
        return 1;
    }
    posix_close(fd_input);

    LZWEntry dict[LZW_MAX_DICT];
    int dictSize = crearDiccionario(dict);
    if (dictSize < 0) { free(codes); fprintf(stderr, "Failed to init dictionary\n"); return 1; }

    unsigned char *outBuf = (unsigned char*)malloc(origSize > 0 ? origSize : 1);
    if (!outBuf) { perror("malloc outBuf"); free(codes); freeDictionary(dict, dictSize); return 1; }
    uint64_t outPos = 0;

    // Process first code
    if (codes[0] >= (uint16_t)dictSize) { fprintf(stderr, "Invalid first code\n"); free(codes); free(outBuf); freeDictionary(dict, dictSize); return 1; }
    unsigned char *entry = dict[codes[0]].data;
    size_t entryLen = dict[codes[0]].len;
    if (outPos + entryLen > origSize) { fprintf(stderr, "Output overflow\n"); free(codes); free(outBuf); freeDictionary(dict, dictSize); return 1; }
    memcpy(outBuf + outPos, entry, entryLen);
    outPos += entryLen;

    // previous entry copy
    unsigned char *prev = (unsigned char*)malloc(entryLen);
    if (!prev) { perror("malloc prev"); free(codes); free(outBuf); freeDictionary(dict, dictSize); return 1; }
    memcpy(prev, entry, entryLen);
    size_t prevLen = entryLen;

    for (uint32_t i = 1; i < count; i++) {
        uint16_t code = codes[i];
        unsigned char *current = NULL;
        size_t curLen = 0;

        if (code < (uint16_t)dictSize) {
            current = (unsigned char*)malloc(dict[code].len);
            if (!current) { perror("malloc current"); free(prev); free(codes); free(outBuf); freeDictionary(dict, dictSize); return 1; }
            memcpy(current, dict[code].data, dict[code].len);
            curLen = dict[code].len;
        } else {
            // special case: code equals dictSize -> prev + first byte of prev
            curLen = prevLen + 1;
            current = (unsigned char*)malloc(curLen);
            if (!current) { perror("malloc special"); free(prev); free(codes); free(outBuf); freeDictionary(dict, dictSize); return 1; }
            memcpy(current, prev, prevLen);
            current[curLen-1] = prev[0];
        }

        if (outPos + curLen > origSize) { fprintf(stderr, "Output overflow while writing\n"); free(current); free(prev); free(codes); free(outBuf); freeDictionary(dict, dictSize); return 1; }
        memcpy(outBuf + outPos, current, curLen);
        outPos += curLen;

        // add prev + first byte of current to dictionary
        if (dictSize < LZW_MAX_DICT) {
            size_t newLen = prevLen + 1;
            dict[dictSize].data = (unsigned char*)malloc(newLen);
            if (!dict[dictSize].data) { perror("malloc dict entry"); free(current); free(prev); free(codes); free(outBuf); freeDictionary(dict, dictSize); return 1; }
            memcpy(dict[dictSize].data, prev, prevLen);
            dict[dictSize].data[prevLen] = current[0];
            dict[dictSize].len = newLen;
            dict[dictSize].code = dictSize;
            dictSize++;
        }

        free(prev);
        prev = current;
        prevLen = curLen;
    }

    if (outPos != origSize) {
        if (outPos > origSize) {
            fprintf(stderr, "Decompressed size mismatch: expected %llu got %llu\n", (unsigned long long)origSize, (unsigned long long)outPos);
            free(prev); free(codes); free(outBuf); freeDictionary(dict, dictSize); return 1;
        }
    }

    // Escribir salida con POSIX
    int fd_output = posix_open_write("File_Manager/output.txt");
    if (fd_output == -1) {
        free(prev); free(codes); free(outBuf); freeDictionary(dict, dictSize);
        return 1;
    }
    
    ssize_t written = posix_write_full(fd_output, outBuf, outPos);
    posix_close(fd_output);

    free(prev);
    free(codes);
    free(outBuf);
    freeDictionary(dict, dictSize);
    return (written == (ssize_t)outPos) ? 0 : 1;
}