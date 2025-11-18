#include "multiFeature.h"
#include <fcntl.h>
#include <unistd.h>

// Declaraciones anticipadas
static int read_original_name_from_compressed(const char* compressed_path, char* out_name, size_t out_size);
static double get_elapsed_time(struct timespec start_time);


void* operationOneFile(void* arg) {
    struct ThreadArgs* args = (struct ThreadArgs*)arg;
    
    // Capturar tiempo de inicio
    clock_gettime(CLOCK_MONOTONIC, &args->start_time);
    
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
        
        // Generar nombre único temporal para el archivo comprimido usando thread ID
        pthread_t tid = pthread_self();
        char compressedFile[512];
        
        if (strcmp(compAlg, "huffman") == 0) {
            snprintf(compressedFile, sizeof(compressedFile), "File_Manager/temp_%lu.bin", (unsigned long)tid);
            writeHuffman((char*)inPath, compressedFile);
        } else if (strcmp(compAlg, "rle") == 0) {
            snprintf(compressedFile, sizeof(compressedFile), "File_Manager/temp_%lu.rle", (unsigned long)tid);
            writeRLE((char*)inPath, compressedFile);
        } else if (strcmp(compAlg, "lzw") == 0) {
            snprintf(compressedFile, sizeof(compressedFile), "File_Manager/temp_%lu.lzw", (unsigned long)tid);
            writeLZW((char*)inPath, compressedFile);
        } else {
            fprintf(stderr, "Algoritmo desconocido: %s\n", compAlg);
            return NULL;
        }
        
        if (!file_exists(compressedFile)) {
            fprintf(stderr, "Error en compresión: %s\n", compressedFile);
            return NULL;
        }
        
        // Ahora encriptar el archivo comprimido
        char encryptedFile[512];
        if (outPath) {
            // Si outPath parece una ruta (contiene '/'), usarla tal cual; de lo contrario tratarla como nombre base en File_Manager
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
            return NULL;
        }
        
        if (encResult != 0) {
            fprintf(stderr, "Error encriptando archivo comprimido\n");
            return NULL;
        }
        
        // Limpiar archivo temporal
        remove(compressedFile);
        
        double elapsed = get_elapsed_time(args->start_time);
        printf("[Hilo %d] %s (%.1fs)\n", args->thread_index, args->thread_file_name, elapsed);
        return NULL;
    }
    
    if (op_u && op_d) {
        // Combinación -ud: desencriptar primero, luego descomprimir (inverso de -ce)
        if (!key) { fprintf(stderr, "-k [clave] es obligatorio para -ud\n"); return NULL; }
        if (!file_exists(inPath)) {
            fprintf(stderr, "Entrada no encontrada: %s\n", inPath);
            return NULL;
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
            return NULL;
        }
        
        if (decResult != 0) {
            fprintf(stderr, "Error desencriptando archivo\n");
            return NULL;
        }
        
        if (!file_exists(decryptedFile)) {
            fprintf(stderr, "Error en desencriptación: %s\n", decryptedFile);
            return NULL;
        }
        
        // Paso 2: Descomprimir el archivo desencriptado usando nombre temporal único
        pthread_t tid = pthread_self();
        char decompressedFile[512];
        snprintf(decompressedFile, sizeof(decompressedFile), "File_Manager/temp_%lu_decomp.txt", (unsigned long)tid);
        
        int result;
        if (strcmp(compAlg, "huffman") == 0) {
            result = readHuffman((char*)decryptedFile, decompressedFile);
        } else if (strcmp(compAlg, "rle") == 0) {
            result = readRLE((char*)decryptedFile, decompressedFile);
        } else if (strcmp(compAlg, "lzw") == 0) {
            result = readLZW((char*)decryptedFile, decompressedFile);
        } else {
            fprintf(stderr, "Algoritmo desconocido: %s\n", compAlg);
            remove(decryptedFile);
            return NULL;
        }
        
        // Limpiar archivo temporal
        remove(decryptedFile);
        
        if (result != 0) {
            fprintf(stderr, "Fallo al descomprimir\n");
            remove(decompressedFile);
            return NULL;
        }
        
        if (!file_exists(decompressedFile)) {
            fprintf(stderr, "No se encontró salida de descompresión: %s\n", decompressedFile);
            return NULL;
        }
        
        if (outPath) {
            char dest[512];
            if (strchr(outPath, '/') != NULL) {
                snprintf(dest, sizeof(dest), "%s", outPath);
            } else {
                const char* base = get_basename(outPath);
                snprintf(dest, sizeof(dest), "File_Manager/%s", base);
            }
            if (strcmp(dest, decompressedFile) != 0) {
                if (move_file(decompressedFile, dest) != 0) return NULL;
            }
        }
        
        double elapsed = get_elapsed_time(args->start_time);
        printf("[Hilo %d] %s (%.1fs)\n", args->thread_index, args->thread_file_name, elapsed);
        return NULL;
    }
    
    if (op_c) {
        // Compresión: entrada -> archivo de salida final directamente
        
        // Determinar destino final
        char final_dest[512];
        if (outPath) {
            if (strchr(outPath, '/') != NULL) {
                snprintf(final_dest, sizeof(final_dest), "%s", outPath);
            } else {
                const char* base = get_basename(outPath);
                snprintf(final_dest, sizeof(final_dest), "File_Manager/%s", base);
            }
        } else {
            // Sin outPath especificado, generar nombre basado en la entrada
            const char* base = get_basename(inPath);
            char name_noext[512];
            strncpy(name_noext, base, sizeof(name_noext)); 
            name_noext[sizeof(name_noext)-1] = '\0';
            char* dot = strrchr(name_noext, '.');
            if (dot) *dot = '\0';
            
            const char* ext = "";
            if (strcmp(compAlg, "huffman") == 0) ext = "bin";
            else if (strcmp(compAlg, "rle") == 0) ext = "rle";
            else if (strcmp(compAlg, "lzw") == 0) ext = "lzw";
            
            snprintf(final_dest, sizeof(final_dest), "File_Manager/%s.%s", name_noext, ext);
        }
        
        // Comprimir directamente al destino final (sin mutex, sin archivos temporales compartidos)
        if (strcmp(compAlg, "huffman") == 0) {
            writeHuffman((char*)inPath, final_dest);
        } else if (strcmp(compAlg, "rle") == 0) {
            writeRLE((char*)inPath, final_dest);
        } else if (strcmp(compAlg, "lzw") == 0) {
            writeLZW((char*)inPath, final_dest);
        } else {
            fprintf(stderr, "Algoritmo desconocido: %s\n", compAlg);
            return NULL;
        }
        
        if (!file_exists(final_dest)) {
            fprintf(stderr, "[Thread] No se encontró salida de compresión: %s\n", final_dest);
            return NULL;
        }
        
        double elapsed = get_elapsed_time(args->start_time);
        printf("[Hilo %d] %s (%.1fs)\n", args->thread_index, args->thread_file_name, elapsed);
        return NULL;
    }

    if (op_d) {
        // Descompresión: entrada -> destino final directamente
        if (!file_exists(inPath)) {
            fprintf(stderr, "Entrada no encontrada: %s\n", inPath);
            return NULL;
        }
        
        // Determinar destino final
        char original_name[512];
        int have_original = read_original_name_from_compressed(inPath, original_name, sizeof(original_name)) == 0;
        char final_dest[1536];
        
        if (have_original) {
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
            snprintf(final_dest, sizeof(final_dest), "%s/%s", folder, get_basename(original_name));
        } else {
            if (outPath) {
                if (strchr(outPath, '/') != NULL) {
                    snprintf(final_dest, sizeof(final_dest), "%s", outPath);
                } else {
                    const char* base = get_basename(outPath);
                    snprintf(final_dest, sizeof(final_dest), "File_Manager/%s", base);
                }
            } else {
                snprintf(final_dest, sizeof(final_dest), "File_Manager/output.txt");
            }
        }
        
        // Descomprimir directamente al destino final (sin mutex)
        int result;
        if (strcmp(compAlg, "huffman") == 0) {
            result = readHuffman((char*)inPath, final_dest);
        } else if (strcmp(compAlg, "rle") == 0) {
            result = readRLE((char*)inPath, final_dest);
        } else if (strcmp(compAlg, "lzw") == 0) {
            result = readLZW((char*)inPath, final_dest);
        } else {
            fprintf(stderr, "Algoritmo desconocido: %s\n", compAlg);
            return NULL;
        }
        
        if (result != 0) {
            fprintf(stderr, "Fallo al descomprimir %s\n", inPath);
            return NULL;
        }
        
        if (!file_exists(final_dest)) {
            fprintf(stderr, "No se encontró salida de descompresión: %s\n", final_dest);
            return NULL;
        }
        
        double elapsed = get_elapsed_time(args->start_time);
        printf("[Hilo %d] %s (%.1fs)\n", args->thread_index, args->thread_file_name, elapsed);
        return NULL;
    }

    if (op_e) {
        // Encriptar archivo (funciona con cualquier tipo de archivo: texto o binario)
        if (!file_exists(inPath)) {
            fprintf(stderr, "Entrada no encontrada: %s\n", inPath);
            return NULL;
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
            return NULL;
        }
        
        if (encResult != 0) {
            fprintf(stderr, "Error encriptando archivo\n");
            return NULL;
        }
        
        double elapsed = get_elapsed_time(args->start_time);
        printf("[Hilo %d] %s (%.1fs)\n", args->thread_index, args->thread_file_name, elapsed);
        return NULL;
    }

    if (op_u) {
        // Desencriptar archivo
        if (!file_exists(inPath)) {
            fprintf(stderr, "Entrada no encontrada: %s\n", inPath);
            return NULL;
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
            return NULL;
        }
        
        if (decResult != 0) {
            fprintf(stderr, "Error desencriptando archivo\n");
            return NULL;
        }
        
        // Después de desencriptar, verificar si el archivo desencriptado contiene metadatos de compresión
        char original_name[512];
        int is_compressed = read_original_name_from_compressed(dest, original_name, sizeof(original_name)) == 0;

        if (is_compressed) {
            // Intentar determinar el algoritmo de compresión desde el nombre del archivo o probando todos los descompresores
            // Generar archivo temporal único para descompresión
            pthread_t tid = pthread_self();
            char decompressed_temp[512];
            snprintf(decompressed_temp, sizeof(decompressed_temp), "File_Manager/temp_%lu_decomp.txt", (unsigned long)tid);
            
            const char* comp_ext = get_extension(inPath);
            int decomp_result = 1;
            if (comp_ext && strcmp(comp_ext, "rle") == 0) {
                decomp_result = readRLE((char*)dest, decompressed_temp);
            } else if (comp_ext && strcmp(comp_ext, "lzw") == 0) {
                decomp_result = readLZW((char*)dest, decompressed_temp);
            } else if (comp_ext && (strcmp(comp_ext, "bin") == 0 || strcmp(comp_ext, "huff") == 0)) {
                decomp_result = readHuffman((char*)dest, decompressed_temp);
            } else {
                // Probar descompresores hasta que uno funcione
                if (readRLE((char*)dest, decompressed_temp) == 0) decomp_result = 0;
                else if (readLZW((char*)dest, decompressed_temp) == 0) decomp_result = 0;
                else if (readHuffman((char*)dest, decompressed_temp) == 0) decomp_result = 0;
            }

            if (decomp_result != 0) {
                fprintf(stderr, "Fallo al descomprimir archivo desencriptado: %s\n", dest);
                remove(decompressed_temp);
                return NULL;
            }

            // Mover salida descomprimida a ruta final usando nombre original de los metadatos
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

            char dest_final[1536];
            snprintf(dest_final, sizeof(dest_final), "%s/%s", folder, get_basename(original_name));

            if (file_exists(decompressed_temp)) {
                if (move_file(decompressed_temp, dest_final) != 0) return NULL;
            } else {
                // Alternativa: mover el archivo desencriptado en sí mismo
                if (move_file(dest, dest_final) != 0) return NULL;
            }

            // Eliminar archivo intermedio desencriptado/comprimido
            if (file_exists(dest)) unlink(dest);
        }
        
        double elapsed = get_elapsed_time(args->start_time);
        printf("[Hilo %d] %s (%.1fs)\n", args->thread_index, args->thread_file_name, elapsed);
        return NULL;

    }

    return NULL;
}

