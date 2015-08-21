#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "ep1.h"

int run(char **argv, char *wd)
{
  unsigned int all = 0, *total = &all;
  Process *process;
  process = malloc(100 * sizeof(*process));

  printf("Run argument 2 = %s\n", argv[1]);
  printf("Run argument 3 = %s\n", argv[2]);
  printf("Run argument 4 = %s\n", argv[3]);

  process = readtfile(process, wd, argv[1], total, argv[3]);
  /*Uncomment the code below and declare an 'unsigned int i' to see stored data*/
  /*for(i = 0; i < *total; i++) {
    printf("Process %d:\n\tArrival = %f\n\tname = %s\n\tDuration = %f\n\tDeadline = %f\n\tPriority = %d\n\n", i+1, process[i].arrival, process[i].name, process[i].duration, process[i].deadline, process[i].priority);
  }*/
  if(process != NULL) {
    unsigned int scheduler;
    pthread_t *threads;

    threads = malloc((*total + 1) * sizeof(*threads));

    switch(scheduler = atoi(argv[0])) {
      case FCFS:
        /*do FCFS*/
        printf("1. FCFS\n");
        break;
      case SJF:
        /*do SJF*/
        printf("2. SJF\n");
        initiate_sjf(threads, process, total);
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
      free(threads); threads = NULL;
      free(process); process = NULL;
    }
  }
  else {
    printf("\nError reading file. Make sure it is written in expected ");
    printf("format and you are executing ep1 command from the right folder.\n");
  }
  return 0;
}

/*Read the trace file 'tfile' and store its content in the array 'process',
  creating process "objects"*/
Process *readtfile(Process *process, char *wd, char *tfile, unsigned int *total, char *paramd)
{
  FILE *fptr;
  char input[256];

  strcat(strcat(strcpy(input, wd), "/inputs/"), tfile);
  if((fptr = fopen(input, "r")) != NULL) {
    char c, tmp[64];
    unsigned int i = 0, j = 0, size = 100, item = 1, dots = 0, done = 0;

    while(!done) {
      if((c = fgetc(fptr)) == EOF) { done = 1; continue; }
      switch (item) {
        case 1: /*Get Arrival*/
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
          break;

        case 2: /*Get name*/
          if(!isspace(c)) tmp[i++] = c;
          else if(isblank(c) && i > 0) {
            tmp[i] = '\0'; i = 0; item = 3;
            strcpy(process[j].name, tmp);
            continue;
          }
          else { free(process); return NULL; }
          break;

        case 3: /*Get duration*/
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
          break;

        case 4: /*Get deadline*/
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
          break;

        case 5: /*Get Priority*/
          if(isdigit(c) || c == '-') {
            tmp[i++] = c;
            if(c == '-') dots++;
            if(dots > 1) { free(process); return NULL; }
          }
          else if(isspace(c) && i > 0) {
            tmp[i] = '\0'; i = 0; item = 1; dots = 0; *total += 1;
            process[j].priority = atoi(tmp);
            process[j].arrived = False;
            process[j].done = False;
            process[j].working = False;
            process[j++].coordinator = False;
            if(j == size / 2) {
              process = realloc(process, (size * 2) * sizeof(*process));
              size *= 2;
            }
          }
        else { free(process); return NULL; }
        break;
      }
    }
    process[j].coordinator = True;
    process[j].total = *total;
    if(paramd != NULL) process[j].paramd = True;
    else process[j].paramd = False;
    process[j++].process = process;
    return process = realloc(process, j * sizeof(*process));
  }
  else {
    printf("EP1: error opening requested file '%s'.\n", input);
    return 0;
  }
}

/*Checks if the character is a space or tab*/
int isblank(char c)
{
  return c == ' ' || c == '\t';
}

/*Assigns a process to a core*/
void use_core(Process *process, Core *core, unsigned int cores)
{
  unsigned int i = 0;
  while(i < cores) {
    if(core[i].available) {
      core[i].available = False;
      core[i].process = process;
      core[i].process->working = True;
      printf("Process '%s' assigned to core %d\n", core[i].process->name, i+1);
      break;
    }
    else i++;
  }
}

/*Shortest Job First*/
void *sjf(void *args)
{
  Process *process = ((Process*) args);

	if(process->coordinator) {
    /*printf("Coordinator: Total = %u  Coord? %d  process0 = %s paramd? %d\n", process->total, process->coordinator, process->process[0].name, process->paramd);*/
    unsigned int cores, available_cores, count;
    clock_t start = clock();
    Core *core;

    cores = available_cores = sysconf(_SC_NPROCESSORS_ONLN);
    core = malloc(cores * sizeof(*core));
    for(count = 0; count < cores; count++) {
      core[count].available = True;
      core[count].process = NULL;
    }

    count = 0;
    while(count != process->total) {
      Process *next = NULL;

      fetchprocess(process->process, process->total, start);
      next = select_sjf(process->process, process->total, start);
      if(next != NULL && available_cores > 0) {
        use_core(next, core, cores); available_cores--;

        count++;
        next->working = True;
      }
    }

    free(core); core = NULL;
  }
	else {
    /*printf("%s: Arrival %f, Duration %f, Deadline %f, Priority %d Coord? %d\n", process->name, process->arrival, process->duration, process->deadline, process->priority, process->coordinator);*/


  }

	return NULL;
}

/*Initiate threads to run SJF scheduling*/
void initiate_sjf(pthread_t *threads, Process *process, unsigned int *total)
{
  unsigned int i;
  for(i = 0; i <= *total; i++) {
    if(pthread_create(&threads[i], NULL, sjf, &process[i])) {
      printf("Error creating thread.\n");
      return;
    }
  }
  for(i = 0; i <= *total; i++) {
    if(pthread_join(threads[i], NULL)) {
      printf("Error joining process thread.\n");
      return;
    }
  }
}

/*Look up for new processes*/
void fetchprocess(Process *process, unsigned int total, clock_t start)
{
  unsigned int i;
  float sec = ((float)(clock() - start)) / CLOCKS_PER_SEC;

  for(i = 0; i < total; i++)
    if(sec >= process[i].arrival && !process[i].arrived) {
      process[i].arrived = True;
      printf("%s has arrived\n", process[i].name);
    }
}

/*Selects the shortest process */
Process *select_sjf(Process *process, unsigned int total, clock_t start)
{
  unsigned int i;
  Process *next = NULL;

  for(i = 0; i < total; i++)
    if(process[i].arrived && !process[i].working && !process[i].done) {
      if(next == NULL) next = &process[i];
      else if(process[i].duration < next->duration) next = &process[i];
    }

  return next;
}
