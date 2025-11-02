#ifndef LZW_H
#define LZW_H

#include <stdio.h>

/**
 * LZW compression interface compatible with the project's other compressors.
 * writeLZW: compress input file and write to File_Manager/output.lzw
 * readLZW: read a .lzw file and write decompressed bytes to File_Manager/output.txt
 */
void writeLZW(char inputFile[]);
int readLZW(char inputFile[]);

#endif
