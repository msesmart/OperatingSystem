/***********************************************/
/*  A simple Linux shell in c language         */
/*  Designed by Chengjun Yuan @UVa 09.2015     */
/***********************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#define CommandSize 101

char *argv[100][100]; 
char line[150];  										// the input commands line.
char cwdBuffer[PATH_MAX+1];								// storage buffer for CWD.
int inputFlag=0; 										// the number of input redirection '<' in one command.
int outputFlag=0;										// the number of output redirection '>' in one command.
int afterRedirector=0;
pid_t pid,wp;

/*	Read the input commands, then store it in line. 
	If the number of characters exceed 100, return 
	false.*/
bool readLine(void){
    char *temp=NULL;
	fflush(NULL);
	fgets(line,150,stdin);
	if(strlen(line)>101)
		return false;
	temp=strchr(line,'\n'); 
	if(temp!=NULL)
		temp[0]='\0';
	return true;
}

/*	Check whether the token is a word or not. Return
	true if yes or false if no.*/
bool checkTokenIsWord(char *token){
    char c=token[0]; int i=0;
    while(c!='\0'){
        if(c==EOF){
            printf("EOF: EXIT ownShell\n"); exit(0);
        }
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='/'||c=='_'||c=='-'||c=='.')){
            printf("ERROR: Invalid token %s is not a word \n",token); 
			return false;
        }
        i++;
        c=token[i];
    }
    return true;
}

/*	Check whether the token is valid or not. Return 
	true if yes or false if no. */
bool checkToken(char *token){
    if(strcmp(token,"<")==0){
        inputFlag++;
		if(outputFlag>0){
			printf("ERROR: '>' is located before '<' \n"); return false;
		}
        if(inputFlag>1){
            printf("ERROR: Number of input redirection '<' exceed two in one command \n"); 
			return false;
        }
		afterRedirector=1;
        return true;
    }
    else if(strcmp(token,">")==0){
        outputFlag++;
        if(outputFlag>1){
            printf("ERROR: Number of output redirection '>' exceed two in one command \n"); 
			return false;
        }
		afterRedirector=1;
        return true;
    }
    else{
        return checkTokenIsWord(token);
    }
}

/*	Skip the space character at the beginning of token.*/
char *skipSpace(char *token){
	while(isspace(*token))token++;
	return token;
}

/*	Split one command into several tokens. They are saved
	in argv[][]. Each token and the whole command are well
	checked to make sure they are correct. */
bool splitCommand(char *command_,int i_){
	int i=0; char temp[100];
	char *pCommand_=skipSpace(command_);
	char *next_=strchr(pCommand_,' ');					// Detect ' ' to split tokens.
	inputFlag=0; 
	outputFlag=0; 
	afterRedirector=0;
	while(next_!=NULL){
		next_[0] = '\0';
		if(afterRedirector==1){
			strcpy(temp,"which "); 
			strcat(temp,pCommand_);
			if(system(temp)==0){
				printf("ERROR: command follow the redirection operator \n"); 
				return false;
			}
		}
		afterRedirector=0;
		if(i==0){
            if(!checkTokenIsWord(pCommand_)){
				printf("ERROR: the command does not begin with a word \n");
				return false;
			}
		}else{
            if(!checkToken(pCommand_))
				return false;
		}
		argv[i_][i]=pCommand_;
		pCommand_=skipSpace(next_+1);
		next_=strchr(pCommand_,' ');
		i++;
	}
	if(pCommand_[0]!='\0'){								// The last token in the command.
        next_=strchr(pCommand_,'\0');
		next_[0]='\0';
		if(afterRedirector==1){
			strcpy(temp,"which "); 
			strcat(temp,pCommand_);
			if(system(temp)==0){
				printf("ERROR: the redirection operator is followed by a command \n"); 
				return false;
			}
		}
		afterRedirector=0;
        if(i==0){
            if(!checkTokenIsWord(pCommand_))
				return false;
		}else{
            if(!checkToken(pCommand_))
				return false;
		}
		argv[i_][i]=pCommand_;
		i++;
		argv[i_][i]=NULL;
		if(i_>0&&inputFlag>0){
			printf("ERROR: pipe operator is followed by an input redirection \n");
			return false;
		}
        return true;
	}else {
		if(i_>0&&inputFlag>0){
			printf("ERROR: pipe operator is followed by an input redirection \n");
			return false;
		}
	    if(i==0)return false;
        else return true;
    }
}

