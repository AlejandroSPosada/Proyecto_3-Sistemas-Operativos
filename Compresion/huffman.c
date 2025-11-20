#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "huffman.h"
#include "../common.h"
#include "../posix_utils.h"

#define MAX_TREE_HT 512
#define MAX_CHARS 256

// ===== BitWriter para POSIX =====
typedef struct {
    int fd;              // File descriptor en lugar de FILE*
    uint8_t buffer;      // Buffer de 1 byte
    int bitCount;        // Bits usados en el buffer (0-7)
    int totalBits;       // Total de bits escritos
} BitWriter_POSIX;

// Inicializar BitWriter con file descriptor
static void bitwriter_init_posix(BitWriter_POSIX* bw, int fd) {
    bw->fd = fd;
    bw->buffer = 0;
    bw->bitCount = 0;
    bw->totalBits = 0;
}

// Escribir un bit
static void bitwriter_write_bit_posix(BitWriter_POSIX* bw, int bit) {
    bw->buffer <<= 1;
    if (bit) bw->buffer |= 1;
    bw->bitCount++;
    bw->totalBits++;
    
    if (bw->bitCount == 8) {
        posix_write_full(bw->fd, &bw->buffer, 1);
        bw->buffer = 0;
        bw->bitCount = 0;
    }
}

// Flush bits pendientes
static void bitwriter_flush_posix(BitWriter_POSIX* bw) {
    if (bw->bitCount > 0) {
        bw->buffer <<= (8 - bw->bitCount);
        posix_write_full(bw->fd, &bw->buffer, 1);
        bw->buffer = 0;
        bw->bitCount = 0;
    }
}

// ----- Node structure -----
struct MinHeapNode* newNode(char data, unsigned freq) {
    struct MinHeapNode* temp = (struct MinHeapNode*)malloc(sizeof(struct MinHeapNode));
    if (!temp) {
        perror("malloc newNode");
        return NULL;
    }
    temp->left = temp->right = NULL;
    temp->data = data;
    temp->freq = freq;
    return temp;
}

// Free Huffman tree recursively
void freeHuffmanTree(struct MinHeapNode* root) {
    if (!root) return;
    freeHuffmanTree(root->left);
    freeHuffmanTree(root->right);
    free(root);
}

// Free MinHeap structure
void freeMinHeap(struct MinHeap* minHeap) {
    if (!minHeap) return;
    if (minHeap->array) free(minHeap->array);
    free(minHeap);
}

// ----- MinHeap -----
struct MinHeap* createMinHeap(unsigned capacity) {
    struct MinHeap* minHeap = (struct MinHeap*)malloc(sizeof(struct MinHeap));
    if (!minHeap) {
        perror("malloc MinHeap");
        return NULL;
    }
    minHeap->size = 0;
    minHeap->capacity = capacity;
    minHeap->array = (struct MinHeapNode**)malloc(minHeap->capacity * sizeof(struct MinHeapNode*));
    if (!minHeap->array) {
        perror("malloc MinHeap array");
        free(minHeap);
        return NULL;
    }
    return minHeap;
}

void swapMinHeapNode(struct MinHeapNode** a, struct MinHeapNode** b) {
    struct MinHeapNode* t = *a;
    *a = *b;
    *b = t;
}

void minHeapify(struct MinHeap* minHeap, int idx) {
    int smallest = idx;
    int left = 2 * idx + 1;
    int right = 2 * idx + 2;

    if (left < (int)minHeap->size && minHeap->array[left]->freq < minHeap->array[smallest]->freq)
        smallest = left;

    if (right < (int)minHeap->size && minHeap->array[right]->freq < minHeap->array[smallest]->freq)
        smallest = right;

    if (smallest != idx) {
        swapMinHeapNode(&minHeap->array[smallest], &minHeap->array[idx]);
        minHeapify(minHeap, smallest);
    }
}

int isSizeOne(struct MinHeap* minHeap) {
    return (minHeap->size == 1);
}

struct MinHeapNode* extractMin(struct MinHeap* minHeap) {
    struct MinHeapNode* temp = minHeap->array[0];
    minHeap->array[0] = minHeap->array[minHeap->size - 1];
    --minHeap->size;
    minHeapify(minHeap, 0);
    return temp;
}

