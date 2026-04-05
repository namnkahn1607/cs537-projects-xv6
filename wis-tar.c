#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define HEADER_SIZE 108
#define NAME_SIZE 100
#define BUFFER_SIZE 4096

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "wis-tar: tar-file file [...]\n");
        exit(1);
    }

    const char *tar_name = argv[1];
    FILE *tar_file = fopen(tar_name, "w");
    if (tar_file == NULL) {
        fprintf(stderr, "wis-tar: cannot create tar-file %s\n", tar_name);
        exit(1);
    }

    for (int i = 2; i < argc; ++i) {
        char *file_name = argv[i];

        FILE *file = fopen(file_name, "r");
        if (file == NULL) {
            fprintf(stderr, "wis-tar: cannot open file %s\n", file_name);
            exit(1);
        }

        struct stat info;
        if (stat(file_name, &info) == -1) {
            fprintf(stderr, "wis-tar: error finding the size of a file\n");
            exit(1);
        }

        uint64_t file_size = (uint64_t)info.st_size;

        char header[HEADER_SIZE];
        memset(header, '\0', HEADER_SIZE);
        strncpy(header, file_name, NAME_SIZE);
        memcpy(header + NAME_SIZE, &file_size, sizeof(uint64_t));

        // Write the 100-byte file name and 8-byte file size.
        fwrite(header, HEADER_SIZE, 1, tar_file);

        // Write the content of the input file.
        char payload_buffer[BUFFER_SIZE];
        size_t n;
        while ((n = fread(payload_buffer, 1, sizeof(payload_buffer), file)) > 0) {
            fwrite(payload_buffer, 1, n, tar_file);
        }

        fclose(file);
    }   

    fclose(tar_file);
    return 0;
}
