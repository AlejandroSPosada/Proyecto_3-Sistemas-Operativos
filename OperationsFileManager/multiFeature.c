#include "multiFeature.h"
#include <fcntl.h>
#include <unistd.h>

// Mutex global para proteger operaciones de compresión/descompresión
// que usan archivos temporales compartidos
static pthread_mutex_t compression_mutex = PTHREAD_MUTEX_INITIALIZER;

// Declaración anticipada
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
        
        printf("Archivo comprimido y encriptado guardado en: %s\n", encryptedFile);
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
            return NULL;
        }
        
        // Limpiar archivo temporal
        remove(decryptedFile);
        
        if (result != 0) {
            fprintf(stderr, "Fallo al descomprimir\n");
            return NULL;
        }
        
        const char* produced = "File_Manager/output.txt";
        if (!file_exists(produced)) {
            fprintf(stderr, "No se encontró salida de descompresión: %s\n", produced);
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
            if (strcmp(dest, produced) != 0) {
                if (move_file(produced, dest) != 0) return NULL;
                printf("Archivo desencriptado y descomprimido guardado en: %s\n", dest);
            } else {
                printf("Archivo desencriptado y descomprimido guardado en: %s\n", produced);
            }
        } else {
            printf("Archivo desencriptado y descomprimido guardado en: %s\n", produced);
        }
        return NULL;
    }
    
    if (op_c) {
        // Compresión: entrada -> archivo temporal único por hilo
        const char* temp_produced;
        const char* final_dest;
        
        // Determinar destino final ANTES de comprimir
        char dest[512];
        if (outPath) {
            if (strchr(outPath, '/') != NULL) {
                snprintf(dest, sizeof(dest), "%s", outPath);
            } else {
                const char* base = get_basename(outPath);
                snprintf(dest, sizeof(dest), "File_Manager/%s", base);
            }
            final_dest = dest;
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
            
            snprintf(dest, sizeof(dest), "File_Manager/%s.%s", name_noext, ext);
            final_dest = dest;
        }
        
        // BLOQUEAR: Solo un hilo puede comprimir a la vez (evita race condition en File_Manager/output.XXX)
        pthread_mutex_lock(&compression_mutex);
        
        // Comprimir (esto genera File_Manager/output.XXX)
        if (strcmp(compAlg, "huffman") == 0) {
            writeHuffman((char*)inPath);
            temp_produced = "File_Manager/output.bin";
        } else if (strcmp(compAlg, "rle") == 0) {
            writeRLE((char*)inPath);
            temp_produced = "File_Manager/output.rle";
        } else if (strcmp(compAlg, "lzw") == 0) {
            writeLZW((char*)inPath);
            temp_produced = "File_Manager/output.lzw";
        } else {
            fprintf(stderr, "Algoritmo desconocido: %s\n", compAlg);
            pthread_mutex_unlock(&compression_mutex);
            return NULL;
        }
        
        // CRÍTICO: Mover INMEDIATAMENTE a destino final para evitar que otro hilo lo sobrescriba
        if (!file_exists(temp_produced)) {
            fprintf(stderr, "[Thread] No se encontró salida de compresión: %s\n", temp_produced);
            pthread_mutex_unlock(&compression_mutex);
            return NULL;
        }
        
        // Mover el archivo temporal al destino final
        if (rename(temp_produced, final_dest) != 0) {
            fprintf(stderr, "[Thread] Error moviendo %s -> %s: %s\n", 
                    temp_produced, final_dest, strerror(errno));
            pthread_mutex_unlock(&compression_mutex);
            return NULL;
        }
        
        // DESBLOQUEAR: Permitir que otro hilo comprima
        pthread_mutex_unlock(&compression_mutex);
        
        printf("Archivo comprimido guardado en: %s\n", final_dest);
        return NULL;
    }

    if (op_d) {
        // Descompresión: entrada -> File_Manager/output.txt
        if (!file_exists(inPath)) {
            fprintf(stderr, "Entrada no encontrada: %s\n", inPath);
            return NULL;
        }
        
        // Determinar destino final primero
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
        
        // BLOQUEAR: Un solo hilo puede descomprimir a la vez
        pthread_mutex_lock(&compression_mutex);
        
        int result;
        if (strcmp(compAlg, "huffman") == 0) {
            result = readHuffman((char*)inPath);
        } else if (strcmp(compAlg, "rle") == 0) {
            result = readRLE((char*)inPath);
        } else if (strcmp(compAlg, "lzw") == 0) {
            result = readLZW((char*)inPath);
        } else {
            fprintf(stderr, "Algoritmo desconocido: %s\n", compAlg);
            pthread_mutex_unlock(&compression_mutex);
            return NULL;
        }
        
        if (result != 0) {
            fprintf(stderr, "Fallo al descomprimir %s\n", inPath);
            pthread_mutex_unlock(&compression_mutex);
            return NULL;
        }
        const char* produced = "File_Manager/output.txt";
        if (!file_exists(produced)) {
            fprintf(stderr, "No se encontró salida de descompresión: %s\n", produced);
            pthread_mutex_unlock(&compression_mutex);
            return NULL;
        }

        // Mover inmediatamente al destino final
        if (strcmp(final_dest, produced) != 0) {
            if (move_file(produced, final_dest) != 0) {
                pthread_mutex_unlock(&compression_mutex);
                return NULL;
            }
        }
        
        // DESBLOQUEAR: Permitir que otro hilo descomprima
        pthread_mutex_unlock(&compression_mutex);
        
        printf("Archivo descomprimido guardado en: %s\n", final_dest);
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
        
        printf("Archivo encriptado guardado en: %s\n", dest);
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
            const char* comp_ext = get_extension(inPath);
            int decomp_result = 1;
            if (comp_ext && strcmp(comp_ext, "rle") == 0) {
                decomp_result = readRLE((char*)dest);
            } else if (comp_ext && strcmp(comp_ext, "lzw") == 0) {
                decomp_result = readLZW((char*)dest);
            } else if (comp_ext && (strcmp(comp_ext, "bin") == 0 || strcmp(comp_ext, "huff") == 0)) {
                decomp_result = readHuffman((char*)dest);
            } else {
                // Probar descompresores hasta que uno funcione
                if (readRLE((char*)dest) == 0) decomp_result = 0;
                else if (readLZW((char*)dest) == 0) decomp_result = 0;
                else if (readHuffman((char*)dest) == 0) decomp_result = 0;
            }

            if (decomp_result != 0) {
                fprintf(stderr, "Fallo al descomprimir archivo desencriptado: %s\n", dest);
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

            if (file_exists("File_Manager/output.txt")) {
                if (move_file("File_Manager/output.txt", dest_final) != 0) return NULL;
            } else if (file_exists("File_Manager/output.bin")) {
                if (move_file("File_Manager/output.bin", dest_final) != 0) return NULL;
            } else if (file_exists("File_Manager/output.lzw")) {
                if (move_file("File_Manager/output.lzw", dest_final) != 0) return NULL;
            } else {
                // Alternativa: mover el archivo desencriptado en sí mismo
                if (move_file(dest, dest_final) != 0) return NULL;
            }

            // Eliminar archivo intermedio desencriptado/comprimido
            if (file_exists(dest)) unlink(dest);

            printf("Archivo desencriptado y descomprimido guardado en: %s\n", dest_final);
            return NULL;
        }
        
        printf("Archivo desencriptado guardado en: %s\n", dest);
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

/*
Here we start with the interaction with File_Manager. 
First let's realize if we are dealing with a file or a folder. 
If only a file, then only a thread. If a folder, then let's use several threads IN PARALLEL.
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
        printf("It is a folder - processing files in PARALLEL...\n");

        // Crear carpeta de salida (usar -o si se proporciona, de lo contrario usar <entrada>_processed por defecto)
        char outFolder[1024];
        if (myargs.outPath && myargs.outPath[0] != '\0') {
            snprintf(outFolder, sizeof(outFolder), "%s", myargs.outPath);
        } else {
            snprintf(outFolder, sizeof(outFolder), "%s_processed", path);
        }
        mkdir(outFolder, 0755);

        // Abrir directorio
        DIR *dir = opendir(path);
        if (!dir) { perror("Error opening directory"); return; }

        // Array dinámico para almacenar información de hilos
        ThreadInfo* threads = NULL;
        int thread_count = 0;
        int thread_capacity = 0;

        struct dirent *entry;

        // FASE 1: Crear todos los hilos (ejecución paralela)
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

            struct stat entry_st;
            if (stat(full_path, &entry_st) != 0 || !S_ISREG(entry_st.st_mode))
                continue;

            // Expandir arreglo si es necesario
            if (thread_count >= thread_capacity) {
                thread_capacity = (thread_capacity == 0) ? 10 : thread_capacity * 2;
                threads = realloc(threads, thread_capacity * sizeof(ThreadInfo));
                if (!threads) {
                    perror("Error al reservar memoria para hilos");
                    closedir(dir);
                    return;
                }
            }

            // Preparar argumentos para este archivo (allocar memoria persistente)
            ThreadArgs* ta = malloc(sizeof(ThreadArgs));
            if (!ta) {
                perror("Error al reservar memoria para ThreadArgs");
                continue;
            }
            
            *ta = myargs;
            ta->inPath = strdup(full_path);

            const char* base = get_basename(entry->d_name);
            // Quitar extensión del nombre base
            char name_noext[512];
            strncpy(name_noext, base, sizeof(name_noext)); 
            name_noext[sizeof(name_noext)-1] = '\0';
            char* dot = strrchr(name_noext, '.');
            if (dot) *dot = '\0';

            char out_full[1024];
            
            // Si se encripta un directorio, comprimir primero y luego encriptar para que la desencriptación pueda restaurar los originales
            if (myargs.op_e) {
                ta->op_e = true;
                ta->op_c = true;
                if (!ta->compAlg || ta->compAlg[0] == '\0') {
                    ta->compAlg = "rle";  // Usar RLE por defecto si no se especifica
                }
                snprintf(out_full, sizeof(out_full), "%s/%s.bin", outFolder, name_noext);
                ta->outPath = strdup(out_full);
            }
            // Si se desencripta, establecer outPath a la carpeta de salida para que funcione la descompresión automática
            else if (myargs.op_u) {
                snprintf(out_full, sizeof(out_full), "%s/%s", outFolder, base);
                ta->outPath = strdup(out_full);
            } else {
                // Construir un nombre de archivo de salida único dentro de outFolder basado en el nombre base de entrada
                const char* compAlg = myargs.compAlg;
                const char* ext = "";
                if (strcmp(compAlg, "huffman") == 0) ext = "bin";
                else if (strcmp(compAlg, "rle") == 0) ext = "rle";
                else if (strcmp(compAlg, "lzw") == 0) ext = "lzw";

                if (ext[0] != '\0')
                    snprintf(out_full, sizeof(out_full), "%s/%s.%s", outFolder, name_noext, ext);
                else
                    snprintf(out_full, sizeof(out_full), "%s/%s", outFolder, name_noext);

                ta->outPath = strdup(out_full); // Destino por archivo

                // Si estamos procesando un directorio para descompresión, intentar detectar el algoritmo por archivo
                if (ta->op_d) {
                    const char* fileext = get_extension(entry->d_name);
                    if (strcmp(fileext, "rle") == 0) {
                        ta->compAlg = "rle";
                    } else if (strcmp(fileext, "lzw") == 0) {
                        ta->compAlg = "lzw";
                    } else if (strcmp(fileext, "bin") == 0 || strcmp(fileext, "huff") == 0) {
                        ta->compAlg = "huffman";
                    } else {
                        // Extensión desconocida: usar el algoritmo proporcionado
                        ta->compAlg = myargs.compAlg;
                    }
                }
            }

            // Crear hilo para procesar este archivo EN PARALELO
            threads[thread_count].args = ta;
            int ret = pthread_create(&threads[thread_count].thread, NULL, operationOneFile, ta);
            if (ret != 0) {
                fprintf(stderr, "Error creando hilo para %s: %s\n", entry->d_name, strerror(ret));
                free((char*)ta->inPath);
                free((char*)ta->outPath);
                free(ta);
                continue;
            }
            
            printf("  → Hilo %d: procesando '%s' en paralelo...\n", thread_count + 1, entry->d_name);
            thread_count++;
        }
        
        closedir(dir);

        // FASE 2: Esperar a que todos los hilos terminen (join)
        printf("\nEsperando a que terminen %d hilos...\n", thread_count);
        for (int i = 0; i < thread_count; i++) {
            pthread_join(threads[i].thread, NULL);
            printf("  ✓ Hilo %d completado\n", i + 1);
            
            // Liberar memoria de argumentos
            free((char*)threads[i].args->inPath);
            free((char*)threads[i].args->outPath);
            free(threads[i].args);
        }

        // Liberar array de hilos
        free(threads);
        
        printf("\n✅ Procesamiento paralelo completado. %d archivos procesados.\n", thread_count);
    } else {
        printf("It is neither a regular file nor a directory.\n");
    }
}