// Leer nombre de archivo original almacenado en los metadatos del archivo comprimido (FileMetadata común al inicio del archivo)
static int read_original_name_from_compressed(const char* compressed_path, char* out_name, size_t out_size) {
    if (!compressed_path || !out_name) return -1;
    int fd = open(compressed_path, O_RDONLY);
    if (fd < 0) return -1;
    FileMetadata meta;
    ssize_t r = read(fd, &meta, sizeof(meta));
    close(fd);
    if (r != sizeof(meta)) return -1;
    if (meta.magic != METADATA_MAGIC) return -1;
    // Copiar solo el nombre base
    const char* orig = get_basename(meta.originalName);
    strncpy(out_name, orig, out_size);
    out_name[out_size-1] = '\0';
    return 0;
}

// Función auxiliar para calcular el tiempo transcurrido en segundos
static double get_elapsed_time(struct timespec start_time) {
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    
    double elapsed = (double)(end_time.tv_sec - start_time.tv_sec) + 
                     (double)(end_time.tv_nsec - start_time.tv_nsec) / 1e9;
    return elapsed;
}

// Verificar si el archivo existe
bool file_exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

// Mover archivo de src a dst (dst puede ser directorio o ruta completa)
int move_file(const char* src, const char* dst_folder) {
    struct stat st;
    if (stat(dst_folder, &st) == 0 && S_ISDIR(st.st_mode)) {
        // dst_folder es un directorio, agregar nombre de archivo
        const char* base = get_basename(src);
        char full_dst[1024];
        snprintf(full_dst, sizeof(full_dst), "%s/%s", dst_folder, base);
        if (rename(src, full_dst) == 0) return 0;
        perror("rename");
        return -1;
    } else {
        // dst_folder se trata como archivo
        if (rename(src, dst_folder) == 0) return 0;
        perror("rename");
        return -1;
    }
}