/*	Parse each line into several commands, restored in argv[]. 
	The splitCommand() is called, the error detections related
	with pipe operator are located, and the number of commands
	is obtained here.*/
bool parseLine(int *numCommand_){
	char *command_=line; char *next_,*temp; int i=0;
	inputFlag=0; 
	outputFlag=0; 
	afterRedirector=0;
	next_=strchr(command_,'|'); 						// Detect the pipe operator '|'.
	while(next_!=NULL){
        temp=next_-1;
        if(temp[0]!=' '){
            printf("ERROR: No space before | \n"); 
			return false;
        }
        if(outputFlag!=0&&i>0){ 
            printf("ERROR: an output redirection is followed by | \n"); 
			return false;
        }
		temp[0]='\0';   
		if(!splitCommand(command_,i))return false;
		command_=next_+1;
		next_=strchr(command_,'|');
		i++;
	}
	if(outputFlag!=0&&i>0){
		printf("ERROR: an output redirection is followed by | \n"); 
		return false;
	}
	if(!splitCommand(command_,i)){						// The last command in the line. 
		if(i>0)printf("ERROR: | is located at end of line \n"); 
		return false;
	}
	else{
		*numCommand_=i+1; 
		return true;
	}
}

/*	Check whether the i_th command contains input or output 
	redirection operator. Return 0 if no, or the position if
	exists. */
int checkInputOutput(int i_,char *s){
    int i=0,j=0;
    while(argv[i_][i]!=NULL){
        if(strcmp(argv[i_][i],s)==0){
			j=i; break;
		}
        i++;
    }
    if(j>0){ // if the operator exists, remove it from the command.
        i=j; 
		while(argv[i_][i]!=NULL){
			argv[i_][i]=argv[i_][i+1]; 
			i++;
		}
    }
    return j;
}

/*	If there is only one command in the line.*/
void singleCommand(void){
	int in,out,input,output,status; 
	if(strcmp(argv[0][0],"exit")==0)
		exit(0);
	else if(strcmp(argv[0][0],"clear")==0)
		system("clear");
	else{
		pid=fork();
		if(pid<0){
			printf("ERROR: create child process failed \n"); return ;
		}else if(pid==0){
			in=checkInputOutput(0,"<");					// Search the position of input redirection operator '<'.
			out=checkInputOutput(0,">");				// Search the position of output redirection operator '>'.
			if(in>0){ 									// If the input redirection operator '<' exists.                  
				input=open(argv[0][in],O_RDONLY,0600);	// Open input file.
				argv[0][in]=NULL;
				if(input<0){printf("ERROR: input file %s can not be opened \n",argv[0][in]); return;}
				dup2(input,STDIN_FILENO); 				// Redirect STDIN_FILENO to input file.
				close(input);							// Close input file.
			}
			if(out>0){ 									// If the output redirection operator '>' exists. 
				output=open(argv[0][out],O_CREAT|O_TRUNC|O_WRONLY,S_IRUSR|S_IRGRP|S_IWGRP|S_IWUSR); // Open output file.
				argv[0][out]=NULL;
				if(output<0){printf("ERROR: output file %s can not be created \n",argv[0][out]); return;}
				dup2(output,STDOUT_FILENO); 			// Redirect STDOUT_FILENO to output file.
				close(output);							// Close output file.
			}
			if(execvp(argv[0][0],argv[0])==-1){ 		// Execute the command.
				printf("ERROR: execvp \n"); 
				kill(getpid(),SIGTERM);
			}		
		}else{
			do{
				wp=waitpid(pid,&status,WUNTRACED|WCONTINUED);
				if(wp==-1){perror("waitpid");exit(EXIT_FAILURE);}
				if(WIFEXITED(status)){
					printf("exited, status=%d\n",WEXITSTATUS(status));
				}else if(WIFSIGNALED(status)){
					printf("killed by signal %d\n",WTERMSIG(status));
				}else if(WIFSTOPPED(status)){
					printf("stopped by signal %d\n",WSTOPSIG(status));
				}else if(WIFCONTINUED(status)){
					printf("continued\n");
				}
			}while(!WIFEXITED(status)&&!WIFSIGNALED(status));
		}	
	}
}

