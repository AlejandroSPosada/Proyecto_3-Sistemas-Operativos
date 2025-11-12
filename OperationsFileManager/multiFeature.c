#include "multiFeature.h"
#include <fcntl.h>
#include <unistd.h>

// forward declaration
static int read_original_name_from_compressed(const char* compressed_path, char* out_name, size_t out_size);


void* operationOneFile(void* arg) {
    struct ThreadArgs* args = (struct ThreadArgs*)arg;
    bool op_c = args->op_c;
    bool op_d = args->op_d;
    bool op_e = args->op_e; 
    bool op_u = args->op_u;
    const char* compAlg = args->compAlg;
    const char* encAlg  = args->encAlg;
    const char* inPath = args->inPath;
    const char* outPath = args->outPath;
    const char* key = args->key;

    // Ejecutar operación individual o combinación
    if (op_c && op_e) {
        // Combinación -ce: comprimir primero, luego encriptar
        if (!key) { fprintf(stderr, "-k [clave] es obligatorio para -ce\n"); return NULL; }
        
        const char* compressedFile;
        if (strcmp(compAlg, "huffman") == 0) {
            writeHuffman((char*)inPath);
            compressedFile = "File_Manager/output.bin";
        } else if (strcmp(compAlg, "rle") == 0) {
            writeRLE((char*)inPath);
            compressedFile = "File_Manager/output.rle";
        } else if (strcmp(compAlg, "lzw") == 0) {
            writeLZW((char*)inPath);
            compressedFile = "File_Manager/output.lzw";
        } else {
            fprintf(stderr, "Algoritmo desconocido: %s\n", compAlg);
            return NULL;
        }
        
        if (!file_exists(compressedFile)) {
            fprintf(stderr, "Error en compresión: %s\n", compressedFile);
            return NULL;
        void* operationOneFile(void* arg) {
            struct ThreadArgs* args = (struct ThreadArgs*)arg;
            bool op_c = args->op_c;
            bool op_d = args->op_d;
            bool op_e = args->op_e; 
            bool op_u = args->op_u;
            const char* compAlg = args->compAlg;
            const char* encAlg  = args->encAlg;
            const char* inPath = args->inPath;
            const char* outPath = args->outPath;
            const char* key = args->key;

            unsigned long tid = (unsigned long)pthread_self();
            if (!inPath) inPath = "(null)";
            printf("[Thread %lu] START processing: %s\n", tid, inPath);

            // Ejecutar operación individual o combinación
            if (op_c && op_e) {
                // Combinación -ce: comprimir primero, luego encriptar
                if (!key) { fprintf(stderr, "-k [clave] es obligatorio para -ce\n"); goto finish; }
        
                const char* compressedFile;
                if (strcmp(compAlg, "huffman") == 0) {
                    writeHuffman((char*)inPath);
                    compressedFile = "File_Manager/output.bin";
                } else if (strcmp(compAlg, "rle") == 0) {
                    writeRLE((char*)inPath);
                    compressedFile = "File_Manager/output.rle";
                } else if (strcmp(compAlg, "lzw") == 0) {
                    writeLZW((char*)inPath);
                    compressedFile = "File_Manager/output.lzw";
                } else {
                    fprintf(stderr, "Algoritmo desconocido: %s\n", compAlg);
                    goto finish;
                }
        
                if (!file_exists(compressedFile)) {
                    fprintf(stderr, "Error en compresión: %s\n", compressedFile);
                    goto finish;
                }
        
                // Ahora encriptar el archivo comprimido
                char encryptedFile[512];
                if (outPath) {
                    // if outPath looks like a path (contains '/'), use it as-is; otherwise treat as basename under File_Manager
                    if (strchr(outPath, '/') != NULL) {
                        snprintf(encryptedFile, sizeof(encryptedFile), "%s", outPath);
                    } else {
                        const char* base = get_basename(outPath);
                        snprintf(encryptedFile, sizeof(encryptedFile), "File_Manager/%s", base);
                    }
                } else {
                    snprintf(encryptedFile, sizeof(encryptedFile), "File_Manager/output.enc");
                }
        
                // Encriptar según el algoritmo seleccionado
                int encResult;
                if (strcmp(encAlg, "vigenere") == 0) {
                    encResult = vigenere_encrypt_file(compressedFile, encryptedFile, key);
                } else if (strcmp(encAlg, "aes") == 0) {
                    encResult = aes_encrypt_file(compressedFile, encryptedFile, key);
                } else {
                    fprintf(stderr, "Algoritmo de encriptación desconocido: %s\n", encAlg);
                    goto finish;
                }
        
                if (encResult != 0) {
                    fprintf(stderr, "Error encriptando archivo comprimido\n");
                    goto finish;
                }
        
                printf("Archivo comprimido y encriptado guardado en: %s\n", encryptedFile);
                goto finish;
            }
    
            if (op_u && op_d) {
                // Combinación -ud: desencriptar primero, luego descomprimir (inverso de -ce)
                if (!key) { fprintf(stderr, "-k [clave] es obligatorio para -ud\n"); goto finish; }
                if (!file_exists(inPath)) {
                    fprintf(stderr, "Entrada no encontrada: %s\n", inPath);
                    goto finish;
                }
        
                // Paso 1: Desencriptar
                const char* decryptedFile = "File_Manager/temp_decrypted.tmp";
        
                int decResult;
                if (strcmp(encAlg, "vigenere") == 0) {
                    decResult = vigenere_decrypt_file(inPath, decryptedFile, key);
                } else if (strcmp(encAlg, "aes") == 0) {
                    decResult = aes_decrypt_file(inPath, decryptedFile, key);
                } else {
                    fprintf(stderr, "Algoritmo de encriptación desconocido: %s\n", encAlg);
                    goto finish;
                }
        
                if (decResult != 0) {
                    fprintf(stderr, "Error desencriptando archivo\n");
                    goto finish;
                }
        
                if (!file_exists(decryptedFile)) {
                    fprintf(stderr, "Error en desencriptación: %s\n", decryptedFile);
                    goto finish;
                }
        
                // Paso 2: Descomprimir el archivo desencriptado
                int result;
                if (strcmp(compAlg, "huffman") == 0) {
                    result = readHuffman((char*)decryptedFile);
                } else if (strcmp(compAlg, "rle") == 0) {
                    result = readRLE((char*)decryptedFile);
                } else if (strcmp(compAlg, "lzw") == 0) {
                    result = readLZW((char*)decryptedFile);
                } else {
                    fprintf(stderr, "Algoritmo desconocido: %s\n", compAlg);
                    remove(decryptedFile);
                    goto finish;
                }
        
                // Limpiar archivo temporal
                remove(decryptedFile);
        
                if (result != 0) {
                    fprintf(stderr, "Fallo al descomprimir\n");
                    goto finish;
                }
        
                const char* produced = "File_Manager/output.txt";
                if (!file_exists(produced)) {
                    fprintf(stderr, "No se encontró salida de descompresión: %s\n", produced);
                    goto finish;
                }
        
                if (outPath) {
                    char dest[512];
                    if (strchr(outPath, '/') != NULL) {
                        snprintf(dest, sizeof(dest), "%s", outPath);
                    } else {
                        const char* base = get_basename(outPath);
                        snprintf(dest, sizeof(dest), "File_Manager/%s", base);
                    }
                    if (strcmp(dest, produced) != 0) {
                        if (move_file(produced, dest) != 0) goto finish;
                        printf("Archivo desencriptado y descomprimido guardado en: %s\n", dest);
                    } else {
                        printf("Archivo desencriptado y descomprimido guardado en: %s\n", produced);
                    }
                } else {
                    printf("Archivo desencriptado y descomprimido guardado en: %s\n", produced);
                }
                goto finish;
            }
    
            if (op_c) {
                // Compression: entrada -> File_Manager/output.[ext]
                const char* produced;
        
                if (strcmp(compAlg, "huffman") == 0) {
                    writeHuffman((char*)inPath);
                    produced = "File_Manager/output.bin";
                } else if (strcmp(compAlg, "rle") == 0) {
                    writeRLE((char*)inPath);
                    produced = "File_Manager/output.rle";
                } else if (strcmp(compAlg, "lzw") == 0) {
                    writeLZW((char*)inPath);
                    produced = "File_Manager/output.lzw";
                } else {
                    fprintf(stderr, "Algoritmo desconocido: %s\n", compAlg);
                    goto finish;
                }
        
                // Siempre dejar en File_Manager: usar nombre dado por -o (si existe) pero dentro de File_Manager
                if (!file_exists(produced)) {
                    fprintf(stderr, "No se encontró salida de compresión: %s\n", produced);
                    goto finish;
                }
                if (outPath) {
                    char dest[512];
                    if (strchr(outPath, '/') != NULL) {
                        snprintf(dest, sizeof(dest), "%s", outPath);
                    } else {
                        const char* base = get_basename(outPath);
                        snprintf(dest, sizeof(dest), "File_Manager/%s", base);
                    }
                    if (strcmp(dest, produced) != 0) {
                        if (move_file(produced, dest) != 0) goto finish;
                        printf("Archivo comprimido guardado en: %s\n", dest);
                    } else {
                        printf("Archivo comprimido guardado en: %s\n", produced);
                    }
                } else {
                    printf("Archivo comprimido guardado en: %s\n", produced);
                }
                goto finish;
            }

            if (op_d) {
                // Decompression: entrada -> File_Manager/output.txt
                if (!file_exists(inPath)) {
                    fprintf(stderr, "Entrada no encontrada: %s\n", inPath);
                    goto finish;
                }
        
                int result;
                if (strcmp(compAlg, "huffman") == 0) {
                    result = readHuffman((char*)inPath);
                } else if (strcmp(compAlg, "rle") == 0) {
                    result = readRLE((char*)inPath);
                } else if (strcmp(compAlg, "lzw") == 0) {
                    result = readLZW((char*)inPath);
                } else {
                    fprintf(stderr, "Algoritmo desconocido: %s\n", compAlg);
                    goto finish;
                }
        
                if (result != 0) {
                    fprintf(stderr, "Fallo al descomprimir %s\n", inPath);
                    goto finish;
                }
                const char* produced = "File_Manager/output.txt";
                if (!file_exists(produced)) {
                    fprintf(stderr, "No se encontró salida de descompresión: %s\n", produced);
                    goto finish;
                }

                // Try to read original filename from compressed input and restore it
                char original_name[512];
                int have_original = read_original_name_from_compressed(inPath, original_name, sizeof(original_name)) == 0;

                if (have_original) {
                    // determine output folder: if outPath contains '/', use its directory; otherwise default to File_Manager
                    char folder[1024];
                    if (outPath && strchr(outPath, '/') != NULL) {
                        const char* last = strrchr(outPath, '/');
                        size_t len = last - outPath;
                        if (len >= sizeof(folder)) len = sizeof(folder) - 1;
                        strncpy(folder, outPath, len);
                        folder[len] = '\0';
                    } else {
                        strncpy(folder, "File_Manager", sizeof(folder)); folder[sizeof(folder)-1] = '\0';
                    }

                    char dest[1536];
                    snprintf(dest, sizeof(dest), "%s/%s", folder, get_basename(original_name));
                    if (move_file(produced, dest) != 0) goto finish;
                    printf("Archivo descomprimido guardado en: %s\n", dest);
                } else {
                    if (outPath) {
                        char dest[512];
                        if (strchr(outPath, '/') != NULL) {
                            snprintf(dest, sizeof(dest), "%s", outPath);
                        } else {
                            const char* base = get_basename(outPath);
                            snprintf(dest, sizeof(dest), "File_Manager/%s", base);
                        }
                        if (strcmp(dest, produced) != 0) {
                            if (move_file(produced, dest) != 0) goto finish;
                            printf("Archivo descomprimido guardado en: %s\n", dest);
                        } else {
                            printf("Archivo descomprimido guardado en: %s\n", produced);
                        }
                    } else {
                        printf("Archivo descomprimido guardado en: %s\n", produced);
                    }
                }
                goto finish;
            }

            if (op_e) {
                // Encriptar archivo (funciona con cualquier tipo de archivo: texto o binario)
                if (!file_exists(inPath)) {
                    fprintf(stderr, "Entrada no encontrada: %s\n", inPath);
                    goto finish;
                }
        
                char dest[512];
                if (outPath) {
                    if (strchr(outPath, '/') != NULL) {
                        snprintf(dest, sizeof(dest), "%s", outPath);
                    } else {
                        const char* base = get_basename(outPath);
                        snprintf(dest, sizeof(dest), "File_Manager/%s", base);
                    }
                } else {
                    snprintf(dest, sizeof(dest), "File_Manager/output.enc");
                }
        
                int encResult;
                if (strcmp(encAlg, "vigenere") == 0) {
                    encResult = vigenere_encrypt_file(inPath, dest, key);
                } else if (strcmp(encAlg, "aes") == 0) {
                    encResult = aes_encrypt_file(inPath, dest, key);
                } else {
                    fprintf(stderr, "Algoritmo de encriptación desconocido: %s\n", encAlg);
                    goto finish;
                }
        
                if (encResult != 0) {
                    fprintf(stderr, "Error encriptando archivo\n");
                    goto finish;
                }
        
                printf("Archivo encriptado guardado en: %s\n", dest);
                goto finish;
            }

            if (op_u) {
                // Desencriptar archivo
                if (!file_exists(inPath)) {
                    fprintf(stderr, "Entrada no encontrada: %s\n", inPath);
                    goto finish;
                }
        
                char dest[512];
                if (outPath) {
                    if (strchr(outPath, '/') != NULL) {
                        snprintf(dest, sizeof(dest), "%s", outPath);
                    } else {
                        const char* base = get_basename(outPath);
                        snprintf(dest, sizeof(dest), "File_Manager/%s", base);
                    }
                } else {
                    snprintf(dest, sizeof(dest), "File_Manager/output.txt");
                }
        
                int decResult;
                if (strcmp(encAlg, "vigenere") == 0) {
                    decResult = vigenere_decrypt_file(inPath, dest, key);
                } else if (strcmp(encAlg, "aes") == 0) {
                    decResult = aes_decrypt_file(inPath, dest, key);
                } else {
                    fprintf(stderr, "Algoritmo de encriptación desconocido: %s\n", encAlg);
                    goto finish;
                }
        
                if (decResult != 0) {
                    fprintf(stderr, "Error desencriptando archivo\n");
                    goto finish;
                }
        
                printf("Archivo desencriptado guardado en: %s\n", dest);
                goto finish;
            }

        finish:
            printf("[Thread %lu] END processing: %s\n", tid, inPath);
            return NULL;
        }

            if (file_exists("File_Manager/output.txt")) {
                if (move_file("File_Manager/output.txt", dest_final) != 0) return NULL;
            } else if (file_exists("File_Manager/output.bin")) {
                if (move_file("File_Manager/output.bin", dest_final) != 0) return NULL;
            } else if (file_exists("File_Manager/output.lzw")) {
                if (move_file("File_Manager/output.lzw", dest_final) != 0) return NULL;
            } else {
                // fallback: move decrypted file itself
                if (move_file(dest, dest_final) != 0) return NULL;
            }
            // Remove the intermediate decrypted/compressed file if it still exists
            if (file_exists(dest)) unlink(dest);

            printf("Archivo desencriptado y descomprimido guardado en: %s\n", dest_final);
            return NULL;
        }

        printf("Archivo desencriptado guardado en: %s\n", dest);
        return NULL;
    }

    return NULL;
}

