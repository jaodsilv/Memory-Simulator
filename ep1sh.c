#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "headers/ep1sh.h"

/*ep1sh main loop.*/
int main(int argc, char **argv)
{
  int exit = 0;
  char wd[256];
  char *cmd = NULL;

  using_history();
  while(!exit) {
    if(getcwd(wd, sizeof(wd)) != NULL) {
      char sh[1024];
      strcat(strcat(strcpy(sh, "["), wd), "] ");

      if((cmd = get_cmd(cmd, sh)) == NULL) {
        printf("Expansion attempt has failed.\n");
        free(cmd); cmd = NULL;
        continue;
      }

      if(cmd_ls(cmd));
      else if(cmd_cd(cmd, wd));
      else if(cmd_show(cmd));
      else if(cmd_ep(cmd, wd));
      else if(cmd_exit(cmd)) exit = 1;
      else if(cmd_pwd(cmd, wd));
      else unrecognized(cmd);
      free(cmd); cmd = NULL;
    }
    else perror("Error (getcwd).\n");
  }
  free(cmd); cmd = NULL;
  return 0;
}

/*Runs the simulator using the "./ep1 <args>" command*/
int cmd_ep(char *cmd, char *wd)
{
  if(strncmp(cmd, "./ep1", 5) == 0) {
    char **args = NULL; int check = 0, i;

    for(i = 5; i < strlen(cmd) && check == 0; i++)
      if(cmd[i] != ' ' && cmd[i] != '\0')
        check = 1;

    if(strlen(cmd) < 6 || check == 0)
      printf("Expected arguments for ./ep1.\n");
    else if(check == 1 && ((args = get_args(cmd)) == NULL))
      printf("Bad arguments for ./ep1.\n");
    else if(check == 1 && run(args, wd) < 0);
    if(args != NULL) free_args(args);
    return 1;
  }
  return 0;
}

/*Get arguments for ./ep1 to send them correctly*/
char **get_args(char *cmd)
{
  char **args;
  int arg = 0, i, j;

  args = malloc(4 * sizeof(*args));
  args[0] = malloc(2 * sizeof(**args));
  args[1] = malloc(64 * sizeof(**args));
  args[2] = malloc(64 * sizeof(**args));
  args[3] = NULL;

  /*Get 1st argument*/
  for(i = 5; isspace(cmd[i]); i++) continue;
  for(j = 0; i < strlen(cmd) && !isspace(cmd[i]); i++) {
    if(isdigit(cmd[i]) && j > 0) { arg = -1; break; }
    else if(isdigit(cmd[i]) && j == 0 && cmd[i] >= '1' && cmd[i] <= '6') {
      args[0][j] = cmd[i]; args[0][j + 1] = '\0'; arg++;
      j++;
    }
  }
  /*Get 2nd argument*/
  if(arg == 1) {
    while(isspace(cmd[i])) { i++; continue; }
    if(cmd[i] != '\0') {
      for(j = 0; i < strlen(cmd) && !isspace(cmd[i]) && j < 64; i++, j++)
        args[1][j] = cmd[i];
      if(j < 64) { args[1][j] = '\0'; arg++; }
      else arg = -1;
    }
    else arg = -1;
  }
  /*Get 3rd argument*/
  if(arg == 2) {
    while(isspace(cmd[i])) { i++; continue; }
    if(cmd[i] != '\0') {
      for(j = 0; i < strlen(cmd) && !isspace(cmd[i]) && j < 64; i++, j++)
        args[2][j] = cmd[i];
      if(j < 64) { args[2][j] = '\0'; arg++; }
      else arg = -1;
    }
    else arg = -1;
  }
  /*Get the optional 4th argument*/
  if(arg == 3) {
    while(isspace(cmd[i])) { i++; continue; }
    if(cmd[i] == 'd' && (cmd[i + 1] == '\0' || isspace(cmd[i + 1]))) {
      args[3] = malloc(2 * sizeof(**args));
      args[3][0] = 'd'; args[3][1] = '\0'; arg++;
    }
  }

  if(arg == 3 || arg == 4) return args;
  else { free_args(args); return NULL; }
}

/*Frees the memory allocated by ep1 arguments*/
void free_args(char **args)
{
  int i;
  for(i = 0; i < 3; i++) {
    free(args[i]); args[i] = NULL;
  }
  free(args); args = NULL;
}

