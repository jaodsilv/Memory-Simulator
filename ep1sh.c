#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/history.h>

void removenl(char *);

int main(int argc, char **argv)
{
  char wd[256];
  char cmd[256];

  while(1){
    if(getcwd(wd, sizeof(wd)) != NULL) {
      fflush(stdin);
      printf("[%s] ", wd);
      fgets(cmd, 255, stdin);
      removenl(cmd);
      if(strcmp(cmd, "/bin/ls -l") == 0) {
        if(system("/bin/ls -l") != 0){
          printf("Command not supported.\n");
          return 1;
        }
      }
      else if(strncmp(cmd, "cd ", 3) == 0 && strlen(cmd) > 3) {
        char *path; int i;

        if(cmd[3] != '/') {
          char *cdarg;
          cdarg = malloc((strlen(cmd) - 2) * sizeof(*cdarg));

          for(i = 3; i < strlen(cmd); i++) cdarg[i - 3] = cmd[i];
          cdarg[i] = '\0';

          path = malloc((strlen(wd) + strlen(cdarg) + 2) * sizeof(*path));
          strcpy(path, wd); strcat(path, "/"); strcat(path, cdarg);

          if(chdir(path) != 0) printf("Bad argument for 'cd'.\n");

          free(cdarg); cdarg = NULL;
        }
        else {
          path = malloc((strlen(cmd) - 2) * sizeof(*path));

          for(i = 3; i < strlen(cmd); i++) path[i - 3] = cmd[i];
          path[i] = '\0';

          if(chdir(path) != 0) printf("Bad argument for 'cd'.\n");
        }
        free(path); path = NULL;
      }
      else if(strncmp(cmd, "cd", 2) == 0) printf("Bad argument for 'cd'.\n");
      else printf("Unrecognized command \"%s\".\n", cmd);
    }
    else perror("Error (getcwd).\n");
  }
  return 0;

}

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


*/
