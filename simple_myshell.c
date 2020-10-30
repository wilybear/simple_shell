#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include<termios.h>
#include<sys/wait.h>
#define MAX_CMD_ARG 10
#define BUFSIZ 256

const char *prompt = "myshell> ";
char* cmdvector[MAX_CMD_ARG];
char  cmdline[BUFSIZ];

void child_handler();

void fatal(char *str){
	perror(str);
	exit(1);
}

int makelist(char *s, const char *delimiters, char** list, int MAX_LIST){	
  int i = 0;
  int numtokens = 0;
  char *snew = NULL;

  if( (s==NULL) || (delimiters==NULL) ) return -1;

  snew = s + strspn(s, delimiters);	/* Skip delimiters */
  if( (list[numtokens]=strtok(snew, delimiters)) == NULL )
    return numtokens;
	
  numtokens = 1;
  
  while(1){
     if( (list[numtokens]=strtok(NULL, delimiters)) == NULL)
	break;
     if(numtokens == (MAX_LIST-1)) return -1;
     numtokens++;
  }
  return numtokens;
}

int my_chdir(char *path){
    if(chdir(path)==-1){
        perror("chdir error");
        return -1;
    }

    return 1;
}

int main(int argc, char**argv){
  int i=0;
  pid_t pid;
  int status;
  int state;
  bool bgflag = false;
  struct sigaction act;
  act.sa_handler = child_handler;
  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_RESTART;

  state = sigaction(SIGCHLD,&act,0);
  if(state !=0 ){
    printf("err");
  }
  while (1) {
  	fputs(prompt, stdout);
    fgets(cmdline, BUFSIZ, stdin);
    cmdline[strlen(cmdline) -1] = '\0';
    
    //if cmdline is 0 segment fault may occur
    if(strlen(cmdline)!=0){
      i = makelist(cmdline, " \t", cmdvector, MAX_CMD_ARG);
      //cd command
      if(!strcmp(cmdvector[0], "cd")){
        my_chdir(cmdvector[1]);
        continue;
      }
      //exit command
      if(!strcmp(cmdvector[0], "exit")){
        exit(1);
      }
      if(!strcmp(cmdvector[i-1],"&")){
          cmdvector[i-1] = '\0';
          bgflag =true ;
      }

    }else{
      continue;
    }

    switch(pid=fork()){
      case 0:
        if(i == 0){
          i = makelist(cmdline, " \t", cmdvector, MAX_CMD_ARG);
        }
        if(bgflag==1){
          setpgid(0,0);
       //   tcsetpgrp(STDIN_FILENO, getpgid(0));
        }
        execvp(cmdvector[0], cmdvector);
        
        fatal("main()");  
      case -1:
        fatal("main()");
      default:
      /*
        if(bgflag==1){
            bgflag = 0;
            printf("bgflag = %d \n",bgflag);
            waitpid(pid,&status,WNOHANG);
           }else{
           printf("waitcalled\n");
          wait(NULL);
        }
        */
       if(bgflag){
          bgflag = false;
       }else{
        waitpid(pid, NULL, 0);
       // waitpid(-1,NULL,WNOHANG);
       }
				tcsetpgrp(STDIN_FILENO, getpgid(0));
    }
  }
  return 0; 
}


void child_handler(){
  pid_t child_pid;
  int state; 
  child_pid = waitpid(-1,0,WNOHANG);
  
}