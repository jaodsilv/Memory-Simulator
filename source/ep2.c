#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "../headers/ep2.h"

char *get_cmd(char *);
int expand(char *);
int cmd_load(char *, char *);
int cmd_exit(char *);
char *get_arg(char *, char *, char *);
void unrecognized(char *);
void free_pointer(void *);

int main(int argc, char **argv)
{
  char *cmd = NULL, *arg = NULL;

  using_history();
  while(True) {

    if((cmd = get_cmd(cmd)) == NULL) {
      printf("Expansion attempt has failed.\n");
      free(cmd); cmd = NULL;
      continue;
    }

    if(cmd_load(cmd, arg));
    /*else if(cmd_space(cmd));
    else if(cmd_subst(cmd));
    else if(cmd_exec(cmd));*/
    else if(cmd_exit(cmd)) break;
    else unrecognized(cmd);

    free_pointer((void*)cmd);
    free_pointer((void*)arg);
  }
  free_pointer((void*)cmd);
  free_pointer((void*)arg);
  return 0;
}

/*Get user command*/
char *get_cmd(char *cmd)
{
  cmd = readline("[EP2]: ");
  return (expand(cmd) != -1) ? cmd : NULL;
}

/*Expand the history or a previous command*/
int expand(char *cmd)
{
  char *expansion;
  int result;

  result = history_expand(cmd, &expansion);

  if(result == 0 || result == 1) {
    add_history(expansion);
    strncpy(cmd, expansion, strlen(cmd) - 1);
  }
  free(expansion); expansion = NULL;
  return result;
}

/*Execute 'load' command*/
int cmd_load(char *cmd, char *arg)
{
  if(strncmp(cmd, "carrega", 7) == 0) {
    if((arg = get_arg(cmd, arg, "carrega")) == NULL)
      printf("Bad argument for 'carrega'.\n");
    else {
      char wd[1024];
      /*Relative path*/
      if(arg[0] != '/') {
        if(getcwd(wd, sizeof(wd)) == NULL)
          printf("Error while retrieving working directory.\n");
        strcat(strcat(wd, "/"), arg);
      }
      /*Absolute Path*/
      else strcpy(wd, arg);
      /******************************/
      /*Load file. Its name is in wd*/
      /*TODO: do_load_file_here*/
      /******************************/
    }
    return 1;
  }
  return 0;
}

/*Terminate session*/
int cmd_exit(char *cmd)
{
  if(strcmp(cmd, "sai") == 0 && strlen(cmd) == 3) return 1;
  return 0;
}

/*User invoked unknown command to ep1sh*/
void unrecognized(char *cmd)
{
  printf("Unrecognized command \"%s\".\n", cmd);
}

/*Get the argument for requested command 'rqst'*/
char *get_arg(char *cmd, char *arg, char *rqst)
{
  int i, j = 0;

  arg = malloc(strlen(cmd) * sizeof(*arg));
  for(i = strlen(rqst); isspace(cmd[i]); i++) continue;
  if(cmd[i] == '\0') return NULL;
  while(!isspace(cmd[i]) && cmd[i] != '\0') arg[j++] = cmd[i++];
  arg[j] = '\0';
  return arg;
}

/*Generic pointer disallocation function*/
void free_pointer(void *pointer)
{
  if(pointer != NULL) free(pointer);
  pointer = NULL;
}





/*
Notas:
- Mudado para C pq não vejo necessidade de usar OOP
- TODO: load file

*/
