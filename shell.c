#include <stdio.h>      // printf, perror, getline
#include <stdlib.h>     // exit, getenv, free
#include <string.h>     // strcmp, strtok
#include <unistd.h>     // fork, execvp, chdir, dup2, close
#include <sys/wait.h>   // wait, WIFEXITED, WEXITSTATUS
#include <fcntl.h>      // open() and its O_* flags

int main() {
    char *line = NULL;
    size_t len = 0;

    while (1) {
        printf("> ");

        // --- READ ---
        ssize_t n = getline(&line, &len, stdin);
        if (n == -1) {              // EOF (Ctrl-D) — getline returns -1
            printf("\n");
            break;
        }
        if (n > 0 && line[n-1] == '\n') line[n-1] = '\0';  // strip trailing newline

        // --- TOKENIZE ---  blob string -> NULL-terminated argv array
        char *tokens[64];
        int count = 0;
        char *tok = strtok(line, " ");
        while (tok != NULL) {
            tokens[count] = tok;
            count++;
            tok = strtok(NULL, " ");
        }
        tokens[count] = NULL;       // sentinel — I place it, strtok doesn't

        if (tokens[0] == NULL) continue;   // empty line — nothing to run

        // --- DETECT REDIRECTION ---  must run AFTER tokenizing (needs the
        // finished array) and AFTER the empty-line guard (no tokens to scan
        // otherwise). Scan for ">" at ANY position, not a fixed index.
         char *infile=NULL;
	 char *outfile = NULL;
        for (int i = 0; tokens[i] != NULL; i++) {
            if (strcmp(tokens[i], ">") == 0) {
                outfile = tokens[i + 1];   // filename is the token after ">"
                tokens[i] = NULL;          // amputate argv here — execvp stops at NULL
            }else if(strcmp(tokens[i],"<")==0){
		infile=tokens[i+1];
		tokens[i]=NULL;
	    }
        }

        // --- BUILT-INS (run in the parent — their state must persist) ---
        if (strcmp(tokens[0], "cd") == 0) {
            if (tokens[1] == NULL) chdir(getenv("HOME"));      // bare cd -> home
            else if (chdir(tokens[1]) != 0) perror("cd");      // chdir moves THIS process's cwd
        } else if (strcmp(tokens[0], "exit") == 0) {
            exit(0);
        } else {
            // --- EXTERNAL COMMAND ---
            pid_t rc = fork();
            if (rc < 0) {
                perror("fork");        // fork failed — skip this command, keep the shell alive
            } else if (rc == 0) {
                // CHILD: rewire fds BEFORE exec (exec overwrites the image;
                // the fd table survives it). Redirect dies with this child.
                if (outfile != NULL) {
                    int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd < 0) { perror("open"); exit(1); }
                    dup2(fd, 1);       // slot 1 -> file (THIS is the redirect)
                    close(fd);         // drop the spare slot; slot 1 already points right
                }
		if(infile!=NULL){
			int fd = open(infile, O_RDONLY);
			if (fd<0){perror("open");exit(1);}
			dup2(fd,0);
			close(fd);
		}
                execvp(tokens[0], tokens);
                perror("execvp");      // only reached if exec FAILED
                exit(1);               // kill the failed child so it can't loop back
            } else {
                // PARENT: block until child done, decode its exit status
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
