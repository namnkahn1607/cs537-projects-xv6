#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_ARGS 128

// Arguments padding preprocessor subroutine
char *preprocess(char *line) {
    char *result = malloc(3 * strlen(line) + 1);
    int i = 0;
    for (int j = 0; line[j] != '\0'; ++j) {
        if (line[j] == '>') {
            result[i++] = ' ';
            result[i++] = line[j];
            result[i++] = ' ';
        } else {
            result[i++] = line[j];
        }
    }

    result[i] = '\0';
    return result;
}

// Shell subroutine function
void shell(FILE *file) { 
    int path_capacity = 128;
    int path_count = 1;
    char **paths = malloc(path_capacity * sizeof(char*));
    paths[0] = strdup("/bin"); // Initial path

    char *lineptr = NULL;
    size_t n = 0;

    // Read-Eval-Print Loop
    while (1) {
        if (file == stdin) { // Interactive prompt
            printf("smash> ");
            fflush(stdout);
        }

        // EOF -> Exit.
        if (getline(&lineptr, &n, file) == -1) {
            break;
        }

        char *processed_line = preprocess(lineptr);

        // Multiple (Sequential) Commands
        char *seq_line = processed_line;
        char *seq_cmd;
        while ((seq_cmd = strsep(&seq_line, ";")) != NULL) {
            if (seq_cmd[0] == '\0') {
                continue;
            }

            // Parallel Commands
            int wait_count = 0;
            char *para_cmd;
            while ((para_cmd = strsep(&seq_cmd, "&")) != NULL) {
                if (para_cmd[0] == '\0') {
                    continue;
                }

                // Arguments parsing & tokenization
                char *argv[MAX_ARGS + 1];
                argv[MAX_ARGS] = NULL;
                int argc = 0;

                char *token;
                while ((token = strsep(&para_cmd, " \t\n")) != NULL) {
                    if (argc > MAX_ARGS) {
                        fprintf(stderr, "An error has occurred\n");
                        free(processed_line);
                        goto close_shell;
                    }

                    // Tokens given by strsep() can be '\0', imagine
                    // tokenizing a string "ls     t", that's 5 SPACES.
                    if (token[0] != '\0') {
                        argv[argc++] = token;
                    }
                }

                argv[argc] = NULL; // End-of-argument

                // Can't read any args, move to the 
                // next prompt line (by continuing).
                if (argc == 0) {
                    continue;
                }

                // Handle built-in commands
                if (strcmp(argv[0], "exit") == 0) { // EXIT
                    if (argc > 1) {
                        fprintf(stderr, "An error has occurred\n");
                    }

                    for (int i = 0; i < path_count; ++i) {
                        free(paths[i]);
                    }

                    free(paths);
                    free(lineptr);
                    exit(0);

                } else if (strcmp(argv[0], "cd") == 0) { // CD
                    if (argc != 2 || chdir(argv[1]) != 0) {
                        fprintf(stderr, "An error has occurred\n");
                    }

                    continue;

                } else if (strcmp(argv[0], "path") == 0) { // PATH
                    if (argc <= 2) {
                        fprintf(stderr, "An error has occurred\n");
                        continue;
                    }

                    if (strcmp(argv[1], "add") == 0 && argc == 3) {
                        if (path_count == path_capacity) {
                            path_capacity *= 2;
                            paths = realloc(paths, path_capacity * sizeof(char*));
                        }

                        for (int i = path_count - 1; i >= 0; --i) {
                            paths[i + 1] = paths[i];
                        }

                        paths[0] = strdup(argv[2]);
                        ++path_count;

                    } else if (strcmp(argv[1], "remove") == 0 && argc == 3) {
                        int found = 0;

                        for (int i = 0; i < path_count; ++i) {
                            if (strcmp(paths[i], argv[2]) == 0) {
                                found = 1;
                                free(paths[i]);
                                --path_count;
                                for (int j = i; j < path_count; ++j) {
                                    paths[j] = paths[j + 1];
                                }

                                break;
                            }
                        }
                        
                        if (!found) {
                            fprintf(stderr, "An error has occurred\n");
                        }
                        
                    } else if (strcmp(argv[1], "clear") == 0 && argc == 2) {
                        for (int i = 0; i < path_count; ++i) {
                            free(paths[i]);
                        }

                        path_count = 0;
                    
                    } else {
                        fprintf(stderr, "An error has occurred\n");
                    }

                    continue;
                }

                // System command execution
                int child_pid = fork();
                if (child_pid < 0) {
                    fprintf(stderr, "An error has occurred\n");
                    continue;

                } else if (child_pid == 0) {
                    // Handle output redirection with '>'
                    int first_redir;
                    int redir_count = 0;
                    for (int i = 0; i < argc; ++i) {
                        if (strcmp(argv[i], ">") == 0) {
                            if (redir_count == 0) {
                                first_redir = i;
                            }

                            ++redir_count;
                        }
                    } 

                    if (redir_count > 0) {
                        if (redir_count > 1 || first_redir < 1 || argc - first_redir != 2) {
                        fprintf(stderr, "An error has occurred\n");
                        exit(1);
                        } else {
                            int new_fd = open(argv[first_redir + 1], O_CREAT | O_WRONLY | O_TRUNC, 0644);
                            if (new_fd == -1 || dup2(new_fd, STDOUT_FILENO) == -1 || dup2(new_fd, STDERR_FILENO) == -1) {
                                fprintf(stderr, "An error has occurred\n");
                                exit(1);
                            }

                            argv[first_redir] = NULL;
                        }
                    }
                    
                    size_t exec_len = strlen(argv[0]);
                    for (int i = 0; i < path_count; ++i) {
                        char *exec_dir = malloc(strlen(paths[i]) + 1 + exec_len + 1);
                        exec_dir = strcpy(exec_dir, paths[i]);
                        exec_dir = strcat(exec_dir, "/");
                        exec_dir = strcat(exec_dir, argv[0]);

                        if (access(exec_dir, X_OK) == 0) {
                            argv[0] = exec_dir;
                            break;
                        }

                        free(exec_dir);
                    }

                    free(lineptr);
                    
                    if (execv(argv[0], argv) == -1) {
                        fprintf(stderr, "An error has occurred\n");
                        exit(1);
                    }

                } else {
                    ++wait_count;
                }
            }

            while (wait_count > 0) {
                wait(NULL);
                --wait_count;
            }
        }

        free(processed_line);
    }

close_shell:
    if (lineptr != NULL) {
        free(lineptr);
    }

    for (int i = 0; i < path_count; ++i) {
        free(paths[i]);
    }

    free(paths);
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        shell(stdin);
        return 0;

    } else if (argc == 2) {
        FILE *file = fopen(argv[1], "r");
        if (file == NULL) {
            fprintf(stderr, "An error has occurred\n");
            return 1;
        }

        shell(file);
        fclose(file);
        return 0;
    }

    fprintf(stderr, "An error has occurred\n");
    return 1;
}