/*	If there are more than one command in the line, run
	the multiple pipes structure command execute. */
void pipeCommand(int numCommand_){
	int status;
	int pipe1[2],pipe2[2]; 						// for odd & even number of i. [0] for output, [1] for input
	int i=0;
	while(i<numCommand_){
		if(strcmp(argv[i][0],"exit")==0)exit(0);
		if(i%2==0) pipe(pipe2); 
		else pipe(pipe1);
		pid=fork();
		if(pid==0){
			if(i==0) dup2(pipe2[1],STDOUT_FILENO); 	// Redirect STDOUT_FILENO to pipe2[1] for the first command.
			else if(i==numCommand_-1){					// for the last command.
				if(i%2==0) dup2(pipe1[0],STDIN_FILENO);// Redirect STDIN_FILENO to pipe1[0] for even i.
				else dup2(pipe2[0],STDIN_FILENO);	// Redirect STDIN_FILENO to pipe2[0] for odd i.
			}else{ 									// for the middle commands.
				if(i%2==0){ 						// if i is an even number.
					dup2(pipe1[0],STDIN_FILENO); 	// Redirect STDIN_FILENO to pipe1[0].
					dup2(pipe2[1],STDOUT_FILENO);	// Redirect STDOUT_FILENO to pipe2[1].
				}else{      							// if i is an odd number.
					dup2(pipe2[0],STDIN_FILENO); 	// ...
					dup2(pipe1[1],STDOUT_FILENO);	// ...
				}
			}
			if(execvp(argv[i][0],argv[i])==-1){printf("ERROR: execvp \n"); kill(getpid(),SIGTERM);}		
		}else{
			if(i==0)close(pipe2[1]); 				// Closed the unused half of pipes.
			else if(i==numCommand_-1){
				if(i%2==0) close(pipe1[0]);			// ...
				else close(pipe2[0]);				// ...
			}else{
				if(i%2==0){
					close(pipe1[0]);				// ...
					close(pipe2[1]);				// ...
				}else{
					close(pipe2[0]);				// ...
					close(pipe1[1]);				// ...
				}
			}
			do{
				wp=waitpid(pid,&status,WUNTRACED|WCONTINUED);
				if(wp==-1){perror("waitpid");exit(EXIT_FAILURE);}
				if(WIFEXITED(status)){
					printf("exited, status=%d\n",WEXITSTATUS(status));
				}else if(WIFSIGNALED(status)){
					printf("killed by signal %d\n",WTERMSIG(status));
				}else if(WIFSTOPPED(status)){
					printf("stopped by signal %d\n",WSTOPSIG(status));
				}else if(WIFCONTINUED(status)){
					printf("continued\n");
				}
			}while(!WIFEXITED(status)&&!WIFSIGNALED(status));
		}
		i++;	
	}
}

int main(void){
	int numCommand=0;
	printf("\n***************************************\n");
	printf("            Welcome to ownShell          \n");
	printf("              by Chengjun Yuan           \n");
	printf("               @UVa Spet.2015            \n");
	printf("***************************************\n\n");
	setenv("current",getcwd(cwdBuffer,PATH_MAX+1),1); 
	while(1){
		printf("osh>");
		if(readLine()&&parseLine(&numCommand)){  		// Read input line & parse line into commands.
			if(numCommand==1)singleCommand();			// If the line contains only one command.
			else pipeCommand(numCommand);				// If there are multiple commands in pipe-structure line.
		}
	}
	return EXIT_SUCCESS;
}
