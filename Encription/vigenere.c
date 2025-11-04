#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vigenere.h"

#define BUFFER_SIZE 8192

static void process_bytes(unsigned char* data, size_t length, const unsigned char* key, size_t keyLen, int encrypt) {
    for (size_t i = 0; i < length; i++) {
        unsigned char k = key[i % keyLen];
        if (encrypt) {
            // XOR encryption
            data[i] = data[i] ^ k;
        } else {
            // XOR decryption (same operation)
            data[i] = data[i] ^ k;
        }
    }
}

static int process_file(const char* inputPath, const char* outputPath, const char* key, int encrypt) {
    FILE* in = fopen(inputPath, "rb");
    if (!in) {
        fprintf(stderr, "Cannot open input file '%s': ", inputPath);
        perror("");
        return 1;
    }

    // Get file size and name for metadata
    fseek(in, 0, SEEK_END);
    long fileSize = ftell(in);
    rewind(in);

    FILE* out = fopen(outputPath, "wb");
    if (!out) {
        fprintf(stderr, "Cannot create output file '%s': ", outputPath);
        perror("");
        fclose(in);
        return 1;
    }

    if (encrypt) {
        // Write metadata for encrypted files
        FileMetadata meta = {
            .magic = METADATA_MAGIC,
            .originalSize = fileSize,
            .flags = 0  // Could add flags for encryption method
        };
        strncpy(meta.originalName, get_basename(inputPath), MAX_FILENAME_LEN-1);
        meta.originalName[MAX_FILENAME_LEN-1] = '\0';
        
        if (fwrite(&meta, sizeof(meta), 1, out) != 1) {
            fprintf(stderr, "Failed to write metadata\n");
            fclose(in);
            fclose(out);
            return 1;
        }
    } else {
        // Read and verify metadata for decryption
        FileMetadata meta;
        if (fread(&meta, sizeof(meta), 1, in) != 1 || meta.magic != METADATA_MAGIC) {
            fprintf(stderr, "Invalid or corrupted encrypted file\n");
            fclose(in);
            fclose(out);
            return 1;
        }
        fileSize -= sizeof(meta);  // Adjust for metadata size
    }

    // Process file in chunks
    unsigned char buffer[BUFFER_SIZE];
    size_t keyLen = strlen(key);
    
    while (!feof(in)) {
        size_t bytesRead = fread(buffer, 1, BUFFER_SIZE, in);
        if (bytesRead == 0) break;
        
        process_bytes(buffer, bytesRead, (const unsigned char*)key, keyLen, encrypt);
        
        if (fwrite(buffer, 1, bytesRead, out) != bytesRead) {
            fprintf(stderr, "Write error\n");
            fclose(in);
            fclose(out);
            return 1;
        }
    }

    fclose(in);
    fclose(out);
    return 0;
}

int vigenere_encrypt_file(const char* inputPath, const char* outputPath, const char* key) {
    return process_file(inputPath, outputPath, key, 1);
}

int vigenere_decrypt_file(const char* inputPath, const char* outputPath, const char* key) {
    return process_file(inputPath, outputPath, key, 0);
}
