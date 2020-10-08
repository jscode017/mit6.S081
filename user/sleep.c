#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


void printSleepErrMsg(char* errMessage){
	write(1,errMessage,strlen(errMessage));
	exit(0);
}

int main(int argc,char *argv[]){
	if (argc<2) printSleepErrMsg("not enough arguments\n");
	if (argv[1][0]<'0' || argv[1][0]>'9') printSleepErrMsg("invalid argument\n");

	int sleepSec=atoi(argv[1]);
	sleep(sleepSec);
	exit(0);
}