void insertMinHeap(struct MinHeap* minHeap, struct MinHeapNode* minHeapNode) {
    ++minHeap->size;
    int i = minHeap->size - 1;
    while (i && minHeapNode->freq < minHeap->array[(i - 1) / 2]->freq) {
        minHeap->array[i] = minHeap->array[(i - 1) / 2];
        i = (i - 1) / 2;
    }
    minHeap->array[i] = minHeapNode;
}

void buildMinHeap(struct MinHeap* minHeap) {
    int n = minHeap->size - 1;
    for (int i = (n - 1) / 2; i >= 0; i--)
        minHeapify(minHeap, i);
}

int isLeaf(struct MinHeapNode* root) {
    return !(root->left) && !(root->right);
}

struct MinHeap* createAndBuildMinHeap(char data[], int freq[], int size) {
    struct MinHeap* minHeap = createMinHeap(size);
    if (!minHeap) return NULL;
    for (int i = 0; i < size; ++i) {
        minHeap->array[i] = newNode(data[i], freq[i]);
        if (!minHeap->array[i]) {
            // Cleanup on failure
            for (int j = 0; j < i; j++) free(minHeap->array[j]);
            freeMinHeap(minHeap);
            return NULL;
        }
    }
    minHeap->size = size;
    buildMinHeap(minHeap);
    return minHeap;
}

struct MinHeapNode* buildHuffmanTree(char data[], int freq[], int size) {
    struct MinHeapNode *left, *right, *top;
    struct MinHeap* minHeap = createAndBuildMinHeap(data, freq, size);
    if (!minHeap) return NULL;

    // Handle single character case
    if (size == 1) {
        top = newNode('$', freq[0]);
        if (top) {
            top->left = minHeap->array[0];
            top->right = NULL;
        }
        freeMinHeap(minHeap);
        return top;
    }

    while (!isSizeOne(minHeap)) {
        left = extractMin(minHeap);
        right = extractMin(minHeap);
        top = newNode('$', left->freq + right->freq);
        if (!top) {
            freeMinHeap(minHeap);
            return NULL;
        }
        top->left = left;
        top->right = right;
        insertMinHeap(minHeap, top);
    }
    
    struct MinHeapNode* root = extractMin(minHeap);
    freeMinHeap(minHeap);
    return root;
}

// --- Build Huffman Codes ---
void storeCodes(struct MinHeapNode* root, int arr[], int top, char codes[][MAX_TREE_HT]) {
    if (root->left) {
        arr[top] = 0;
        storeCodes(root->left, arr, top + 1, codes);
    }

    if (root->right) {
        arr[top] = 1;
        storeCodes(root->right, arr, top + 1, codes);
    }

    if (isLeaf(root)) {
        for (int i = 0; i < top; i++)
            codes[(unsigned char)root->data][i] = arr[i] + '0';
        codes[(unsigned char)root->data][top] = '\0';
    }
}

void HuffmanCodes(char data[], int freq[], int size, char codes[][MAX_TREE_HT]) {
    struct MinHeapNode* root = buildHuffmanTree(data, freq, size);
    if (!root) {
        fprintf(stderr, "Error al construir árbol de Huffman\n");
        return;
    }
    int arr[MAX_TREE_HT], top = 0;
    
    // Special case: single character
    if (size == 1) {
        codes[(unsigned char)data[0]][0] = '0';
        codes[(unsigned char)data[0]][1] = '\0';
    } else {
        storeCodes(root, arr, top, codes);
    }
    
    freeHuffmanTree(root);
}

