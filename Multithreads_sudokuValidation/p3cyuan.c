/*****************************************/
/* using multi thread to validate sudoku */
/* Author: Chengjun Yuan @UVa            */
/* Time:   Sept.26.2015                  */
/*****************************************/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#define numThreads 11
bool validateFlag=true;
int matrix[9][9];
typedef struct
{
	int row;
	int column;
}parameters;

// read Sudoku matrix from fileName, and store it in matrix[][].
void readSudokuMatrix(char *fileName){
	FILE *fp;
	int i,j;
	fp=fopen(fileName,"r");
	if(!fp){
		printf("ERROR: Cannot read file %s \n",fileName);
		exit;
	}
	for(i=0;i<9;i++){
		for(j=0;j<9;j++){
			if(i==8&&j==8)
				fscanf(fp,"%d",&matrix[i][j]);
			else 
				fscanf(fp,"%d ",&matrix[i][j]);
		}
	}
	fclose(fp);
}

void *validateRow(void *para){
	int flag[10];
	int i,j;
	for(i=0;i<9;i++){					// traverse all 9 rows.
		memset(flag,0,sizeof(flag));	// reset the values of flag[] to zero.
		for(j=0;j<9;j++){				// traverse one row.
			flag[matrix[i][j]]=1;		// mark the presence of digits by assigning 1.
		}
		for(j=1;j<=9;j++){				// examine the presence of digits 1-9.
			if(flag[j]==0){				// If digit j does not exist.
				printf("Digit %d is missing from Row %d \n",j,i+1);
				validateFlag=false;		// assign false to validateFlag if digit j is missing.
			}
		}
	}
}

void *validateCol(void *para){
	int flag[10];
	int i,j;
	for(j=0;j<9;j++){					// traverse all 9 columns.
		memset(flag,0,sizeof(flag));	// reset the values of flag[] to zero.
		for(i=0;i<9;i++){				// traverse one column.
			flag[matrix[i][j]]=1;		// mark the presence of digits by assigning 1.
		}
		for(i=1;i<=9;i++){				// examine the presence of digits 1-9.
			if(flag[i]==0){				// If digit i does not exist.
				printf("Digit %d is missing from Column %d \n",i,j+1);
				validateFlag=false;		// assign false to validateFlag if digit i is missing.
			}
		}
	}
}

void *validateSubgrid(void *para){
	int flag[10];
	int i,j;
	int istart,jstart;
	parameters *para_=para;				// get the first row # of the sub-grid.
	istart=3*(para_->row);				// get the first row # of the sub-grid.
	jstart=3*(para_->column);			// get the first column # of the sub-grid.
	memset(flag,0,sizeof(flag));		// reset the values of flag[] to zero.
	for(i=istart;i<istart+3;i++){		// traverse the sub-grid.
		for(j=jstart;j<jstart+3;j++){
			flag[matrix[i][j]]=1;		// mark the presence of digits by assigning 1.
		}
	}
	for(i=1;i<=9;i++){					// examine the presence of digits 1-9.
		if(flag[i]==0){					// If digit i does not exist.
			printf("Digit %d is missing from the 3x3 Subgrid of row (%d..%d) and column (%d..%d) \n",i,istart+1,istart+3,jstart+1,jstart+3);
			validateFlag=false;			// assign false to validateFlag if digit i is missing.
		}
	}
}

int main(int argc,char *argv[]){
	int i;
	pthread_mutex_t lock;				// define a mutual exclusion lock for threads.
	parameters *para[numThreads];
	pthread_t tid[numThreads];			// to store the id of each threads.
	pthread_attr_t attr;				// define the thread attribute.
	if(argc<2)readSudokuMatrix("sudoku.txt"); // read Sudoku from "sudoku.txt".
	else readSudokuMatrix(argv[1]);		// or from the specified file in the command line.
	pthread_mutex_init(&lock,NULL);		// initiate the mutual exclusion lock.
	pthread_attr_init(&attr);			// initiate the thread attribute.
	for(i=0;i<numThreads;i++){
		para[i]=(parameters*)malloc(sizeof(parameters));
		if(i==0){						// create the thread for row validation.
			pthread_create(&tid[i],&attr,validateRow,para[i]);
		}else if(i==1){					// create the thread for column validation.
			pthread_create(&tid[i],&attr,validateCol,para[i]);
		}else{							// create the threads for each sub-grid validation.
			para[i]->row=(i-2)/3; para[i]->column=(i-2)%3;
			pthread_create(&tid[i],&attr,validateSubgrid,para[i]);
		}
	}
	for(i=0;i<numThreads;i++)			// join each thread.
         pthread_join(tid[i],NULL);
	pthread_mutex_destroy(&lock);		// destroy the mutual exclusion lock.
	if(validateFlag)					// determine whether the Sudoku is valid or not.
		printf("The sudoku puzzle is VALID \n");
	else 
		printf("The sudoku puzzle is INVALID \n");
}