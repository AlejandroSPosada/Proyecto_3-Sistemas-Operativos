#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MAX_TREE_HT
#define MAX_TREE_HT 512
#endif

// Nodo del árbol de Huffman
struct MinHeapNode {
    char data;               // carácter
    unsigned freq;           // frecuencia
    struct MinHeapNode *left, *right;
};

// Min-Heap (cola de prioridad)
struct MinHeap {
    unsigned size;
    unsigned capacity;
    struct MinHeapNode** array;
};

struct MinHeapNode* newNode(char data, unsigned freq);
struct MinHeap* createMinHeap(unsigned capacity) ;
void swapMinHeapNode();
void minHeapify();
int isSizeOne();
struct MinHeapNode* extractMin(struct MinHeap* minHeap);
void insertMinHeap(struct MinHeap* minHeap, struct MinHeapNode* minHeapNode);
void buildMinHeap(struct MinHeap* minHeap);
int isLeaf(struct MinHeapNode* root);
struct MinHeap* createAndBuildMinHeap(char data[], int freq[], int size);
struct MinHeapNode* buildHuffmanTree(char data[], int freq[], int size);
void printCodes(struct MinHeapNode* root, int arr[], int top);
void HuffmanCodes(char data[], int freq[], int size, char codes[][MAX_TREE_HT]);
void writeHuffman(char arr[]);
void decodeHuffman(struct MinHeapNode*, unsigned char*, int, FILE*);
int readHuffman(char arr[]);

#endif