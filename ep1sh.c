#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <readline/history.h>

/*Prototypes*/
void removenl(char *);
int getcmd(char *);
int expand(char *);
int cmdls(char *);
int cmdcd(char *, char *);
int cmdexit(char *);
int cmdsave(char *);
void unrecognized(char *);
/*************/

/*ep1sh main loop.*/
int main(int argc, char **argv)
{
  int exit = 0;
  char wd[256];
  char cmd[256];

  using_history();
  while(!exit){
    if(getcwd(wd, sizeof(wd)) != NULL) {
      int result;
      printf("[%s] ", wd);

      if((result = getcmd(cmd)) == -1)
        printf("Expansion attempt has failed.\n");


      if(cmdls(cmd));
      else if(cmdcd(wd, cmd));
      else if(cmdexit(cmd)) exit = 1;
      else if(cmdsave(cmd));
      else unrecognized(cmd);

    }
    else perror("Error (getcwd).\n");
  }
  return 0;
}

/*Saves history in a file named 'fname'*/
int cmdsave(char *cmd)
{
  if(strncmp(cmd, "save", 4) == 0) {
    char *fname; int i, spaces = 0;
    fname = malloc((strlen(cmd) - 2) * sizeof(*fname));

    for(i = 4; i < strlen(cmd); i++) {
      if(i == 4) while(isspace(cmd[i])) { i++; spaces++;}
      else if(!isalnum(cmd[i])) break;
      fname[i - 4 - spaces] = cmd[i];
    }
    fname[i - 4 - spaces] = '\0';

    if(isalnum(fname[0]))
      write_history(fname);
    else
      printf("Expected alphanumeric file name argument for 'save'.\n");

    free(fname); fname = NULL;
    return 1;
  }
  return 0;
}

/*Terminates ep1sh session*/
int cmdexit(char *cmd)
{
  if(strcmp(cmd, "exit") == 0 && strlen(cmd) == 4) return 1;
  return 0;
}

/*User invoked unknown command to ep1sh*/
void unrecognized(char *cmd)
{
  printf("Unrecognized command \"%s\".\n", cmd);
}

/*Check if user invoked cd command to ep1sh*/
int cmdcd(char *wd, char *cmd)
{
  if(strncmp(cmd, "cd", 2) == 0) {
    char *path; int i;
    if(strlen(cmd) > 2 && cmd[3] != '/') {
      char *cdarg; int spaces = 0;
      cdarg = malloc((strlen(cmd) - 2) * sizeof(*cdarg));

      for(i = 3; i < strlen(cmd); i++) {
        if(i == 3) while(isspace(cmd[i])) { i++; spaces++;}
        else if(isspace(cmd[i])) break;
        cdarg[i - 3 - spaces] = cmd[i];
      }
      cdarg[i - 3 - spaces] = '\0';

      path = malloc((strlen(wd) + strlen(cdarg) + 2) * sizeof(*path));
      strcpy(path, wd); strcat(path, "/"); strcat(path, cdarg);

      if(cdarg[0] == '\0' || chdir(path) != 0)
        printf("Bad argument for 'cd'.\n");

      free(cdarg); cdarg = NULL;
    }
    else {
      path = malloc((strlen(cmd) - 2) * sizeof(*path));

      for(i = 3; i < strlen(cmd); i++) path[i - 3] = cmd[i];
      path[i] = '\0';

      if(chdir(path) != 0) printf("Bad argument for 'cd'.\n");
    }
    free(path); path = NULL;
    return 1;
  }
  return 0;
}

/*Check if user invoked /bin/ls -l command to ep1sh*/
int cmdls(char *cmd)
{
  if(strcmp(cmd, "/bin/ls -l") == 0) {
    if(system("/bin/ls -l") != 0) printf("Command not supported.\n");
    return 1;
  }
  return 0;
}

/*Get user command*/
int getcmd(char *cmd)
{
  fflush(stdin);
  fgets(cmd, 255, stdin);
  removenl(cmd);
  return expand(cmd);
}

/*Expand the history or a previous command*/
int expand(char *cmd)
{
  char *expansion;
  int result;

  result = history_expand(cmd, &expansion);

  if(result == 0 || result == 1) {
    add_history(expansion);
    strncpy(cmd, expansion, sizeof(cmd) - 1);
  }
  free(expansion); expansion = NULL;

  return result;
}

/*Remove final character \n from user command*/
void removenl(char *str)
{
  if ((strlen(str) > 0) && (str[strlen(str) - 1] == '\n'))
        str[strlen(str) - 1] = '\0';
}

/*
Notes:

1) Certifique-se de que o pacote libreadline-dev foi instlado. Isso pode ser
feito, por exemplo, atraves do comando aptitude install libreadline-dev

2) cd e pwd estão implementados de forma parecida com o seguinte link:
https://cnswww.cns.cwru.edu/php/chet/readline/readline.html
com cada comando usando chdir() e getcwd(), respectivamente.

3) função expand foi baseada no código da página da biblioteca GNU History
e adaptada para o nosso uso.

*/