// Estructura para mantener información de cada hilo
typedef struct {
    pthread_t thread;
    ThreadArgs* args;
} ThreadInfo;

// Estructura para gestionar arrays dinámicos de hilos a través de recursión
typedef struct {
    ThreadInfo* threads;
    int count;
    int capacity;
} ThreadPool;

// Función auxiliar para crear carpeta si no existe
void ensure_directory_exists(const char* dir_path) {
    struct stat st;
    if (stat(dir_path, &st) != 0) {
        mkdir(dir_path, 0755);
    }
}

// Función auxiliar para obtener la ruta relativa desde la carpeta base
// Por ejemplo: si base="/input", full="/input/sub/file.txt", retorna "sub/file.txt"
static void get_relative_path(const char* base, const char* full, char* rel, size_t rel_size) {
    size_t base_len = strlen(base);
    if (strncmp(full, base, base_len) == 0) {
        if (full[base_len] == '/') {
            strncpy(rel, full + base_len + 1, rel_size - 1);
        } else {
            strncpy(rel, full + base_len, rel_size - 1);
        }
    } else {
        strncpy(rel, full, rel_size - 1);
    }
    rel[rel_size - 1] = '\0';
}

// Función recursiva para procesar directorios y crear hilos para archivos
static void process_directory_recursive(const char* base_input_dir, const char* current_dir, 
                                       const char* base_output_dir, ThreadArgs myargs,
                                       ThreadPool* pool, int* global_thread_count) {
    DIR *dir = opendir(current_dir);
    if (!dir) { 
        perror("Error opening directory"); 
        return; 
    }

    struct dirent *entry;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char full_path[2048];
        snprintf(full_path, sizeof(full_path), "%s/%s", current_dir, entry->d_name);

        struct stat entry_st;
        if (stat(full_path, &entry_st) != 0)
            continue;

        // Si es un directorio: recursión + crear carpeta de salida correspondiente
        if (S_ISDIR(entry_st.st_mode)) {
            char rel_path[1024];
            get_relative_path(base_input_dir, full_path, rel_path, sizeof(rel_path));
            
            char output_subdir[2048];
            snprintf(output_subdir, sizeof(output_subdir), "%s/%s", base_output_dir, rel_path);
            ensure_directory_exists(output_subdir);
            
            printf("  → Entrando en subdirectorio: %s\n", rel_path);
            process_directory_recursive(base_input_dir, full_path, base_output_dir, myargs, pool, global_thread_count);
            continue;
        }

        // Si es un archivo regular: crear hilo para procesarlo
        if (S_ISREG(entry_st.st_mode)) {
            // Expandir arreglo si es necesario
            if (pool->count >= pool->capacity) {
                pool->capacity = (pool->capacity == 0) ? 20 : pool->capacity * 2;
                pool->threads = realloc(pool->threads, pool->capacity * sizeof(ThreadInfo));
                if (!pool->threads) {
                    perror("Error al reservar memoria para hilos");
                    closedir(dir);
                    return;
                }
            }

            // Preparar argumentos para este archivo
            ThreadArgs* ta = malloc(sizeof(ThreadArgs));
            if (!ta) {
                perror("Error al reservar memoria para ThreadArgs");
                continue;
            }
            
            *ta = myargs;
            ta->inPath = strdup(full_path);

            // Obtener ruta relativa del archivo desde el directorio base
            char rel_file_path[1024];
            get_relative_path(base_input_dir, full_path, rel_file_path, sizeof(rel_file_path));

            // Quitar extensión del nombre base para construir salida
            char name_noext[512];
            strncpy(name_noext, rel_file_path, sizeof(name_noext)); 
            name_noext[sizeof(name_noext)-1] = '\0';
            char* dot = strrchr(name_noext, '.');
            if (dot) *dot = '\0';

            char out_full[2048];
            
            // Si se encripta un directorio, comprimir primero y luego encriptar
            if (myargs.op_e) {
                ta->op_e = true;
                ta->op_c = true;
                if (!ta->compAlg || ta->compAlg[0] == '\0') {
                    ta->compAlg = "rle";
                }
                snprintf(out_full, sizeof(out_full), "%s/%s.bin", base_output_dir, name_noext);
            }
            // Si se desencripta
            else if (myargs.op_u) {
                snprintf(out_full, sizeof(out_full), "%s/%s", base_output_dir, rel_file_path);
                ta->outPath = strdup(out_full);
            } 
            else {
                // Compresión o descompresión normal
                const char* compAlg = myargs.compAlg;
                const char* ext = "";
                if (strcmp(compAlg, "huffman") == 0) ext = "bin";
                else if (strcmp(compAlg, "rle") == 0) ext = "rle";
                else if (strcmp(compAlg, "lzw") == 0) ext = "lzw";

                if (ext[0] != '\0')
                    snprintf(out_full, sizeof(out_full), "%s/%s.%s", base_output_dir, name_noext, ext);
                else
                    snprintf(out_full, sizeof(out_full), "%s/%s", base_output_dir, name_noext);

                ta->outPath = strdup(out_full);

                // Detectar algoritmo por extensión en descompresión
                if (ta->op_d) {
                    const char* fileext = get_extension(entry->d_name);
                    if (strcmp(fileext, "rle") == 0) {
                        ta->compAlg = "rle";
                    } else if (strcmp(fileext, "lzw") == 0) {
                        ta->compAlg = "lzw";
                    } else if (strcmp(fileext, "bin") == 0 || strcmp(fileext, "huff") == 0) {
                        ta->compAlg = "huffman";
                    } else {
                        ta->compAlg = myargs.compAlg;
                    }
                }
            }

            // Crear hilo para procesar este archivo EN PARALELO
            ta->thread_index = pool->count + 1;  // Asignar número de hilo
            ta->thread_file_name = strdup(rel_file_path);  // Asignar nombre de archivo
            
            pool->threads[pool->count].args = ta;
            int ret = pthread_create(&pool->threads[pool->count].thread, NULL, operationOneFile, ta);
            if (ret != 0) {
                fprintf(stderr, "Error creando hilo para %s: %s\n", entry->d_name, strerror(ret));
                free((char*)ta->inPath);
                free((char*)ta->outPath);
                free((char*)ta->thread_file_name);
                free(ta);
                continue;
            }
            
            printf("  → Hilo %d: procesando '%s' en paralelo...\n", ta->thread_index, rel_file_path);
            pool->count++;
            (*global_thread_count)++;
        }
    }
    
    closedir(dir);
}

