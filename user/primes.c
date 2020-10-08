#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc,char *argv[]){

	int fd[35][2];

	int id=0;
	int n;
	int i;

	pipe(fd[0]);
	int child_pid;
	if ((child_pid=fork())>0){ //parent, root

		close(fd[id][0]);
		printf("prime %d\n",2);
		for(i=3;i<=35;i++){
			if(i%2!=0){
				write(fd[id][1],&i,sizeof(int));
			}
		}
		close(fd[id][1]);
		wait(&child_pid);
	}else{
		while(1==1){
			id++;
			close(fd[id-1][1]);
			int num;
			int gate;
			n=read(fd[id-1][0],&num,sizeof(int));
			if (n==0){
				break;
			}
			gate=num;
			printf("prime %d\n",num);
			pipe(fd[id]);
			if ((child_pid= fork())>0){
				close(fd[id][0]);
				while(read(fd[id-1][0],&num,sizeof(int))!=0){
					if (num%gate!=0) write(fd[id][1],&num,sizeof(int));
				}
				close(fd[id][1]);
				close(fd[id-1][0]);

				wait(&child_pid);
				break;
			}
		}
	}

	exit(0);
}
