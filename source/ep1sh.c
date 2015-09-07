#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "../headers/ep1sh.h"

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
      else if(cmd_ep(cmd));
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

/*Calls the simulator binary using the "./ep1 <args>" command*/
int cmd_ep(char *cmd)
{
  if(strncmp(cmd, "./ep1", 5) == 0) {
    char number[8], input[256], output[256], optional[4];
    int count = 0, i, j = 0;

    number[0] = '\0'; input[0] = '\0'; output[0] = '\0';
    optional[0] = '\0'; optional[1] = '\0';
    for(i = 5; i < strlen(cmd) && count < 4; i++) {
        if(isspace(cmd[i])) continue;
        while(!isspace(cmd[i]) && cmd[i] != '\0') {
          if(count == 0) {
            number[j++] = cmd[i++];
            number[j] = '\0';
            if(j == 7 || i >= strlen(cmd)) break;
          }
          else if(count == 1) {
            input[j++] = cmd[i++];
            input[j] = '\0';
            if(j == 255 || i >= strlen(cmd)) break;
          }
          else if(count == 2) {
            output[j++] = cmd[i++];
            output[j] = '\0';
            if(j == 255 || i >= strlen(cmd)) break;
          }
          else /*count == 3 */ {
            optional[j++] = cmd[i++];
            if(cmd[i] != '\0') optional[j++] = cmd[i];
            optional[j] = '\0';
            break;
          }
        }
        j = 0; count++;
    }

    if(fork() != 0) {
      int status;
      waitpid(-1, &status, 0);
      return 1;
    }
    else {
      char *arguments[] = {"./ep1", number, input, output, optional, NULL};
      execve("./ep1", arguments, NULL);
    }
  }
  return 0;
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
    filter(cmd, cdarg);

    if(cdarg[0] != '/') {
      path = malloc((strlen(wd) + strlen(cdarg) + 2) * sizeof(*path));
      strcat(strcat(strcpy(path, wd), "/"), cdarg);
      if(cdarg[0] == '\0' || chdir(path) != 0)
        printf("Bad argument for 'cd'.\n");
    }
    else {
      path = malloc((strlen(cmd) - 2) * sizeof(*path));
      filter(cmd, path);
      if(path[0] == '\0' || chdir(path) != 0)
        printf("Bad argument for 'cd'.\n");
    }
    free(cdarg); cdarg = NULL;
    free(path); path = NULL;

    return 1;
  }
  return 0;
}

/*Check if user invoked '/bin/ls -1' binary to ep1sh*/
int cmd_ls(char *cmd)
{
  if(strncmp(cmd, "/bin/ls ", 8) == 0) {
    int i;
    for(i = 8; i < strlen(cmd); i++) {
      if(cmd[i] == '-' && cmd[i + 1] == '1') break;
      else if(cmd[i] == ' ') continue;
      else return 0;
    }
    if(fork() != 0) {
      int *status = NULL;
      waitpid(-1, status, 0);
      return 1;
    }
    else {
      char *arguments[] = {"/bin/ls", "-1", NULL};
      execve("/bin/ls", arguments, NULL);
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
  starting from index '3', eliminating unnecessary spaces*/
void filter(char *cmd, char *arg)
{
  int i, spaces = 0;
  i = 3;
  while(isspace(cmd[i])) { i++; spaces++;}
  for(; i < strlen(cmd); i++) {
    if(!isalnum(cmd[i])) {
      if(cmd[i] == '.' && cmd[i + 1] == '.' &&
        (cmd[i + 2] == '\0' || cmd[i + 2] == ' ')) {
          arg[i - 3 - spaces] = cmd[i]; i++;
          arg[i - 3 - spaces] = cmd[i];
          continue;
        }
      else if(cmd[i] == '\\' && cmd[i+1] == ' ') {
        arg[i - 3 - spaces] = ' ';
        i++; spaces++;
        continue;
      }
      else if(cmd[i] == '\\' || cmd[i] == '\0') {
        break;
      } else if(cmd[i] == ' ') {
        arg[i - 3 - spaces] = '\0';
        return;
      }
    }
    arg[i - 3 - spaces] = cmd[i];
  }
  arg[i - 3 - spaces] = '\0';
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

4) Os únicos caracteres proibidos para nomes de diretórios na maioria dos
sistemas UNIX são '\' e '\0'. Para espaços o comportamento é o mesmo do 'cd'
do linux, usa-se '\ '.

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