/*
Here we start with the interaction with File_Manager. 
First let's realize if we are dealing with a file or a folder. 
If only a file, then only a thread. If a folder, then let's use several threads IN PARALLEL.
Now it recursively processes subdirectories while maintaining folder structure.
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
        printf("It is a folder - processing files RECURSIVELY in PARALLEL...\n");

        // Crear carpeta de salida
        char outFolder[1024];
        if (myargs.outPath && myargs.outPath[0] != '\0') {
            snprintf(outFolder, sizeof(outFolder), "%s", myargs.outPath);
        } else {
            snprintf(outFolder, sizeof(outFolder), "%s_processed", path);
        }
        ensure_directory_exists(outFolder);

        // Inicializar pool de hilos
        ThreadPool pool = {0};
        int global_thread_count = 0;

        // FASE 1: Recorrer recursivamente y crear hilos
        printf("\nFASE 1: Escaneando directorios y creando hilos...\n");
        process_directory_recursive(path, path, outFolder, myargs, &pool, &global_thread_count);

        // FASE 2: Esperar a que todos los hilos terminen
        printf("\nFASE 2: Esperando a que terminen %d hilos...\n", pool.count);
        for (int i = 0; i < pool.count; i++) {
            pthread_join(pool.threads[i].thread, NULL);
            
            // Liberar memoria de argumentos
            free((char*)pool.threads[i].args->inPath);
            free((char*)pool.threads[i].args->outPath);
            free((char*)pool.threads[i].args->thread_file_name);
            free(pool.threads[i].args);
        }

        // Liberar array de hilos
        free(pool.threads);
        
        printf("\nProcesamiento paralelo y recursivo completado. %d archivos procesados.\n", pool.count);
    } else {
        printf("It is neither a regular file nor a directory.\n");
    }
}
