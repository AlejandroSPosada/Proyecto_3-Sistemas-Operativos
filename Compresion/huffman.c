#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "huffman.h"

#define MAX_TREE_HT 100
#define MAX_CHARS 300

// ----- Node structure -----
struct MinHeapNode* newNode(char data, unsigned freq) {
    struct MinHeapNode* temp = (struct MinHeapNode*)malloc(sizeof(struct MinHeapNode));
    temp->left = temp->right = NULL;
    temp->data = data;
    temp->freq = freq;
    return temp;
}

// ----- MinHeap -----
struct MinHeap* createMinHeap(unsigned capacity) {
    struct MinHeap* minHeap = (struct MinHeap*)malloc(sizeof(struct MinHeap));
    minHeap->size = 0;
    minHeap->capacity = capacity;
    minHeap->array = (struct MinHeapNode**)malloc(minHeap->capacity * sizeof(struct MinHeapNode*));
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
    for (int i = 0; i < size; ++i)
        minHeap->array[i] = newNode(data[i], freq[i]);
    minHeap->size = size;
    buildMinHeap(minHeap);
    return minHeap;
}

struct MinHeapNode* buildHuffmanTree(char data[], int freq[], int size) {
    struct MinHeapNode *left, *right, *top;
    struct MinHeap* minHeap = createAndBuildMinHeap(data, freq, size);

    while (!isSizeOne(minHeap)) {
        left = extractMin(minHeap);
        right = extractMin(minHeap);
        top = newNode('$', left->freq + right->freq);
        top->left = left;
        top->right = right;
        insertMinHeap(minHeap, top);
    }
    return extractMin(minHeap);
}

// --- Build Huffman Codes ---
void storeCodes(struct MinHeapNode* root, int arr[], int top, char codes[256][MAX_TREE_HT]) {
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

void HuffmanCodes(char data[], int freq[], int size, char codes[256][MAX_TREE_HT]) {
    struct MinHeapNode* root = buildHuffmanTree(data, freq, size);
    int arr[MAX_TREE_HT], top = 0;
    storeCodes(root, arr, top, codes);
}

// --- Write in .bin ---
void writeHuffman(char inputFile[]) {
    FILE* input = fopen(inputFile, "r");
    if (!input) { perror("Cannot open input file"); return; }

    char arr[10000]; // adjust size as needed
    int len = fread(arr, 1, sizeof(arr) - 1, input);
    arr[len] = '\0'; // null terminate
    fclose(input);

    int freq[MAX_CHARS] = {0};

    for (int i = 0; i < len; i++)
        freq[(unsigned char)arr[i]]++;

    char chars[MAX_CHARS];
    int freqs[MAX_CHARS];
    int size = 0;

    for (int i = 0; i < MAX_CHARS; i++) {
        if (freq[i] > 0) {
            chars[size] = (char)i;
            freqs[size] = freq[i];
            size++;
        }
    }

    char codes[256][MAX_TREE_HT] = {{0}};
    HuffmanCodes(chars, freqs, size, codes);

    // Encode text into bits
    unsigned char buffer = 0;
    int bitCount = 0;
    int totalBits = 0;

    FILE* out = fopen("File_Manager/output.bin", "wb");
    if (!out) { perror("Cannot open output.bin"); return; }

    // Write header
    fwrite(&size, sizeof(int), 1, out);
    for (int i = 0; i < size; i++) {
        fwrite(&chars[i], sizeof(char), 1, out);
        fwrite(&freqs[i], sizeof(int), 1, out);
    }

    // Reserve space for totalBits (we’ll overwrite later)
    long bitCountPos = ftell(out);
    int placeholder = 0;
    fwrite(&placeholder, sizeof(int), 1, out);

    for (int i = 0; i < len; i++) {
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
    fwrite(&totalBits, sizeof(int), 1, out);

    fclose(out);
    printf("Compressed data saved to output.bin\n");
}

// ---- Decompress function ----
void decodeHuffman(struct MinHeapNode* root, unsigned char* data, int dataSizeBits, FILE* outTxt) {
    struct MinHeapNode* current = root;

    for (int i = 0; i < dataSizeBits; i++) {
        int byteIndex = i / 8;
        int bitIndex = 7 - (i % 8);
        int bit = (data[byteIndex] >> bitIndex) & 1;

        if (bit == 0)
            current = current->left;
        else
            current = current->right;

        if (isLeaf(current)) {
            fputc(current->data, outTxt);
            current = root;
        }
    }
}

int readHuffman(char arr[] ) {
    FILE* in = fopen(arr, "rb");
    if (!in) {
        perror("Cannot open output.bin");
        return 1;
    }

    int size;
    fread(&size, sizeof(int), 1, in);

    char* chars = (char*)malloc(size);
    int* freqs = (int*)malloc(size * sizeof(int));

    for (int i = 0; i < size; i++) {
        fread(&chars[i], sizeof(char), 1, in);
        fread(&freqs[i], sizeof(int), 1, in);
    }

    // Read totalBits
    int totalBits;
    fread(&totalBits, sizeof(int), 1, in);

    // Read encoded data
    fseek(in, 0, SEEK_END);
    long endPos = ftell(in);
    long dataStart = sizeof(int) + size * (sizeof(char) + sizeof(int)) + sizeof(int);
    long bytesToRead = endPos - dataStart;
    fseek(in, dataStart, SEEK_SET);

    unsigned char* encodedData = (unsigned char*)malloc(bytesToRead);
    fread(encodedData, 1, bytesToRead, in);
    fclose(in);

    // Rebuild tree
    struct MinHeapNode* root = buildHuffmanTree(chars, freqs, size);

    // Decode using totalBits
    FILE* outTxt = fopen("File_Manager/output.txt", "w");
    decodeHuffman(root, encodedData, totalBits, outTxt);
    fclose(outTxt);

    printf("Decompressed data saved to output.txt\n");

    free(chars);
    free(freqs);
    free(encodedData);
    return 0;
}