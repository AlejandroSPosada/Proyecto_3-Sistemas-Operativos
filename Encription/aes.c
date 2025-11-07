#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aes.h"

// AES-256 constants
#define Nb 4  // Number of columns (32-bit words) in the state (always 4 for AES)
#define Nk 8  // Number of 32-bit words in the key (8 for AES-256)
#define Nr 14 // Number of rounds (14 for AES-256)

// S-box: Substitution box for SubBytes transformation
static const uint8_t sbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

// Inverse S-box for InvSubBytes
static const uint8_t inv_sbox[256] = {
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
};

// Round constant for key expansion
static const uint8_t Rcon[11] = {
    0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
};

// Galois Field multiplication by 2
static uint8_t gmul2(uint8_t a) {
    return (a << 1) ^ (((a >> 7) & 1) * 0x1b);
}

// Galois Field multiplication by 3
static uint8_t gmul3(uint8_t a) {
    return gmul2(a) ^ a;
}

// Galois Field multiplication by 9
static uint8_t gmul9(uint8_t a) {
    return gmul2(gmul2(gmul2(a))) ^ a;
}

// Galois Field multiplication by 11
static uint8_t gmul11(uint8_t a) {
    return gmul2(gmul2(gmul2(a)) ^ a) ^ a;
}

// Galois Field multiplication by 13
static uint8_t gmul13(uint8_t a) {
    return gmul2(gmul2(gmul2(a) ^ a)) ^ a;
}

// Galois Field multiplication by 14
static uint8_t gmul14(uint8_t a) {
    return gmul2(gmul2(gmul2(a) ^ a) ^ a);
}

// SubBytes transformation
static void SubBytes(uint8_t state[4][4]) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            state[i][j] = sbox[state[i][j]];
        }
    }
}

// InvSubBytes transformation
static void InvSubBytes(uint8_t state[4][4]) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            state[i][j] = inv_sbox[state[i][j]];
        }
    }
}

// ShiftRows transformation
static void ShiftRows(uint8_t state[4][4]) {
    uint8_t temp;
    
    // Row 1: shift left by 1
    temp = state[1][0];
    state[1][0] = state[1][1];
    state[1][1] = state[1][2];
    state[1][2] = state[1][3];
    state[1][3] = temp;
    
    // Row 2: shift left by 2
    temp = state[2][0];
    state[2][0] = state[2][2];
    state[2][2] = temp;
    temp = state[2][1];
    state[2][1] = state[2][3];
    state[2][3] = temp;
    
    // Row 3: shift left by 3 (or right by 1)
    temp = state[3][3];
    state[3][3] = state[3][2];
    state[3][2] = state[3][1];
    state[3][1] = state[3][0];
    state[3][0] = temp;
}

// InvShiftRows transformation
static void InvShiftRows(uint8_t state[4][4]) {
    uint8_t temp;
    
    // Row 1: shift right by 1
    temp = state[1][3];
    state[1][3] = state[1][2];
    state[1][2] = state[1][1];
    state[1][1] = state[1][0];
    state[1][0] = temp;
    
    // Row 2: shift right by 2
    temp = state[2][0];
    state[2][0] = state[2][2];
    state[2][2] = temp;
    temp = state[2][1];
    state[2][1] = state[2][3];
    state[2][3] = temp;
    
    // Row 3: shift right by 3 (or left by 1)
    temp = state[3][0];
    state[3][0] = state[3][1];
    state[3][1] = state[3][2];
    state[3][2] = state[3][3];
    state[3][3] = temp;
}

// MixColumns transformation
static void MixColumns(uint8_t state[4][4]) {
    uint8_t temp[4];
    
    for (int i = 0; i < 4; i++) {
        temp[0] = state[0][i];
        temp[1] = state[1][i];
        temp[2] = state[2][i];
        temp[3] = state[3][i];
        
        state[0][i] = gmul2(temp[0]) ^ gmul3(temp[1]) ^ temp[2] ^ temp[3];
        state[1][i] = temp[0] ^ gmul2(temp[1]) ^ gmul3(temp[2]) ^ temp[3];
        state[2][i] = temp[0] ^ temp[1] ^ gmul2(temp[2]) ^ gmul3(temp[3]);
        state[3][i] = gmul3(temp[0]) ^ temp[1] ^ temp[2] ^ gmul2(temp[3]);
    }
}