// Read original filename stored in compressed file's metadata (common FileMetadata at file start)
static int read_original_name_from_compressed(const char* compressed_path, char* out_name, size_t out_size) {
    if (!compressed_path || !out_name) return -1;
    int fd = open(compressed_path, O_RDONLY);
    if (fd < 0) return -1;
    FileMetadata meta;
    ssize_t r = read(fd, &meta, sizeof(meta));
    close(fd);
    if (r != sizeof(meta)) return -1;
    if (meta.magic != METADATA_MAGIC) return -1;
    // copy basename only
    const char* orig = get_basename(meta.originalName);
    strncpy(out_name, orig, out_size);
    out_name[out_size-1] = '\0';
    return 0;
}


static bool file_exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

static int move_file(const char* src, const char* dst_folder) {
    struct stat st;
    if (stat(dst_folder, &st) == 0 && S_ISDIR(st.st_mode)) {
        // dst_folder is a directory, append filename
        const char* base = get_basename(src);
        char full_dst[1024];
        snprintf(full_dst, sizeof(full_dst), "%s/%s", dst_folder, base);
        if (rename(src, full_dst) == 0) return 0;
        perror("rename");
        return -1;
    } else {
        // dst_folder is treated as file
        if (rename(src, dst_folder) == 0) return 0;
        perror("rename");
        return -1;
    }
}

