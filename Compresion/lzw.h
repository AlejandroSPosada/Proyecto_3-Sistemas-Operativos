#ifndef LZW_H
#define LZW_H

#include <stdio.h>

/**
 * LZW compression interface compatible with the project's other compressors.
 * writeLZW: compress input file and write to specified output file
 * readLZW: read a .lzw file and write decompressed bytes to specified output file
 */
void writeLZW(char inputFile[], char outputFile[]);
int readLZW(char inputFile[], char outputFile[]);

#endif
