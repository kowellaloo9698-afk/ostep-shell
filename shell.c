#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
int main(){
	char *line=NULL;
	size_t len=0;
	while(1){
		printf("> ");
		ssize_t n=getline(&line,&len,stdin);
		if(n==-1){
			printf("\n");
			break;
			}
		if(n>0 && line[n-1]=='\n')line[n-1]='\0';
		char *tokens[64];
		int count=0;
		char *tok=strtok(line," ");
		while(tok!=NULL){
			tokens[count]=tok;
			count++;
			tok=strtok(NULL," ");
		}
		tokens[count]=NULL;
		pid_t rc=fork();
		if(rc<0)perror("fork");
		else if(rc==0){
			execvp(tokens[0],tokens);
			perror("execvp");
			exit(1);
			}
		else{
		        wait(NULL);
		}
	}
	free(line);
	return 0;
}