// InvMixColumns transformation
static void InvMixColumns(uint8_t state[4][4]) {
    uint8_t temp[4];
    
    for (int i = 0; i < 4; i++) {
        temp[0] = state[0][i];
        temp[1] = state[1][i];
        temp[2] = state[2][i];
        temp[3] = state[3][i];
        
        state[0][i] = gmul14(temp[0]) ^ gmul11(temp[1]) ^ gmul13(temp[2]) ^ gmul9(temp[3]);
        state[1][i] = gmul9(temp[0]) ^ gmul14(temp[1]) ^ gmul11(temp[2]) ^ gmul13(temp[3]);
        state[2][i] = gmul13(temp[0]) ^ gmul9(temp[1]) ^ gmul14(temp[2]) ^ gmul11(temp[3]);
        state[3][i] = gmul11(temp[0]) ^ gmul13(temp[1]) ^ gmul9(temp[2]) ^ gmul14(temp[3]);
    }
}

// AddRoundKey transformation
static void AddRoundKey(uint8_t state[4][4], const uint8_t* roundKey) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            state[j][i] ^= roundKey[i * 4 + j];
        }
    }
}

// Key expansion for AES-256
static void KeyExpansion(const uint8_t* key, uint8_t* expandedKey) {
    uint8_t temp[4];
    int i = 0;
    
    // First Nk words are the key itself
    while (i < Nk) {
        expandedKey[i * 4] = key[i * 4];
        expandedKey[i * 4 + 1] = key[i * 4 + 1];
        expandedKey[i * 4 + 2] = key[i * 4 + 2];
        expandedKey[i * 4 + 3] = key[i * 4 + 3];
        i++;
    }
    
    // Generate remaining words
    i = Nk;
    while (i < Nb * (Nr + 1)) {
        temp[0] = expandedKey[(i - 1) * 4];
        temp[1] = expandedKey[(i - 1) * 4 + 1];
        temp[2] = expandedKey[(i - 1) * 4 + 2];
        temp[3] = expandedKey[(i - 1) * 4 + 3];
        
        if (i % Nk == 0) {
            // RotWord
            uint8_t k = temp[0];
            temp[0] = temp[1];
            temp[1] = temp[2];
            temp[2] = temp[3];
            temp[3] = k;
            
            // SubWord
            temp[0] = sbox[temp[0]];
            temp[1] = sbox[temp[1]];
            temp[2] = sbox[temp[2]];
            temp[3] = sbox[temp[3]];
            
            temp[0] ^= Rcon[i / Nk];
        } else if (i % Nk == 4) {
            // SubWord for AES-256
            temp[0] = sbox[temp[0]];
            temp[1] = sbox[temp[1]];
            temp[2] = sbox[temp[2]];
            temp[3] = sbox[temp[3]];
        }
        
        expandedKey[i * 4] = expandedKey[(i - Nk) * 4] ^ temp[0];
        expandedKey[i * 4 + 1] = expandedKey[(i - Nk) * 4 + 1] ^ temp[1];
        expandedKey[i * 4 + 2] = expandedKey[(i - Nk) * 4 + 2] ^ temp[2];
        expandedKey[i * 4 + 3] = expandedKey[(i - Nk) * 4 + 3] ^ temp[3];
        i++;
    }
}

// AES encryption of a single block
static void AES_Encrypt_Block(const uint8_t* input, uint8_t* output, const uint8_t* expandedKey) {
    uint8_t state[4][4];
    
    // Copy input to state
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            state[j][i] = input[i * 4 + j];
        }
    }
    
    // Initial round
    AddRoundKey(state, expandedKey);
    
    // Main rounds
    for (int round = 1; round < Nr; round++) {
        SubBytes(state);
        ShiftRows(state);
        MixColumns(state);
        AddRoundKey(state, expandedKey + round * 16);
    }
    
    // Final round (no MixColumns)
    SubBytes(state);
    ShiftRows(state);
    AddRoundKey(state, expandedKey + Nr * 16);
    
    // Copy state to output
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            output[i * 4 + j] = state[j][i];
        }
    }
}

