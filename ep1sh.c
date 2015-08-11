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
      else printf("Unrecognized command \"%s\".\n", cmd);
    }
    else perror("Error (getcwd)");
  }
  return 0;

}

void removenl(char *str)
{
  if ((strlen(str) > 0) && (str[strlen(str) - 1] == '\n'))
        str[strlen(str) - 1] = '\0';
}