/*Shows commands in history list*/
int cmd_show(char *cmd)
{
  if(strncmp(cmd, "show", 4) == 0) {
    int fine = 1, i;
    HIST_ENTRY **hlist;

    for(i = 4; i < strlen(cmd); i++)
      if(!isspace(cmd[i]))
        fine = 0;

    if(fine) {
      hlist = history_list();
      if(hlist)
        for(i = 0; hlist[i] != NULL; i++)
          printf("%d: %s\n", i + history_base, hlist[i]->line);
    }
    else printf("Expected no argument for 'show'.\n");
    return 1;
  }
  return 0;
}

/*Terminates ep1sh session*/
int cmd_exit(char *cmd)
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
int cmd_cd(char *cmd, char *wd)
{
  if(strncmp(cmd, "cd", 2) == 0 && strlen(cmd) > 2) {
    char *cdarg, *path;

    cdarg = malloc((strlen(cmd) - 2) * sizeof(*cdarg));
    filter(cmd, 3, cdarg);

    if(cdarg[0] != '/') {
      path = malloc((strlen(wd) + strlen(cdarg) + 2) * sizeof(*path));
      strcat(strcat(strcpy(path, wd), "/"), cdarg);
      if(cdarg[0] == '\0' || chdir(path) != 0)
        printf("Bad argument for 'cd'.\n");
    }
    else {
      path = malloc((strlen(cmd) - 2) * sizeof(*path));
      filter(cmd, 3, path);
      if(path[0] == '\0' || chdir(path) != 0)
        printf("Bad argument for 'cd'.\n");
    }
    free(cdarg); cdarg = NULL;
    free(path); path = NULL;

    return 1;
  }
  return 0;
}

/*Check if user invoked '/bin/ls -l' binary to ep1sh*/
int cmd_ls(char *cmd)
{
  if(strncmp(cmd, "/bin/ls ", 8) == 0) {
    int i;
    for(i = 8; i < strlen(cmd); i++) {
      if(cmd[i] == '-' && cmd[i + 1] == 'l') break;
      else if(cmd[i] == ' ') continue;
      else return 0;
    }
    if(fork() != 0) {
      int *status = NULL;
      waitpid(-1, status, 0);
      return 1;
    }
    else {
      char *argument[] = {"/bin/ls", "-l", NULL};
      execve("/bin/ls", argument, NULL);
    }
  }
  return 0;
}

/*Get and print the working directory*/
int cmd_pwd(char *cmd, char *wd)
{
  if(strncmp(cmd, "pwd", 3) == 0) {
    printf("%s\n", wd);
    return 1;
  }
  return 0;
}

/*Get user command*/
char *get_cmd(char *cmd, char *wd)
{
  cmd = readline(wd);
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

/*Filter command 'cmd' arguments 'arg',
  starting from index 'begin', eliminating unnecessary spaces*/
void filter(char *cmd, int begin, char *arg)
{
  int i, spaces = 0;
  for(i = begin; i < strlen(cmd); i++) {
    if(i == begin) while(isspace(cmd[i])) { i++; spaces++;}
    if(!isalnum(cmd[i])) {
      if(begin == 3 && cmd[i] == '.' && cmd[i + 1] == '.' &&
        (cmd[i + 2] == '\0' || cmd[i + 2] == ' ')) {
          arg[i - begin - spaces] = cmd[i]; i++;
          arg[i - begin - spaces] = cmd[i];
          continue;
        }
      else if(begin == 3 && cmd[i] == '/') {
        arg[i - begin - spaces] = cmd[i];
        continue;
      }
      break;
    }
    arg[i - begin - spaces] = cmd[i];
  }
  arg[i - begin - spaces] = '\0';
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

4) Não está sendo considerado que haverão diretórios e arquivos com espaços no
nome. Os nomes também devem ser inteiramente alfanuméricos.

5) A função "readline" está causando vazamento de memória. Não foi encontrado
nem na documentação e nem no google uma solução. Os ponteiros para a memória
que deve ser desalocada, porém, são marcados como "ainda alcançáveis" no
valgrind. Com uma query certa no google é fácil de encontrar outros usuários
que tiveram este problema.

6) Para usar o ep1, os arquivos de trace devem, obrigatoriamente, estarem no
diretório inputs/

7) Os arquivos de trace (input) precisam estar com cada item separado de apenas
UM espaço (' ') e precisa ter uma linha extra vazia no final.
*/