//(POSIX VERSION)
void writeHuffman(char inputFile[], char outputFile[]) {
    // Abrir archivo de entrada con POSIX
    int fd_input = posix_open_read(inputFile);
    if (fd_input == -1) return;

    // Obtener tamaño del archivo
    off_t fileSize = posix_get_file_size(fd_input);
    if (fileSize <= 0) {
        fprintf(stderr, "Archivo '%s' está vacío o es inválido\n", inputFile);
        posix_close(fd_input);
        return;
    }

    // Allocar buffer para todo el archivo
    char* arr = (char*)malloc(fileSize);
    if (!arr) {
        fprintf(stderr, "Falló asignación de memoria: %s\n", strerror(errno));
        posix_close(fd_input);
        return;
    }
    
    ssize_t bytes_read = posix_read_full(fd_input, arr, fileSize);
    posix_close(fd_input);

    if (bytes_read != fileSize) {
        fprintf(stderr, "Falló lectura desde '%s'\n", inputFile);
        free(arr);
        return;
    }

    uint32_t freq[MAX_CHARS] = {0};

    for (ssize_t i = 0; i < bytes_read; i++)
        freq[(unsigned char)arr[i]]++;

    char chars[MAX_CHARS];
    uint32_t freqs[MAX_CHARS];
    uint32_t size = 0;

    for (int i = 0; i < MAX_CHARS; i++) {
        if (freq[i] > 0) {
            chars[size] = (char)i;
            freqs[size] = (uint32_t)freq[i];
            size++;
        }
    }

    if (size == 0) {
        fprintf(stderr, "Sin caracteres para codificar\n");
        free(arr);
        return;
    }

    char codes[256][MAX_TREE_HT] = {{0}};
    // Convert uint32_t to int for HuffmanCodes
    int freqs_int[MAX_CHARS];
    for (uint32_t i = 0; i < size; i++) {
        freqs_int[i] = (int)freqs[i];
    }
    HuffmanCodes(chars, freqs_int, size, codes);

    // Abrir archivo de salida con POSIX
    int fd_output = posix_open_write(outputFile);
    if (fd_output == -1) {
        free(arr);
        return;
    }

    // Escribir metadata header
    FileMetadata meta = {0};
    meta.magic = METADATA_MAGIC;
    meta.originalSize = (uint64_t)bytes_read;
    meta.flags = 0;
    strncpy(meta.originalName, get_basename(inputFile), MAX_FILENAME_LEN-1);
    meta.originalName[MAX_FILENAME_LEN-1] = '\0';
    
    if (posix_write_full(fd_output, &meta, sizeof(meta)) != sizeof(meta)) {
        fprintf(stderr, "Falló escritura de metadatos\n");
        posix_close(fd_output);
        free(arr);
        return;
    }

    // Escribir header con tipos de tamaño fijo
    if (posix_write_full(fd_output, &size, sizeof(uint32_t)) != sizeof(uint32_t)) {
        fprintf(stderr, "Falló escritura de tamaño\n");
        posix_close(fd_output);
        free(arr);
        return;
    }
    
    for (uint32_t i = 0; i < size; i++) {
        if (posix_write_full(fd_output, &chars[i], sizeof(char)) != sizeof(char) ||
            posix_write_full(fd_output, &freqs[i], sizeof(uint32_t)) != sizeof(uint32_t)) {
            fprintf(stderr, "Falló escritura de car/freq\n");
            posix_close(fd_output);
            free(arr);
            return;
        }
    }

    // Guardar posición para totalBits (lo sobreescribiremos después)
    off_t bitCountPos = lseek(fd_output, 0, SEEK_CUR);
    uint32_t placeholder = 0;
    posix_write_full(fd_output, &placeholder, sizeof(uint32_t));

    // Inicializar BitWriter con POSIX
    BitWriter_POSIX bw;
    bitwriter_init_posix(&bw, fd_output);

    // Codificar texto en bits
    for (ssize_t i = 0; i < bytes_read; i++) {
        char* code = codes[(unsigned char)arr[i]];
        for (int j = 0; code[j] != '\0'; j++) {
            bitwriter_write_bit_posix(&bw, code[j] == '1' ? 1 : 0);
        }
    }

    // Flush bits pendientes
    bitwriter_flush_posix(&bw);
    int totalBits = bw.totalBits;

    // Escribir totalBits en la posición reservada
    lseek(fd_output, bitCountPos, SEEK_SET);
    posix_write_full(fd_output, &totalBits, sizeof(uint32_t));

    posix_close(fd_output);
    free(arr);
}

// ---- Decompress function ----
// ---- Decompress function (POSIX VERSION) ----
void decodeHuffman(struct MinHeapNode* root, unsigned char* data, int dataSizeBits, int fd_output) {
    if (!root) return;
    
    // Handle single character case
    if (!root->left && !root->right) {
        for (int i = 0; i < dataSizeBits; i++) {
            unsigned char ch = (unsigned char)root->data;
            posix_write_full(fd_output, &ch, 1);
        }
        return;
    }
    
    struct MinHeapNode* current = root;

    for (int i = 0; i < dataSizeBits; i++) {
        int byteIndex = i / 8;
        int bitIndex = 7 - (i % 8);
        int bit = (data[byteIndex] >> bitIndex) & 1;

        if (bit == 0)
            current = current->left;
        else
            current = current->right;

        if (!current) {
            fprintf(stderr, "Error de decodificación: travesía de árbol inválida\n");
            return;
        }

        if (isLeaf(current)) {
            unsigned char ch = (unsigned char)current->data;
            posix_write_full(fd_output, &ch, 1);
            current = root;
        }
    }
}

