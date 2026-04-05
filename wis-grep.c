#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_BUFFER 4096

void process_stream(FILE *file, const char *term, char **lineptr, size_t *n) {
    while (getline(lineptr, n, file) != -1) {
        if (strstr(*lineptr, term) != NULL) {
            printf("%s\n", *lineptr);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        printf("wis-grep: searchterm [file ...]\n");
        exit(1);
    }

    const char *term = argv[1];
    size_t n = LINE_BUFFER;
    int status = 0;

    char *lineptr = malloc(n);
    if (lineptr == NULL) {
        fprintf(stderr, "wis-grep: program encounter OOM\n");
        exit(1);
    }

    if (argc == 2) {
        process_stream(stdin, term, &lineptr, &n);
        goto cleanup;
    }

    for (int i = 2; i < argc; ++i) {
        FILE *file = fopen(argv[i], "r");
        if (file == NULL) {
            fprintf(stderr, "wis-grep: cannot open file %s\n", argv[i]);
            status = 1;
            goto cleanup;
        }

        process_stream(file, term, &lineptr, &n);
        fclose(file);
    }

cleanup:
    free(lineptr);
    return status;
}
