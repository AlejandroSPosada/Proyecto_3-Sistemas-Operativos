#ifndef RLE_H
#define RLE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// RLE (Run-Length Encoding) Compression Functions
// Simple and efficient for data with many repeated sequences

/**
 * Compress a file using Run-Length Encoding
 * Format: [count][byte] pairs where count is uint32_t
 * 
 * @param inputFile Path to the input file
 * @param outputFile Path to the output file
 */
void writeRLE(char inputFile[], char outputFile[]);

/**
 * Decompress an RLE-compressed file
 * 
 * @param inputFile Path to the compressed .rle file
 * @param outputFile Path to the output file
 * @return 0 on success, 1 on error
 */
int readRLE(char inputFile[], char outputFile[]);

#endif