// AES decryption of a single block
static void AES_Decrypt_Block(const uint8_t* input, uint8_t* output, const uint8_t* expandedKey) {
    uint8_t state[4][4];
    
    // Copy input to state
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            state[j][i] = input[i * 4 + j];
        }
    }
    
    // Initial round
    AddRoundKey(state, expandedKey + Nr * 16);
    
    // Main rounds
    for (int round = Nr - 1; round > 0; round--) {
        InvShiftRows(state);
        InvSubBytes(state);
        AddRoundKey(state, expandedKey + round * 16);
        InvMixColumns(state);
    }
    
    // Final round (no InvMixColumns)
    InvShiftRows(state);
    InvSubBytes(state);
    AddRoundKey(state, expandedKey);
    
    // Copy state to output
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            output[i * 4 + j] = state[j][i];
        }
    }
}

// Simple password-to-key derivation
static void derive_key_from_password(const char* password, uint8_t* key) {
    memset(key, 0, AES_KEY_SIZE);
    size_t len = strlen(password);
    
    if (len == 0) {
        for (int i = 0; i < AES_KEY_SIZE; i++) {
            key[i] = i;
        }
        return;
    }
    
    // Repeat password and XOR with index-based values
    for (size_t i = 0; i < AES_KEY_SIZE; i++) {
        key[i] = password[i % len] ^ (uint8_t)(i * 7 + 13);
    }
    
    // Additional mixing rounds
    for (int round = 0; round < 100; round++) {
        for (int i = 0; i < AES_KEY_SIZE; i++) {
            key[i] = sbox[key[i]] ^ key[(i + 7) % AES_KEY_SIZE];
        }
    }
}

// PKCS7 padding
static size_t add_padding(uint8_t* data, size_t dataLen, size_t bufferSize) {
    size_t paddingLen = AES_BLOCK_SIZE - (dataLen % AES_BLOCK_SIZE);
    if (dataLen + paddingLen > bufferSize) {
        return 0;
    }
    
    for (size_t i = 0; i < paddingLen; i++) {
        data[dataLen + i] = (uint8_t)paddingLen;
    }
    
    return dataLen + paddingLen;
}

// Remove PKCS7 padding
static size_t remove_padding(const uint8_t* data, size_t dataLen) {
    if (dataLen == 0 || dataLen % AES_BLOCK_SIZE != 0) {
        return 0;
    }
    
    uint8_t paddingLen = data[dataLen - 1];
    
    if (paddingLen == 0 || paddingLen > AES_BLOCK_SIZE) {
        return 0;
    }
    
    for (size_t i = dataLen - paddingLen; i < dataLen; i++) {
        if (data[i] != paddingLen) {
            return 0;
        }
    }
    
    return dataLen - paddingLen;
}

// Encrypt file
int aes_encrypt_file(const char* inputPath, const char* outputPath, const char* password) {
    FILE* in = fopen(inputPath, "rb");
    if (!in) {
        fprintf(stderr, "Cannot open input file '%s': ", inputPath);
        perror("");
        return -1;
    }
    
    fseek(in, 0, SEEK_END);
    long fileSize = ftell(in);
    rewind(in);
    
    uint8_t* fileData = (uint8_t*)malloc(fileSize + AES_BLOCK_SIZE);
    if (!fileData) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(in);
        return -1;
    }
    
    if (fread(fileData, 1, fileSize, in) != (size_t)fileSize) {
        fprintf(stderr, "Failed to read file\n");
        free(fileData);
        fclose(in);
        return -1;
    }
    fclose(in);
    
    size_t paddedSize = add_padding(fileData, fileSize, fileSize + AES_BLOCK_SIZE);
    if (paddedSize == 0) {
        fprintf(stderr, "Padding failed\n");
        free(fileData);
        return -1;
    }
    
    uint8_t key[AES_KEY_SIZE];
    derive_key_from_password(password, key);
    
    uint8_t expandedKey[Nb * (Nr + 1) * 4];
    KeyExpansion(key, expandedKey);
    
    uint8_t* encrypted = (uint8_t*)malloc(paddedSize);
    if (!encrypted) {
        fprintf(stderr, "Memory allocation failed\n");
        free(fileData);
        return -1;
    }
    
    for (size_t i = 0; i < paddedSize; i += AES_BLOCK_SIZE) {
        AES_Encrypt_Block(fileData + i, encrypted + i, expandedKey);
    }
    
    FILE* out = fopen(outputPath, "wb");
    if (!out) {
        fprintf(stderr, "Cannot create output file '%s': ", outputPath);
        perror("");
        free(fileData);
        free(encrypted);
        return -1;
    }
    
    FileMetadata meta = {
        .magic = METADATA_MAGIC,
        .originalSize = fileSize,
        .flags = 0x02
    };
    strncpy(meta.originalName, get_basename(inputPath), MAX_FILENAME_LEN - 1);
    meta.originalName[MAX_FILENAME_LEN - 1] = '\0';
    
    if (fwrite(&meta, sizeof(meta), 1, out) != 1) {
        fprintf(stderr, "Failed to write metadata\n");
        fclose(out);
        free(fileData);
        free(encrypted);
        return -1;
    }
    
    if (fwrite(encrypted, 1, paddedSize, out) != paddedSize) {
        fprintf(stderr, "Failed to write encrypted data\n");
        fclose(out);
        free(fileData);
        free(encrypted);
        return -1;
    }
    
    fclose(out);
    free(fileData);
    free(encrypted);
    
    memset(key, 0, sizeof(key));
    memset(expandedKey, 0, sizeof(expandedKey));
    
    return 0;
}

