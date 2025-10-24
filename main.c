#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "comp_folder/huffman.h"
#include "enc_folder/vigenere.h"

int main() {
    char opcion[30];
    char compalg[30];
    char encalg[30];

    printf("Seleccione opcion:\n");
    printf(" -c (comprimir)\n -d (descomprimir)\n -e (encriptar)\n -u (desencriptar)\n -ce (comprimir y luego encriptar)\n");
    printf("Opcion: ");
    scanf("%s", opcion);

    if (strcmp(opcion, "-c") == 0){ 
        printf("Algoritmos de compresion: \n - vigenere ");
        scanf("%s", compalg);
    }
    else if (strcmp(opcion, "-d") == 0) printf("je");
    else if (strcmp(opcion, "-e") == 0) printf("ji");
    else if (strcmp(opcion, "-u") == 0) printf("jo");
    else if (strcmp(opcion, "-ce") == 0) printf("ju");
    else printf("Opcion no valida.\n");

    runHuffman("ADACADABRA!");
    return 0;
}
