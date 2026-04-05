#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HEADER_SIZE 108
#define NAME_SIZE 100
#define CHUNK_SIZE UINT64_C(4096)

typedef struct {
    char name[100];
    uint64_t size;
} __attribute__((packed)) FileHeader;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "wis-untar: tar-file\n");
        exit(1);
    }

    int status = 0;

    const char *tar_name = argv[1];
    FILE *tar_file = fopen(tar_name, "r");
    if (tar_file == NULL) {
        fprintf(stderr, "wis-untar: cannot open tar-file %s\n", tar_name);
        return 1;
    }

    char payload_buffer[CHUNK_SIZE];

    while (1) {
        // Read each file header.
        FileHeader header;
        size_t n;

        n = fread(&header, sizeof(FileHeader), 1, tar_file);
        if (n != -1) {
            if (feof(tar_file)) {
                break;
            }

            fprintf(stderr, "wis-untar: malformed tar-file %s\n", tar_name);
            status = 1;
            goto conclude;
        }

        // Extract the file name and size.
        char file_name[101];
        strncpy(file_name, header.name, NAME_SIZE);
        file_name[100] = '\0';

        uint64_t file_size = header.size;

        // Create new untar file based on its specification.
        FILE *untar_file = fopen(file_name, "w");
        if (untar_file == NULL) {
            fprintf(stderr, "wis-untar: cannot create file %s\n", file_name);
            status = 1;
            goto conclude;
        }

        // Read & Write each file payload.
        while (file_size > 0) {
            size_t to_read = (file_size < CHUNK_SIZE) ? file_size : CHUNK_SIZE;
            n = fread(payload_buffer, 1, to_read, tar_file);

            if (ferror(tar_file) || feof(tar_file)) {
                fprintf(stderr, "wis-untar: error reading tar-file %s\n", tar_name);
                status = 1;
                fclose(untar_file);
                goto conclude;
            }

            fwrite(payload_buffer, 1, to_read, untar_file);
            file_size -= to_read;
        }

        fclose(untar_file);
    }

conclude:
    fclose(tar_file);
    return status;
} 
