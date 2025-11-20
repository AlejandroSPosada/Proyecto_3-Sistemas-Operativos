# Proyecto_4 — Gestor de Archivos, Compresión y Encriptación

Resumen breve
- Aplicación en C para comprimir/descomprimir y encriptar/desencriptar archivos, con soporte para procesamiento recursivo y multihilo.
- Entrada principal: [main.c](main.c). El comportamiento por archivo/carpeta y la combinación de operaciones se maneja en [OperationsFileManager/multiFeature.c](OperationsFileManager/multiFeature.c) y su cabecera [OperationsFileManager/multiFeature.h](OperationsFileManager/multiFeature.h).

Compilación
- Usar el [Makefile](Makefile) en la raíz del proyecto.

Uso (ejemplos)
- Comprimir con LZW: 
  ./programa -c --comp-alg lzw -i File_Manager/testing -o File_Manager/comprimido
- Descomprimir:
  ./programa -d --comp-alg lzw -i File_Manager/comprimido -o File_Manager/descomprimido
- Encriptar con vigenere:
  ./programa -e --enc-alg vigenere -i File_Manager/testing -o File_Manager/encriptado -k MiClave
- Desencriptar:
  ./programa -u --enc-alg vigenere -i File_Manager/encriptado -o File_Manager/desencriptado -k MiClave

Estructura principal y responsabilidades
- Ejecutable / flujo principal:
  - [main.c](main.c) — parseo de argumentos y creación de `ThreadArgs` para iniciar la operación con [`initOperation`](OperationsFileManager/multiFeature.c).
  - [`initOperation`](OperationsFileManager/multiFeature.c) / [`operationOneFile`](OperationsFileManager/multiFeature.c) — orquestan compresión/encriptación, recursión y creación de hilos. Ver [OperationsFileManager/multiFeature.h](OperationsFileManager/multiFeature.h) para la estructura [`ThreadArgs`](OperationsFileManager/multiFeature.h).

- Compresión (módulo Compresion/):
  - Huffman:
    - Implementación y utilidades: [Compresion/huffman.c](Compresion/huffman.c), [Compresion/huffman.h](Compresion/huffman.h)
    - Interfaces: [`writeHuffman`](Compresion/huffman.c), [`readHuffman`](Compresion/huffman.c)
  - RLE:
    - [Compresion/rle.c](Compresion/rle.c), [Compresion/rle.h](Compresion/rle.h)
    - Interfaces: [`writeRLE`](Compresion/rle.c), [`readRLE`](Compresion/rle.c)
  - LZW:
    - [Compresion/lzw.c](Compresion/lzw.c), [Compresion/lzw.h](Compresion/lzw.h)
    - Interfaces: [`writeLZW`](Compresion/lzw.c), [`readLZW`](Compresion/lzw.c)

- Encriptación (módulo Encription/):
  - Vigenère (operando sobre bytes):
    - [Encription/vigenere.c](Encription/vigenere.c), [Encription/vigenere.h](Encription/vigenere.h)
    - Interfaces: [`vigenere_encrypt_file`](Encription/vigenere.c), [`vigenere_decrypt_file`](Encription/vigenere.c)
  - AES-256-ECB (PKCS7):
    - [Encription/aes.c](Encription/aes.c), [Encription/aes.h](Encription/aes.h)
    - Interfaces: [`aes_encrypt_file`](Encription/aes.c), [`aes_decrypt_file`](Encription/aes.c)

I/O y utilidades POSIX
- Lectura/escritura robusta y helpers:
  - [posix_utils.c](posix_utils.c), [posix_utils.h](posix_utils.h)
  - Funciones clave: [`posix_read_full`](posix_utils.c), [`posix_write_full`](posix_utils.c)
- Funciones auxiliares comunes:
  - [common.h](common.h) — definición de `FileMetadata` y utilidades como [`get_basename`](common.h) y [`get_extension`](common.h)

Formato de archivo (metadatos)
- Todos los compresores y los encriptadores almacenan un encabezado común al inicio de los archivos: la estructura [`FileMetadata`](common.h) con `magic`, `originalSize`, `originalName` y `flags`. Esto permite identificación y restauración del nombre original al descomprimir/desencriptar.

Carpeta de pruebas
- Archivos de prueba disponibles en:
  - [File_Manager/testing/prueba.txt](File_Manager/testing/prueba.txt)
  - [File_Manager/testing/fortnite/test.txt](File_Manager/testing/fortnite/test.txt)
  - [File_Manager/testing/fortnite/minecraft/enough.txt](File_Manager/testing/fortnite/minecraft/enough.txt)

Notas importantes / Consideraciones
- AES implementa AES-256 en modo ECB con relleno PKCS7 tal como especificado en [Encription/aes.h](Encription/aes.h) / [Encription/aes.c](Encription/aes.c).
- Operaciones combinadas soportadas: -ce (comprimir → encriptar) y -ud (desencriptar → descomprimir). El control de combinaciones está en [main.c](main.c) y se ejecuta por medio de [`initOperation`](OperationsFileManager/multiFeature.c).
- Para procesamiento recursivo y paralelo, revisar la lógica en [OperationsFileManager/multiFeature.c](OperationsFileManager/multiFeature.c) (creación de hilos, pool y sincronización implícita por archivos temporales).

Referencias rápidas (archivos y símbolos citados)
- [main.c](main.c)
- [Makefile](Makefile)
- Compresión:
  - [`writeHuffman`](Compresion/huffman.c) / [`readHuffman`](Compresion/huffman.c) — [Compresion/huffman.c](Compresion/huffman.c), [Compresion/huffman.h](Compresion/huffman.h)
  - [`writeRLE`](Compresion/rle.c) / [`readRLE`](Compresion/rle.c) — [Compresion/rle.c](Compresion/rle.c), [Compresion/rle.h](Compresion/rle.h)
  - [`writeLZW`](Compresion/lzw.c) / [`readLZW`](Compresion/lzw.c) — [Compresion/lzw.c](Compresion/lzw.c), [Compresion/lzw.h](Compresion/lzw.h)
- Encriptación:
  - [`vigenere_encrypt_file`](Encription/vigenere.c) / [`vigenere_decrypt_file`](Encription/vigenere.c) — [Encription/vigenere.c](Encription/vigenere.c), [Encription/vigenere.h](Encription/vigenere.h)
  - [`aes_encrypt_file`](Encription/aes.c) / [`aes_decrypt_file`](Encription/aes.c) — [Encription/aes.c](Encription/aes.c), [Encription/aes.h](Encription/aes.h)
- Gestión de archivos / multihilo:
  - [`initOperation`](OperationsFileManager/multiFeature.c), [`operationOneFile`](OperationsFileManager/multiFeature.c) — [OperationsFileManager/multiFeature.c](OperationsFileManager/multiFeature.c), [OperationsFileManager/multiFeature.h](OperationsFileManager/multiFeature.h)
  - [`ThreadArgs`](OperationsFileManager/multiFeature.h)
- Utilidades POSIX y comunes:
  - [`posix_read_full`](posix_utils.c), [`posix_write_full`](posix_utils.c) — [posix_utils.c](posix_utils.c), [posix_utils.h](posix_utils.h)
  - [`FileMetadata`](common.h), [`get_basename`](common.h), [`get_extension`](common.h) — [common.h](common.h)
