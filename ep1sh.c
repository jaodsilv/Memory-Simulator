#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void removenl(char *);

int main(int argc, char **argv)
{
  char wd[256];
  char cmd[16];

  while(1){
    if(getcwd(wd, sizeof(wd)) != NULL) {
      fflush(stdin);
      printf("[%s] ", wd);
      fgets(cmd, 15, stdin);
      removenl(cmd);
      if(strcmp(cmd, "/bin/ls -l") == 0) {
        if(system("/bin/ls -l") != 0){
          printf("Command not supported.\n");
          return 1;
        }
      }
      else if(strncmp(cmd, "cd ", 3) == 0) {
        char *cdarg, *path; int i;

        cdarg = malloc((strlen(cmd) - 2) * sizeof(*cdarg));

        for(i = 3; i < strlen(cmd); i++) cdarg[i - 3] = cmd[i];
        cdarg[i] = '\0';

        path = malloc((strlen(wd) + strlen(cdarg) + 2) * sizeof(*path));
        strcpy(path, wd); strcat(path, "/"); strcat(path, cdarg);
        if(chdir(path) != 0)
          printf("Error changing directory. Make sure directory exists.\n");

        free(cdarg); cdarg = NULL;
      }
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
