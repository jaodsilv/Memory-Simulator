#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ep1.h"

int run(char **argv, char *wd)
{
  char **buff = NULL;
  unsigned int bsize = 100, *bsizeptr = &bsize;

  printf("Run argument 2 = %s\n", argv[1]);
  printf("Run argument 3 = %s\n", argv[2]);

  buff = readtfile(buff, bsizeptr, wd, argv[1]);

  if(buff != NULL) {
    unsigned int scheduler;
    switch(scheduler = atoi(argv[0])) {
      case FCFS:
        /*do FCFS*/
        printf("1. FCFS\n");
        break;
      case SJF:
        /*do SJF*/
        printf("2. SJF\n");
        break;
      case SRTN:
        /*do SRTN*/
        printf("3. SRTN\n");
        break;
      case RR:
        /*do RR*/
        printf("4. RR\n");
        break;
      case PS:
        /*do PS*/
        printf("5. PS\n");
        break;
      case RRTS:
        /*do RRTS*/
        printf("6. RRTS\n");
        break;
      freebuff(buff, bsizeptr);
    }
  }
  return 0;
}

/*Read the trace file 'tfile' and store its content in the buffer 'buff'*/
char **readtfile(char **buff, unsigned int *bsizeptr, char *wd, char *tfile)
{
  FILE *fptr;
  char input[256];

  strcat(strcat(strcpy(input, wd), "/inputs/"), tfile);
  if((fptr = fopen(input, "r")) != NULL) {
    char str[128]; unsigned int i = 0;

    buff = malloc(*bsizeptr * sizeof(*buff));
    while(fgets(str, 128, fptr) != NULL) {
      buff[i] = malloc(128 * sizeof(**buff));
      removenl(str);
      strcpy(buff[i], str);
      if(++i == (*bsizeptr / 2)) {
        buff = realloc(buff, *bsizeptr * 2 * sizeof(*buff));
        *bsizeptr *= 2;
      }
    }
    while(i < *bsizeptr) buff[i++] = NULL;
    fclose(fptr);
    return buff;
  }
  else {
    printf("EP1: error opening requested file '%s'.\n", input);
    return NULL;
  }
}

/*Remove final character \n from string 'str'*/
void removenl(char *str)
{
  if((strlen(str) > 0) && (str[strlen(str) - 1] == '\n'))
    str[strlen(str) - 1] = '\0';
}


 /*Frees buffer memory*/
void freebuff(char **buff, unsigned int *bsizeptr)
{
  unsigned int i;
  for(i = 0; i < *bsizeptr; i++) {
    free(buff[i]); buff[i] = NULL;
  }
  free(buff); buff = NULL;
}