int readHuffman(char inputFile[], char outputFile[]) {
    // Abrir archivo comprimido con POSIX
    int fd_input = posix_open_read(inputFile);
    if (fd_input == -1) return 1;

    // Leer metadata header
    FileMetadata meta;
    if (posix_read_full(fd_input, &meta, sizeof(meta)) != sizeof(meta) ||
        meta.magic != METADATA_MAGIC) {
        fprintf(stderr, "Metadatos faltantes o inválidos en archivo comprimido\n");
        posix_close(fd_input);
        return 1;
    }

    uint32_t size;
    if (posix_read_full(fd_input, &size, sizeof(uint32_t)) != sizeof(uint32_t)) {
        fprintf(stderr, "Falló lectura de tamaño\n");
        posix_close(fd_input);
        return 1;
    }

    if (size == 0 || size > MAX_CHARS) {
        fprintf(stderr, "Tamaño inválido en archivo comprimido: %u\n", size);
        posix_close(fd_input);
        return 1;
    }

    char* chars = (char*)malloc(size);
    uint32_t* freqs = (uint32_t*)malloc(size * sizeof(uint32_t));
    
    if (!chars || !freqs) {
        fprintf(stderr, "Falló asignación de memoria: %s\n", strerror(errno));
        free(chars);
        free(freqs);
        posix_close(fd_input);
        return 1;
    }

    for (uint32_t i = 0; i < size; i++) {
        if (posix_read_full(fd_input, &chars[i], sizeof(char)) != sizeof(char)) {
            fprintf(stderr, "Falló lectura de carácter\n");
            posix_close(fd_input);
            free(chars); free(freqs);
            return 1;
        }
        if (posix_read_full(fd_input, &freqs[i], sizeof(uint32_t)) != sizeof(uint32_t)) {
            fprintf(stderr, "Falló lectura de frecuencia\n");
            posix_close(fd_input);
            free(chars); free(freqs);
            return 1;
        }
    }

    // Leer totalBits
    uint32_t totalBits;
    if (posix_read_full(fd_input, &totalBits, sizeof(uint32_t)) != sizeof(uint32_t)) {
        fprintf(stderr, "Falló lectura de totalBits\n");
        posix_close(fd_input);
        free(chars); free(freqs);
        return 1;
    }

    // Calcular cuántos bytes de datos codificados quedan
    off_t fileSize = posix_get_file_size(fd_input);
    off_t dataStart = sizeof(meta) + sizeof(uint32_t) + size * (sizeof(char) + sizeof(uint32_t)) + sizeof(uint32_t);
    off_t bytesToRead = fileSize - dataStart;
    
    if (bytesToRead <= 0) {
        fprintf(stderr, "Formato de archivo comprimido inválido\n");
        posix_close(fd_input);
        free(chars); free(freqs);
        return 1;
    }

    unsigned char* encodedData = (unsigned char*)malloc(bytesToRead);
    if (!encodedData) {
        fprintf(stderr, "Falló asignación de memoria: %s\n", strerror(errno));
        posix_close(fd_input);
        free(chars); free(freqs);
        return 1;
    }
    
    ssize_t bytes_read = posix_read_full(fd_input, encodedData, bytesToRead);
    posix_close(fd_input);
    
    if (bytes_read != bytesToRead) {
        fprintf(stderr, "Falló lectura de datos codificados\n");
        free(chars); free(freqs); free(encodedData);
        return 1;
    }

    // Reconstruir árbol - convertir uint32_t a int
    int freqs_int[MAX_CHARS];
    for (uint32_t i = 0; i < size; i++) {
        freqs_int[i] = (int)freqs[i];
    }
    struct MinHeapNode* root = buildHuffmanTree(chars, freqs_int, size);
    
    if (!root) {
        fprintf(stderr, "Falló reconstrucción de árbol de Huffman\n");
        free(chars); free(freqs); free(encodedData);
        return 1;
    }

    // Abrir archivo de salida con POSIX
    int fd_output = posix_open_write(outputFile);
    if (fd_output == -1) {
        freeHuffmanTree(root);
        free(chars); free(freqs); free(encodedData);
        return 1;
    }
    
    // Decodificar usando totalBits
    decodeHuffman(root, encodedData, totalBits, fd_output);
    posix_close(fd_output);

    freeHuffmanTree(root);
    free(chars);
    free(freqs);
    free(encodedData);
    return 0;
}