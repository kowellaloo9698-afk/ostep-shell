#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

int main() {
    char *line = NULL;
    size_t len = 0;

    while (1) {
        printf("> ");

        ssize_t n = getline(&line, &len, stdin);
        if (n == -1) {
            printf("\n");
            break;
        }
        if (n > 0 && line[n-1] == '\n') line[n-1] = '\0';

        char *tokens[64];
        int count = 0;
        char *tok = strtok(line, " ");
        while (tok != NULL && count < 63) {
            tokens[count] = tok;
            count++;
            tok = strtok(NULL, " ");
        }
        tokens[count] = NULL;

        if (tokens[0] == NULL) continue;

        // --- reject redirection combined with pipe (out of scope) ---
        int has_pipe = 0;
        int has_redir = 0;
        for (int i = 0; tokens[i] != NULL; i++) {
            if (strcmp(tokens[i], "|") == 0) has_pipe = 1;
            if (strcmp(tokens[i], ">") == 0 || strcmp(tokens[i], "<") == 0) has_redir = 1;
        }
        if (has_pipe && has_redir) {
            fprintf(stderr, "shell: redirection and pipe cannot be combined\n");
            continue;
        }

        char *infile = NULL;
        char *outfile = NULL;
        for (int i = 0; tokens[i] != NULL; i++) {
            if (strcmp(tokens[i], ">") == 0) {
                if (tokens[i + 1] == NULL) {
                    fprintf(stderr, "shell: expected filename after >\n");
                    tokens[0] = NULL;
                    break;
                }
                outfile = tokens[i + 1];
                tokens[i] = NULL;
            } else if (strcmp(tokens[i], "<") == 0) {
                if (tokens[i + 1] == NULL) {
                    fprintf(stderr, "shell: expected filename after <\n");
                    tokens[0] = NULL;
                    break;
                }
                infile = tokens[i + 1];
                tokens[i] = NULL;
            }
        }
        if (tokens[0] == NULL) continue;

        if (strcmp(tokens[0], "cd") == 0) {
            if (tokens[1] == NULL) chdir(getenv("HOME"));
            else if (chdir(tokens[1]) != 0) perror("cd");
        } else if (strcmp(tokens[0], "exit") == 0) {
            exit(0);
        } else {
            int pipe_index = -1;
            for (int i = 0; tokens[i] != NULL; i++) {
                if (strcmp(tokens[i], "|") == 0) {
                    pipe_index = i;
                    break;
                }
            }

            if (pipe_index != -1) {
                tokens[pipe_index] = NULL;
                char **left  = tokens;
                char **right = &tokens[pipe_index + 1];

                if (left[0] == NULL || right[0] == NULL) {
                    fprintf(stderr, "shell: empty command near |\n");
                    continue;
                }

                int fds[2];
                if (pipe(fds) < 0) { perror("pipe"); continue; }

                pid_t rc1 = fork();
                if (rc1 < 0) { perror("fork"); continue; }
                if (rc1 == 0) {
                    dup2(fds[1], 1);
                    close(fds[0]);
                    close(fds[1]);
                    execvp(left[0], left);
                    perror("execvp");
                    exit(1);
                }

                pid_t rc2 = fork();
                if (rc2 < 0) { perror("fork"); continue; }
                if (rc2 == 0) {
                    dup2(fds[0], 0);
                    close(fds[0]);
                    close(fds[1]);
                    execvp(right[0], right);
                    perror("execvp");
                    exit(1);
                }

                close(fds[0]);
                close(fds[1]);
                wait(NULL);
                wait(NULL);
            } else {
                pid_t rc = fork();
                if (rc < 0) {
                    perror("fork");
                } else if (rc == 0) {
                    if (outfile != NULL) {
                        int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (fd < 0) { perror("open"); exit(1); }
                        dup2(fd, 1);
                        close(fd);
                    }
                    if (infile != NULL) {
                        int fd = open(infile, O_RDONLY);
                        if (fd < 0) { perror("open"); exit(1); }
                        dup2(fd, 0);
                        close(fd);
                    }
                    execvp(tokens[0], tokens);
                    perror("execvp");
                    exit(1);
                } else {
                    int status;
                    wait(&status);
                    if (WIFEXITED(status)) {
                        int code = WEXITSTATUS(status);
                        printf("[exit code: %d]\n", code);
                    } else {
                        printf("[child killed by signal]\n");
                    }
                }
            }
        }
    }  // end while(1)

    free(line);
    return 0;
}