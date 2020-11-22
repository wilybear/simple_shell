#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/wait.h>
#include <errno.h>
#define MAX_CMD_ARG 10
#define BUFSIZ 256

const char *prompt = "myshell> ";

static bool restart = true;
static void child_handler();
static void sig_handler();

void fatal(char *str)
{
  perror(str);
  exit(1);
}

int makelist(char *s, const char *delimiters, char **list, int MAX_LIST)
{
  int i = 0;
  int numtokens = 0;
  char *snew = NULL;

  if ((s == NULL) || (delimiters == NULL))
    return -1;

  snew = s + strspn(s, delimiters); /* Skip delimiters */
  if ((list[numtokens] = strtok(snew, delimiters)) == NULL)
    return numtokens;

  numtokens = 1;

  while (1)
  {
    if ((list[numtokens] = strtok(NULL, delimiters)) == NULL)
      break;
    if (numtokens == (MAX_LIST - 1))
      return -1;
    numtokens++;
  }
  return numtokens;
}

int my_chdir(char *path)
{
  if (chdir(path) == -1)
  {
    perror("chdir error");
    return -1;
  }

  return 1;
}

int main(int argc, char **argv)
{
  int i = 0;
  pid_t pid;
  int cur_pipe=0;
  int status;
  int state;
  int pipefd[2];
  char* input_file;
  char* output_file;
  char* detect;
  char* read;
  bool last_pipe_flag = false;
  extern int errno;
  bool piping = false;
  char *cmdvector[MAX_CMD_ARG];
  char cmdline[BUFSIZ];
  bool bgflag = false;
  static struct sigaction act;
  static struct sigaction sig_act;
  memset(&act, 0, sizeof(struct sigaction));
  memset(&sig_act, 0, sizeof(struct sigaction));
  act.sa_handler = child_handler;
  sigemptyset(&act.sa_mask);

  sig_act.sa_handler = sig_handler;
  sigemptyset(&sig_act.sa_mask);
  signal(SIGQUIT, SIG_IGN);
  sigaction(SIGINT, &sig_act, 0);
  state = sigaction(SIGCHLD, &act, 0);
  if (state != 0)
  {
    printf("err");
  }
  while (1)
  {
    if(!piping){
      fputs(prompt, stdout);
      cmdline[0] = '\0';
      
      while (!fgets(cmdline, BUFSIZ, stdin) && !restart)
        ;
      if (restart)
      {
        restart = false;
      }
      cmdline[strlen(cmdline) - 1] = '\0';
			read = cmdline;
      if (feof(stdin))
      {
        printf("\n");
        exit(0);
      }
    }

    memset(cmdvector, 0, sizeof(char *) * MAX_CMD_ARG);

    if(detect = strstr(read,"|")){
        piping = true;
        detect[0] = '\0';
        i = makelist(read, " \t", cmdvector, MAX_CMD_ARG);
        read = detect + 1;
        last_pipe_flag = false;
        pipe(pipefd);
        if(!cur_pipe){
          cur_pipe= dup(STDIN_FILENO);
        }
    }else{
    i = makelist(read, " \t", cmdvector, MAX_CMD_ARG);
      if(piping){
        last_pipe_flag = true;
        piping = false;
      }
      
    }
    if (!cmdvector[0])
    {
      continue;
    }
    if (!strcmp(cmdvector[0], "cd"))
    {
      my_chdir(cmdvector[1]);
      continue;
    }
    //exit command
    if (!strcmp(cmdvector[0], "exit"))
    {
      exit(1);
    }

    if (!strcmp(cmdvector[i - 1], "&"))
    {
      cmdvector[i - 1] = '\0';
      bgflag = true;
      if (!strtok(cmdvector[i - 1], " \t"))
        cmdvector[i - 1] = NULL;
      i--;
    }
    else
    {
      bgflag = false;
    }


    switch (pid = fork())
    {
    case 0:
      signal(SIGQUIT, SIG_DFL);
      signal(SIGINT, SIG_DFL);

      int fd;
      int pivot = 0;
      if (bgflag)
      {
        setpgid(0, 0);
      }
      else
      {
        tcsetpgrp(STDIN_FILENO, getpgid(0));
      }
      for(int j =0; j<i;j++){
          if(strchr(cmdvector[j],'>')!=NULL){
            if(pivot == 0)
              pivot = j;
            output_file = cmdvector[++j];
            if((fd = open(output_file,O_WRONLY | O_CREAT | O_TRUNC))== -1){
              perror("open error: ");
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
          }else if(strchr(cmdvector[j],'<')!=NULL){
            if(pivot == 0)
              pivot = j;
            input_file = cmdvector[++j];
               if((fd = open(input_file,O_RDONLY))== -1){
              perror("open error: ");
            }
            dup2(fd,STDIN_FILENO);
            close(fd);
          }
      }
      if(pivot !=0){
        cmdvector[pivot] = NULL;
      }

      if(piping){
          close(pipefd[0]);
          dup2(pipefd[1],STDOUT_FILENO);
          close(pipefd[1]);
      }


      
      execvp(cmdvector[0], cmdvector);

    case -1:
      fatal("main()");
    default:

      if (!bgflag)
      {    	
        if (piping) {
					close(pipefd[1]);
					dup2(pipefd[0], STDIN_FILENO);
					close(pipefd[0]);
				}else{
        while (1)
        {
          if (waitpid(pid, NULL, 0) == -1)
          {
            if (errno == EINTR)
            {
              continue;
            }
            else
            {
              perror("wait error: ");
              exit(1);
            }
          }
          else
          {
            break;
          }
        }
        }
        if (last_pipe_flag) {
          while( waitpid(-1, 0, WNOHANG)>0);
					close(pipefd[0]);
					dup2(cur_pipe, STDIN_FILENO);
					close(cur_pipe);
					cur_pipe= 0;
					last_pipe_flag = false;
				}
        tcsetpgrp(STDIN_FILENO, getpgid(0));
      }
    }
  }
  return 0;
}

static void child_handler()
{
  pid_t child_pid;
  int state;
  while ((child_pid = waitpid(-1, 0, WNOHANG)) > 0)
  {
    printf("pid = %ld done \n", (long)child_pid);

     restart = true;
  }
}

static void sig_handler(int signum)
{
  printf("\n");
  restart = true;
}