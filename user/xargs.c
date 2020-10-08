#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MAXPARAMS 32
#define MaxLineLen 128
#define MaxArgLen 128
int main(int argc,char *argv[]){
	if (argc<2){
		exit(0);
	}
	char *args[MAXPARAMS];
	int params=0;
	int n;


	char *cmd=argv[1];
	int i;
	for(i=1;i<argc;i++){
		args[params++]=argv[i];
	}
	char input[MaxLineLen];
	while((n=read(0,input,MaxLineLen))>0){
		if(fork()==0){
			char *arg=(char *)malloc(sizeof(char)*MaxArgLen);
			int ptr=0;
			for(i=0;i<n;i++){
				if(input[i]==' ' || input[i]=='\n' || input[i]=='\t') {
					arg[ptr++]='\0';
					args[params++]=arg;
					ptr=0;
					arg=(char *)malloc(sizeof(char)*MaxArgLen);
				}else{
					arg[ptr++]=input[i];
				}

			}
			arg[ptr++]='\0';
			args[params++]=arg;
			exec(cmd,args);
		}else{
			wait(0);
		}
	}
	exit(0);
}
