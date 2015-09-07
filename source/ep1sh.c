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
      char *arguments[] = {"/bin/ls", "-l", NULL};
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