// Ensure a directory exists, creating parents as needed (mkdir -p behavior)
static int mkdir_p(const char* path) {
    if (!path || path[0] == '\0') return -1;
    char tmp[1024];
    strncpy(tmp, path, sizeof(tmp)); tmp[sizeof(tmp)-1] = '\0';
    size_t len = strlen(tmp);
    if (len == 0) return -1;
    if (tmp[len-1] == '/') tmp[len-1] = '\0';
    for (char* p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0755) != 0) {
                if (errno != EEXIST) { *p = '/'; return -1; }
            }
            *p = '/';
        }
    }
    if (mkdir(tmp, 0755) != 0) {
        if (errno != EEXIST) return -1;
    }
    return 0;
}

// Recursive processing: mirror directory tree under outFolder and process each regular file
static void process_path_recursive(const char* basePath, const char* curPath, ThreadArgs myargs, const char* outFolder) {
    struct stat st;
    if (stat(curPath, &st) != 0) return;

    if (S_ISDIR(st.st_mode)) {
        DIR* dir = opendir(curPath);
        if (!dir) return;
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
            char next[1024];
            snprintf(next, sizeof(next), "%s/%s", curPath, entry->d_name);
            process_path_recursive(basePath, next, myargs, outFolder);
        }
        closedir(dir);
        return;
    }

    if (!S_ISREG(st.st_mode)) return;

    // compute relative path from basePath
    size_t base_len = strlen(basePath);
    const char* rel = curPath + base_len;
    if (*rel == '/') rel++;
    char rel_copy[1024];
    strncpy(rel_copy, rel, sizeof(rel_copy)); rel_copy[sizeof(rel_copy)-1] = '\0';

    // determine target directory under outFolder and ensure it exists
    char target_dir[1024];
    const char* last_sl = strrchr(rel_copy, '/');
    if (last_sl) {
        size_t dirlen = last_sl - rel_copy;
        snprintf(target_dir, sizeof(target_dir), "%s/%.*s", outFolder, (int)dirlen, rel_copy);
    } else {
        snprintf(target_dir, sizeof(target_dir), "%s", outFolder);
    }
    if (mkdir_p(target_dir) != 0) {
        fprintf(stderr, "No se pudo crear carpeta destino: %s\n", target_dir);
        return;
    }

    // prepare ThreadArgs for this file
    ThreadArgs ta = myargs;
    ta.inPath = strdup(curPath);

    const char* compAlg = myargs.compAlg;
    const char* ext = "";
    if (strcmp(compAlg, "huffman") == 0) ext = "bin";
    else if (strcmp(compAlg, "rle") == 0) ext = "rle";
    else if (strcmp(compAlg, "lzw") == 0) ext = "lzw";

    const char* base = get_basename(rel_copy);
    char name_noext[512];
    strncpy(name_noext, base, sizeof(name_noext)); name_noext[sizeof(name_noext)-1] = '\0';
    char* dot = strrchr(name_noext, '.'); if (dot) *dot = '\0';

    char out_full[1536];
    if (myargs.op_e) {
        // If encrypting a directory, prefer to compress then encrypt so decryption can restore original.
        // Use provided compAlg or default to rle.
        ta.op_e = true;
        ta.op_c = true;
        if (!ta.compAlg || ta.compAlg[0] == '\0') {
            ta.compAlg = "rle";
        }
        // encrypted/compressed files use .bin extension
        snprintf(out_full, sizeof(out_full), "%s/%s.bin", target_dir, name_noext);
    } else if (myargs.op_c) {
        if (ext[0] != '\0')
            snprintf(out_full, sizeof(out_full), "%s/%s.%s", target_dir, name_noext, ext);
        else
            snprintf(out_full, sizeof(out_full), "%s/%s", target_dir, name_noext);
    } else if (myargs.op_d) {
        // for decompression, provide an outPath that contains the target directory so operationOneFile
        // will place the decompressed file under that directory using original metadata
        snprintf(out_full, sizeof(out_full), "%s/%s", target_dir, base);
    } else {
        // default: set out to target dir + original basename
        snprintf(out_full, sizeof(out_full), "%s/%s", target_dir, base);
    }

    ta.outPath = strdup(out_full);

    // If we're processing a directory for decompression, detect algorithm per-file by extension
    if (ta.op_d) {
        const char* fileext = get_extension(rel_copy);
        if (strcmp(fileext, "rle") == 0) {
            ta.compAlg = "rle";
        } else if (strcmp(fileext, "lzw") == 0) {
            ta.compAlg = "lzw";
        } else if (strcmp(fileext, "bin") == 0 || strcmp(fileext, "huff") == 0) {
            ta.compAlg = "huffman";
        } else {
            ta.compAlg = myargs.compAlg;
        }
    }

    // Process synchronously (safe with existing compressors that write to fixed temp files)
    operationOneFile(&ta);

    free((char*)ta.inPath);
    free((char*)ta.outPath);
}

/*
Here we start with the interaction with File_Manager. 
First let's realize if we are dealing with a file or a folder. 
If only a file, then only a thread. If a folder, then let's use several threads.
*/
void initOperation(ThreadArgs myargs) {
    const char *path = myargs.inPath;
    struct stat st;

    if (stat(path, &st) != 0) {
        perror("Error accessing path");
        return;
    }

    if (S_ISREG(st.st_mode)) {
        printf("It is a file.\n");
        pthread_t thread1;
        pthread_create(&thread1, NULL, operationOneFile, &myargs);
        pthread_join(thread1, NULL);
        return;
    } else if (S_ISDIR(st.st_mode)) {
        printf("It is a folder.\n");

        // Create output folder
        char outFolder[1024];
        snprintf(outFolder, sizeof(outFolder), "%s_processed", path);
        mkdir_p(outFolder);

        // Process recursively and mirror tree
        process_path_recursive(path, path, myargs, outFolder);

        printf("Folder processing done.\n");
    } else {
        printf("It is neither a regular file nor a directory.\n");
    }
}
