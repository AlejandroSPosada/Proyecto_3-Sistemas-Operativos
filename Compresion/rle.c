#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "rle.h"
#include "../common.h"

#define MAX_RUN_LENGTH 0xFFFFFFFF  // Maximum run length for uint32_t

/**
 * Run-Length Encoding Compression
 * Encodes consecutive runs of the same byte as [count][byte] pairs
 * Uses uint32_t for counts to handle long runs efficiently
 */
void writeRLE(char inputFile[]) {
    FILE* input = fopen(inputFile, "rb");
    if (!input) {
        fprintf(stderr, "Cannot open input file '%s': ", inputFile);
        perror("");
        return;
    }

    // Get file size
    fseek(input, 0, SEEK_END);
    long fileSize = ftell(input);
    rewind(input);
    
    if (fileSize <= 0) {
        fprintf(stderr, "File '%s' is empty or invalid\n", inputFile);
        fclose(input);
        return;
    }

    // Allocate buffer for entire file
    unsigned char* data = (unsigned char*)malloc(fileSize);
    if (!data) {
        perror("malloc file buffer");
        fclose(input);
        return;
    }
    
    size_t len = fread(data, 1, fileSize, input);
    fclose(input);

    if (len == 0) {
        fprintf(stderr, "Failed to read from '%s'\n", inputFile);
        free(data);
        return;
    }

    // Open output file
    FILE* out = fopen("File_Manager/output.rle", "wb");
    if (!out) {
        perror("Cannot open File_Manager/output.rle");
        free(data);
        return;
    }

    // Write metadata header
    FileMetadata meta = {0};
    meta.magic = METADATA_MAGIC;
    meta.originalSize = (uint64_t)len;
    meta.flags = 0;
    strncpy(meta.originalName, get_basename(inputFile), MAX_FILENAME_LEN-1);
    meta.originalName[MAX_FILENAME_LEN-1] = '\0';
    fwrite(&meta, sizeof(meta), 1, out);

    // RLE encoding
    size_t i = 0;
    while (i < len) {
        unsigned char currentByte = data[i];
        uint32_t count = 1;
        
        // Count consecutive occurrences of the same byte
        while (i + count < len && data[i + count] == currentByte && count < MAX_RUN_LENGTH) {
            count++;
        }
        
        // Write [count][byte] pair
        fwrite(&count, sizeof(uint32_t), 1, out);
        fwrite(&currentByte, sizeof(unsigned char), 1, out);
        
        i += count;
    }

    fclose(out);
    free(data);
}

/**
 * Run-Length Encoding Decompression
 * Reads [count][byte] pairs and expands them to original data
 */
int readRLE(char inputFile[]) {
    FILE* in = fopen(inputFile, "rb");
    if (!in) {
        fprintf(stderr, "Cannot open file '%s': ", inputFile);
        perror("");
        return 1;
    }

    // Read metadata header
    FileMetadata meta;
    if (fread(&meta, sizeof(meta), 1, in) != 1 || meta.magic != METADATA_MAGIC) {
        fprintf(stderr, "Invalid or missing metadata in compressed file\n");
        fclose(in);
        return 1;
    }

    uint64_t originalSize = meta.originalSize;
    if (originalSize == 0 || originalSize > 1024ULL * 1024 * 1024 * 10) { // Max 10GB sanity check
        fprintf(stderr, "Invalid or too large original size: %llu\n", 
                (unsigned long long)originalSize);
        fclose(in);
        return 1;
    }

    // Allocate output buffer
    unsigned char* output = (unsigned char*)malloc(originalSize);
    if (!output) {
        perror("malloc output buffer");
        fclose(in);
        return 1;
    }

    // Decompress RLE data
    size_t outputPos = 0;
    while (!feof(in) && outputPos < originalSize) {
        uint32_t count;
        unsigned char byte;
        
        size_t readCount = fread(&count, sizeof(uint32_t), 1, in);
        if (readCount != 1) {
            if (feof(in)) break;  // End of file
            perror("fread count");
            free(output);
            fclose(in);
            return 1;
        }
        
        if (fread(&byte, sizeof(unsigned char), 1, in) != 1) {
            perror("fread byte");
            free(output);
            fclose(in);
            return 1;
        }
        
        // Validate count doesn't exceed remaining space
        if (outputPos + count > originalSize) {
            fprintf(stderr, "RLE data corruption: run exceeds original size\n");
            free(output);
            fclose(in);
            return 1;
        }
        
        // Expand the run
        for (uint32_t i = 0; i < count; i++) {
            output[outputPos++] = byte;
        }
    }
    
    fclose(in);

    // Verify we got the expected amount of data
    if (outputPos != originalSize) {
        fprintf(stderr, "Size mismatch: expected %llu, got %zu bytes\n", 
                (unsigned long long)originalSize, outputPos);
        free(output);
        return 1;
    }

    // Write decompressed data to output file
    FILE* outFile = fopen("File_Manager/output.txt", "wb");
    if (!outFile) {
        perror("Cannot open File_Manager/output.txt");
        free(output);
        return 1;
    }
    
    size_t written = fwrite(output, 1, originalSize, outFile);
    fclose(outFile);
    
    if (written != originalSize) {
        fprintf(stderr, "Failed to write complete output\n");
        free(output);
        return 1;
    }

    free(output);
    return 0;
}
