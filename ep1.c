#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ep1.h"

int run(char **argv, char *wd)
{
  unsigned int all = 0, *total = &all;
  Process *process;
  process = malloc(100 * sizeof(*process));

  printf("Run argument 2 = %s\n", argv[1]);
  printf("Run argument 3 = %s\n", argv[2]);
  printf("Run argument 4 = %s\n", argv[3]);

  process = readtfile(process, wd, argv[1], total);
  /*Uncomment the code below and declare an 'unsigned int i' to see stored data*/
  /*for(i = 0; i < *total; i++) {
    printf("Process %d:\n\tArrival = %f\n\tname = %s\n\tDuration = %f\n\tDeadline = %f\n\tPriority = %d\n\n", i+1, process[i].arrival, process[i].name, process[i].duration, process[i].deadline, process[i].priority);
  }*/
  if(process != NULL) {
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
      free(process); process = NULL;
    }
  }
  else printf("\nError reading file. Make sure it is written in expected format.\n");
  return 0;
}

/*Read the trace file 'tfile' and store its content in the array 'process',
  creating process "objects"*/
Process *readtfile(Process *process, char *wd, char *tfile, unsigned int *total)
{
  FILE *fptr;
  char input[256];

  strcat(strcat(strcpy(input, wd), "/inputs/"), tfile);
  if((fptr = fopen(input, "r")) != NULL) {
    char c, tmp[64];
    unsigned int i = 0, j = 0, size = 100, item = 1, dots = 0, done = 0;

    while(!done) {
      if((c = fgetc(fptr)) == EOF) { done = 1; continue; }

      /*Get Arrival*/
      if(item  == 1) {
        if(isdigit(c) || c == '.') {
          tmp[i++] = c;
          if(c == '.') dots++;
          if(dots > 1) { free(process); return NULL; }
        }
        else if(isblank(c) && i > 0) {
          tmp[i] = '\0'; i = 0; item = 2; dots = 0;
          process[j].arrival = atof(tmp);
          continue;
        }
        else { free(process); return NULL; }
      }

      /*Get name*/
      if(item  == 2) {
        if(!isspace(c)) tmp[i++] = c;
        else if(isblank(c) && i > 0) {
          tmp[i] = '\0'; i = 0; item = 3;
          strcpy(process[j].name, tmp);
          continue;
        }
        else { free(process); return NULL; }
      }

      /*Get duration*/
      if(item  == 3) {
        if(isdigit(c) || c == '.') {
          tmp[i++] = c;
          if(c == '.') dots++;
          if(dots > 1) { free(process); return NULL; }
        }
        else if(isblank(c) && i > 0) {
          tmp[i] = '\0'; i = 0; item = 4; dots = 0;
          process[j].duration = atof(tmp);
          continue;
        }
        else { free(process); return NULL; }
      }

      /*Get deadline*/
      if(item  == 4) {
        if(isdigit(c) || c == '.') {
          tmp[i++] = c;
          if(c == '.') dots++;
          if(dots > 1) { free(process); return NULL; }
        }
        else if(isblank(c) && i > 0) {
          tmp[i] = '\0'; i = 0; item = 5; dots = 0;
          process[j].deadline = atof(tmp);
          continue;
        }
        else { free(process); return NULL; }
      }

      /*Get Priority*/
      if(item  == 5) {
        if(isdigit(c) || c == '-') {
          tmp[i++] = c;
          if(c == '-') dots++;
          if(dots > 1) { free(process); return NULL; }
        }
        else if(isspace(c) && i > 0) {
          tmp[i] = '\0'; i = 0; item = 1; dots = 0; *total += 1;
          process[j++].priority = atoi(tmp);
          if(j == size / 2) {
            process = realloc(process, (size * 2) * sizeof(*process));
            size *= 2;
          }
        }
        else { free(process); return NULL; }
      }
    }
    return process = realloc(process, j * sizeof(*process));
  }
  else {
    printf("EP1: error opening requested file '%s'.\n", input);
    return 0;
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

int isblank(char c)
{
  return c == ' ' || c == '\t';
}
