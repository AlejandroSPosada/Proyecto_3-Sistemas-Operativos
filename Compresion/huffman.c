#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "huffman.h"

#define MAX_TREE_HT 512
#define MAX_CHARS 256

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
        fprintf(stderr, "Error building Huffman tree\n");
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

// --- Write in .bin ---
void writeHuffman(char inputFile[]) {
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
    char* arr = (char*)malloc(fileSize);
    if (!arr) {
        perror("malloc file buffer");
        fclose(input);
        return;
    }
    
    size_t len = fread(arr, 1, fileSize, input);
    fclose(input);

    if (len == 0) {
        fprintf(stderr, "Failed to read from '%s'\n", inputFile);
        free(arr);
        return;
    }

    uint32_t freq[MAX_CHARS] = {0};

    for (size_t i = 0; i < len; i++)
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
        fprintf(stderr, "No characters to encode\n");
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

    // Encode text into bits
    unsigned char buffer = 0;
    int bitCount = 0;
    int totalBits = 0;

    FILE* out = fopen("File_Manager/output.bin", "wb");
    if (!out) { 
        perror("Cannot open File_Manager/output.bin"); 
        free(arr);
        return; 
    }

    // Write header with fixed-size types
    fwrite(&size, sizeof(uint32_t), 1, out);
    for (uint32_t i = 0; i < size; i++) {
        fwrite(&chars[i], sizeof(char), 1, out);
        fwrite(&freqs[i], sizeof(uint32_t), 1, out);
    }

    // Reserve space for totalBits (we'll overwrite later)
    long bitCountPos = ftell(out);
    uint32_t placeholder = 0;
    fwrite(&placeholder, sizeof(uint32_t), 1, out);

    for (size_t i = 0; i < len; i++) {
        char* code = codes[(unsigned char)arr[i]];
        for (int j = 0; code[j] != '\0'; j++) {
            buffer <<= 1;
            if (code[j] == '1') buffer |= 1;
            bitCount++;
            totalBits++;
            if (bitCount == 8) {
                fwrite(&buffer, 1, 1, out);
                buffer = 0;
                bitCount = 0;
            }
        }
    }

    if (bitCount > 0) {
        buffer <<= (8 - bitCount);
        fwrite(&buffer, 1, 1, out);
    }

    // Write totalBits value
    fseek(out, bitCountPos, SEEK_SET);
    fwrite(&totalBits, sizeof(uint32_t), 1, out);

    fclose(out);
    free(arr);
}

// ---- Decompress function ----
void decodeHuffman(struct MinHeapNode* root, unsigned char* data, int dataSizeBits, FILE* outTxt) {
    if (!root) return;
    
    // Handle single character case
    if (!root->left && !root->right) {
        for (int i = 0; i < dataSizeBits; i++) {
            fputc(root->data, outTxt);
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
            fprintf(stderr, "Decode error: invalid tree traversal\n");
            return;
        }

        if (isLeaf(current)) {
            fputc(current->data, outTxt);
            current = root;
        }
    }
}

int readHuffman(char arr[] ) {
    FILE* in = fopen(arr, "rb");
    if (!in) {
        fprintf(stderr, "Cannot open file '%s': ", arr);
        perror("");
        return 1;
    }

    uint32_t size;
    if (fread(&size, sizeof(uint32_t), 1, in) != 1) {
        perror("fread size");
        fclose(in);
        return 1;
    }

    if (size == 0 || size > MAX_CHARS) {
        fprintf(stderr, "Invalid size in compressed file: %u\n", size);
        fclose(in);
        return 1;
    }

    char* chars = (char*)malloc(size);
    uint32_t* freqs = (uint32_t*)malloc(size * sizeof(uint32_t));
    
    if (!chars || !freqs) {
        perror("malloc for chars/freqs");
        free(chars);
        free(freqs);
        fclose(in);
        return 1;
    }

    for (uint32_t i = 0; i < size; i++) {
        if (fread(&chars[i], sizeof(char), 1, in) != 1) {
            perror("fread char");
            fclose(in);
            free(chars); free(freqs);
            return 1;
        }
        if (fread(&freqs[i], sizeof(uint32_t), 1, in) != 1) {
            perror("fread freq");
            fclose(in);
            free(chars); free(freqs);
            return 1;
        }
    }

    // Read totalBits
    uint32_t totalBits;
    if (fread(&totalBits, sizeof(uint32_t), 1, in) != 1) {
        perror("fread totalBits");
        fclose(in);
        free(chars); free(freqs);
        return 1;
    }

    // Read encoded data
    fseek(in, 0, SEEK_END);
    long endPos = ftell(in);
    long dataStart = sizeof(uint32_t) + size * (sizeof(char) + sizeof(uint32_t)) + sizeof(uint32_t);
    long bytesToRead = endPos - dataStart;
    
    if (bytesToRead <= 0) {
        fprintf(stderr, "Invalid compressed file format\n");
        fclose(in);
        free(chars); free(freqs);
        return 1;
    }
    
    fseek(in, dataStart, SEEK_SET);

    unsigned char* encodedData = (unsigned char*)malloc(bytesToRead);
    if (!encodedData) {
        perror("malloc encodedData");
        fclose(in);
        free(chars); free(freqs);
        return 1;
    }
    
    size_t rd = fread(encodedData, 1, bytesToRead, in);
    if (rd != (size_t)bytesToRead) {
        perror("fread encodedData");
        fclose(in);
        free(chars); free(freqs); free(encodedData);
        return 1;
    }
    fclose(in);

    // Rebuild tree - convert uint32_t to int
    int freqs_int[MAX_CHARS];
    for (uint32_t i = 0; i < size; i++) {
        freqs_int[i] = (int)freqs[i];
    }
    struct MinHeapNode* root = buildHuffmanTree(chars, freqs_int, size);
    
    if (!root) {
        fprintf(stderr, "Failed to rebuild Huffman tree\n");
        free(chars); free(freqs); free(encodedData);
        return 1;
    }

    // Decode using totalBits
    FILE* outTxt = fopen("File_Manager/output.txt", "w");
    if (!outTxt) {
        perror("Cannot open File_Manager/output.txt");
        freeHuffmanTree(root);
        free(chars); free(freqs); free(encodedData);
        return 1;
    }
    
    decodeHuffman(root, encodedData, totalBits, outTxt);
    fclose(outTxt);

    freeHuffmanTree(root);
    free(chars);
    free(freqs);
    free(encodedData);
    return 0;
}