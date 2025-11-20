#ifndef RLE_H
#define RLE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// RLE (Codificación de Longitud de Ejecución) Funciones de Compresión
// Simple y eficiente para datos con muchas secuencias repetidas

/**
 * Comprime un archivo usando Codificación de Longitud de Ejecución
 * Formato: pares [count][byte] donde count es uint32_t
 * 
 * @param inputFile Ruta del archivo de entrada
 * @param outputFile Ruta del archivo de salida
 */
void writeRLE(char inputFile[], char outputFile[]);

/**
 * Descomprime un archivo comprimido con RLE
 * 
 * @param inputFile Ruta del archivo .rle comprimido
 * @param outputFile Ruta del archivo de salida
 * @return 0 en éxito, 1 en error
 */
int readRLE(char inputFile[], char outputFile[]);

#endif
