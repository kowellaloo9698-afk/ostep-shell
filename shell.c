#include <stdio.h>      // printf, perror, getline
#include <stdlib.h>     // exit, getenv, free
#include <string.h>     // strcmp, strtok
#include <unistd.h>     // fork, execvp, chdir
#include <sys/wait.h>   // wait, WIFEXITED, WEXITSTATUS

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
        while (tok != NULL) {
            tokens[count] = tok;
            count++;
            tok = strtok(NULL, " ");
        }
        tokens[count] = NULL;

        if (tokens[0] == NULL) continue;

        if (strcmp(tokens[0], "cd") == 0) {
            if (tokens[1] == NULL) chdir(getenv("HOME"));
            else if (chdir(tokens[1]) != 0) perror("cd");
        } else if (strcmp(tokens[0], "exit") == 0) {
            exit(0);
        } else {
            pid_t rc = fork();
            if (rc < 0) {
                perror("fork");
            } else if (rc == 0) {
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

    free(line);
    return 0;
}