// Decrypt file
int aes_decrypt_file(const char* inputPath, const char* outputPath, const char* password) {
    FILE* in = fopen(inputPath, "rb");
    if (!in) {
        fprintf(stderr, "Cannot open input file '%s': ", inputPath);
        perror("");
        return -1;
    }
    
    FileMetadata meta;
    if (fread(&meta, sizeof(meta), 1, in) != 1 || meta.magic != METADATA_MAGIC) {
        fprintf(stderr, "Invalid or corrupted encrypted file\n");
        fclose(in);
        return -1;
    }
    
    fseek(in, 0, SEEK_END);
    long totalSize = ftell(in);
    long encryptedSize = totalSize - sizeof(meta);
    fseek(in, sizeof(meta), SEEK_SET);
    
    if (encryptedSize <= 0 || encryptedSize % AES_BLOCK_SIZE != 0) {
        fprintf(stderr, "Invalid encrypted data size\n");
        fclose(in);
        return -1;
    }
    
    uint8_t* encrypted = (uint8_t*)malloc(encryptedSize);
    if (!encrypted) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(in);
        return -1;
    }
    
    if (fread(encrypted, 1, encryptedSize, in) != (size_t)encryptedSize) {
        fprintf(stderr, "Failed to read encrypted data\n");
        free(encrypted);
        fclose(in);
        return -1;
    }
    fclose(in);
    
    uint8_t key[AES_KEY_SIZE];
    derive_key_from_password(password, key);
    
    uint8_t expandedKey[Nb * (Nr + 1) * 4];
    KeyExpansion(key, expandedKey);
    
    uint8_t* decrypted = (uint8_t*)malloc(encryptedSize);
    if (!decrypted) {
        fprintf(stderr, "Memory allocation failed\n");
        free(encrypted);
        return -1;
    }
    
    for (size_t i = 0; i < (size_t)encryptedSize; i += AES_BLOCK_SIZE) {
        AES_Decrypt_Block(encrypted + i, decrypted + i, expandedKey);
    }
    
    size_t originalSize = remove_padding(decrypted, encryptedSize);
    if (originalSize == 0) {
        fprintf(stderr, "Decryption failed (wrong password or corrupted file)\n");
        free(encrypted);
        free(decrypted);
        return -1;
    }
    
    FILE* out = fopen(outputPath, "wb");
    if (!out) {
        fprintf(stderr, "Cannot create output file '%s': ", outputPath);
        perror("");
        free(encrypted);
        free(decrypted);
        return -1;
    }
    
    if (fwrite(decrypted, 1, originalSize, out) != originalSize) {
        fprintf(stderr, "Failed to write decrypted data\n");
        fclose(out);
        free(encrypted);
        free(decrypted);
        return -1;
    }
    
    fclose(out);
    free(encrypted);
    free(decrypted);
    
    memset(key, 0, sizeof(key));
    memset(expandedKey, 0, sizeof(expandedKey));
    
    return 0;
}
