#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define nl 2000
void removeNums(char *str,int move){
	int i=0;
	for(i=0;i<nl-move;i++){
		str[i]=str[i+move];
	}
}

void main(){
	FILE *fpRead,*fpWrite;
	char oneLine[nl];
	fpRead=fopen("removeNumber.txt","r");
	fpWrite=fopen("removed.txt","w");
	while(!feof(fpRead)){
		fgets(oneLine,nl,fpRead);
		removeNums(oneLine,4);
		fputs(oneLine,fpWrite);
	}
	printf("finished ! \n");
	fclose(fpRead);
	fclose(fpWrite);
}
