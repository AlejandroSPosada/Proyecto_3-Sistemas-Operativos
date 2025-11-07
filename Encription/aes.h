#ifndef AES_H
#define AES_H

#include <stddef.h>
#include <stdint.h>
#include "../common.h"

/**
 * AES-256 encryption/decryption implementation from scratch
 * 
 * Key size: 256 bits (32 bytes)
 * Block size: 128 bits (16 bytes)
 * Mode: ECB (Electronic Codebook) - simple implementation
 * Padding: PKCS7
 * 
 * File format:
 * [FileMetadata (272 bytes)]
 * [Encrypted data blocks (multiple of 16 bytes)]
 */

#define AES_BLOCK_SIZE 16
#define AES_KEY_SIZE 32    // 256 bits

/**
 * Encrypt a file using AES-256-ECB
 * 
 * @param inputPath Path to input file
 * @param outputPath Path to output encrypted file
 * @param password Password for encryption (will be hashed to 256-bit key)
 * @return 0 on success, -1 on error
 */
int aes_encrypt_file(const char* inputPath, const char* outputPath, const char* password);

/**
 * Decrypt a file using AES-256-ECB
 * 
 * @param inputPath Path to encrypted input file
 * @param outputPath Path to output decrypted file
 * @param password Password for decryption
 * @return 0 on success, -1 on error
 */
int aes_decrypt_file(const char* inputPath, const char* outputPath, const char* password);

#endif
