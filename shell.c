#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(){
	char *line=NULL;
	size_t len=0;
	while(1){
		printf("> ");
		ssize_t n=getline(&line,&len,stdin);
		if(n==-1)break;
		if(n>0 && line[n-1]=='\n')line[n-1]='\0';
		char *tokens[64];
		int count=0;
		char *tok=strtok(line," ");
		while(tok!=NULL){
			tokens[count]=tok;
			count++;
			tok=strtok(NULL," ");
		}
		for(int i=0;i<count;i++)printf("token[%d] = %s\n",i,tokens[i]);
	}
	free(line);
	return 0;
}
