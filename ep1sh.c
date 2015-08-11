#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void removenl(char *);

int main(int argc, char **argv)
{
  char cmd[16];

  /*funções da família "exec" quebram o loop. Soluções:
  1 - usar "system"
  2 - fazer um fork e depois usar as funções exec

  Abaixo, a opção 1
  */

  while(1){
    fflush(stdin);
    printf("$ ");
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
  return 0;
}

void removenl(char *str)
{
  if ((strlen(str) > 0) && (str[strlen(str) - 1] == '\n'))
        str[strlen(str) - 1] = '\0';
}
