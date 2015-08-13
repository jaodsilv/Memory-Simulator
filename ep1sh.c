#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <readline/history.h>

/*Prototypes*/
void removenl(char *);
void filter(char *, int, char *);
int getcmd(char *);
int expand(char *);
int cmdls(char *);
int cmdcd(char *, char *);
int cmdexit(char *);
int cmdsave(char *);
int cmdload(char *);
int cmdshow(char *);
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
      else if(cmdload(cmd));
      else if(cmdshow(cmd));
      else unrecognized(cmd);
    }
    else perror("Error (getcwd).\n");
  }
  return 0;
}

/*Shows commands in history list*/
int cmdshow(char *cmd)
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
        for(i = 0; hlist[i]; i++)
          printf("%d: %s\n", i + history_base, hlist[i]->line);
    }
    else printf("Expected no argument for 'show'.\n");
    return 1;
  }
  return 0;
}

/*Loads history of a file named 'fname'*/
int cmdload(char *cmd)
{
  if(strncmp(cmd, "load", 4) == 0) {
    char *fname;
    fname = malloc((strlen(cmd) - 2) * sizeof(*fname));

    filter(cmd, 4, fname);

    if(isalnum(fname[0]))
      read_history(fname);
    else
      printf("Expected alphanumeric file name argument for 'load'.\n");

    free(fname); fname = NULL;
    return 1;
  }
  return 0;
}

/*Saves history in a file named 'fname'*/
int cmdsave(char *cmd)
{
  if(strncmp(cmd, "save", 4) == 0) {
    char *fname;
    fname = malloc((strlen(cmd) - 2) * sizeof(*fname));

    filter(cmd, 4, fname);

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
  if(strncmp(cmd, "cd", 2) == 0 && strlen(cmd) > 2) {
    char *cdarg, *path;

    cdarg = malloc((strlen(cmd) - 2) * sizeof(*cdarg));
    filter(cmd, 3, cdarg);

    if(cdarg[0] != '/') {
      path = malloc((strlen(wd) + strlen(cdarg) + 2) * sizeof(*path));
      strcpy(path, wd); strcat(path, "/"); strcat(path, cdarg);
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


*/
