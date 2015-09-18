#include <bits/stdc++.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include "../headers/ep2.h"

using namespace std;

#define PAGE_SIZE 16

int main()
{
  string command, path;
  int espaco, substitui, intervalo;
  while (true) {
    cout << "[EP2]: ";
    cin >> command;
    if (command == "carrega") // Should also load the file;
      cin >> path;
    else if (command == "espaco")
      cin >> espaco;
    else if (command == "substitui")
      cin >> substitui;
    else if (command == "sai")
      return EXIT_SUCCESS;
    else if (command == "executa")
    {
      cin >> intervalo;
      run(path, espaco, substitui, intervalo);
    }
    else cerr << "\nep2 error: command not found: " << command << ".\n";
  }
}

int run(char *path, int espaco, int substitui, int intervalo)
{
  unsigned int all = 0, *total = &all;
  Process *process, *p;
  process = malloc(100 * sizeof(*process));

  if((p = read_trace_file(process, wd, argv[1], total)) == NULL) {
    free(process); process = NULL;
  }
  else process = p;

  if(argv[3][0] == 'd') paramd = true;
  else paramd = false;

  if(process != NULL) {
    unsigned int scheduler;
    pthread_t *threads;

    if(!initialize_mutex(process, total)) return 0;
    threads = malloc((*total + 1) * sizeof(*threads));

    switch(scheduler = atoi(argv[0])) {
      case FCFS:
        /*do FCFS*/
        printf("1. FCFS\n");
        do_fcfs(threads, process, total);
        printf("\n\n* * * * * * * * * *\n\n");
        printf("FCFS simulation has finished in:\nCPU Time: %fs\nReal: %d.%ds\nWriting output...\n", ((float)finish_cpu_time / CLOCKS_PER_SEC), (int)finish_elapsed_time.tv_sec, (int)finish_elapsed_time.tv_nsec);
        write_output(process, wd, argv[2], total);
        free(threads); threads = NULL;
        free_mutex(process, total);
        free(process); process = NULL;
        break;
      case SJF:
        /*do SJF*/
        printf("2. SJF\n");
        do_sjf(threads, process, total);
        printf("\n\n* * * * * * * * * *\n\n");
        printf("SJF simulation has finished in:\nCPU Time: %fs\nReal: %d.%ds\nWriting output...\n", ((float)finish_cpu_time / CLOCKS_PER_SEC), (int)finish_elapsed_time.tv_sec, (int)finish_elapsed_time.tv_nsec);
        write_output(process, wd, argv[2], total);
        free(threads); threads = NULL;
        free_mutex(process, total);
        free(process); process = NULL;
        break;
      case SRTN:
        /*do SRTN*/
        printf("3. SRTN\n");
        do_srtn(threads, process, total);
        printf("\n\n* * * * * * * * * *\n\n");
        printf("SRTN simulation has finished in:\nCPU Time: %fs\nReal: %d.%ds\nWriting output...\n", ((float)finish_cpu_time / CLOCKS_PER_SEC), (int)finish_elapsed_time.tv_sec, (int)finish_elapsed_time.tv_nsec);
        write_output(process, wd, argv[2], total);
        free(threads); threads = NULL;
        free_mutex(process, total);
        free(process); process = NULL;
        break;
      case RR:
        /*do RR*/
        printf("4. RR\n");
        do_rr(threads, process, total);
        printf("\n\n* * * * * * * * * *\n\n");
        printf("RR simulation has finished in:\nCPU Time: %fs\nReal: %d.%ds\nWriting output...\n", ((float)finish_cpu_time / CLOCKS_PER_SEC), (int)finish_elapsed_time.tv_sec, (int)finish_elapsed_time.tv_nsec);
        write_output(process, wd, argv[2], total);
        free(threads); threads = NULL;
        free_mutex(process, total);
        free(process); process = NULL;
        break;
      case PS:
        /*do PS*/
        printf("5. PS\n");
        do_ps(threads, process, total);
        printf("\n\n* * * * * * * * * *\n\n");
        printf("PS simulation has finished in:\nCPU Time: %fs\nReal: %d.%ds\nWriting output...\n", ((float)finish_cpu_time / CLOCKS_PER_SEC), (int)finish_elapsed_time.tv_sec, (int)finish_elapsed_time.tv_nsec);
        write_output(process, wd, argv[2], total);
        free(threads); threads = NULL;
        free_mutex(process, total);
        free(process); process = NULL;
        break;
      case EDF:
        /*do EDF*/
        printf("6. EDF\n");
        do_edf(threads, process, total);
        printf("\n\n* * * * * * * * * *\n\n");
        printf("EDF simulation has finished in:\nCPU Time: %fs\nReal: %d.%ds\nWriting output...\n", ((float)finish_cpu_time / CLOCKS_PER_SEC), (int)finish_elapsed_time.tv_sec, (int)finish_elapsed_time.tv_nsec);
        write_output(process, wd, argv[2], total);
        free(threads); threads = NULL;
        free_mutex(process, total);
        free(process); process = NULL;
        break;
    }
  }
  else {
    printf("\nError reading file. Make sure it is written in expected ");
    printf("format and you are executing ep1 command from the right folder.\n");
  }
  return 0;
}

/*Writes the output file*/
void write_output(Process *process, char *wd, char *tfile, unsigned int *total)
{
  FILE *fptr;
  char output[256];

  strcat(strcat(strcpy(output, wd), "/outputs/"), tfile);
  if((fptr = fopen(output, "w")) != NULL) {
    unsigned int i;
    for(i = 0; i < *total; i++)
      fprintf(fptr, "%s %f %f\n", process[i].name, process[i].finish_elapsed_time, process[i].finish_elapsed_time - process[i].arrival);
    fprintf(fptr, "%u\n", *process[i].context_changes);
    fclose(fptr);
    printf("Done!\nYour output file can be found in '%s'.\n", output);
  }
  else printf("Error writing output file.\n");

}


/*Free mutexes*/
void free_mutex(Process *process, unsigned int *total)
{
  unsigned int i;
  for(i = 0; i < *total; i++) {
    sem_destroy(&process[i].next_stage);
    pthread_mutex_destroy(&process[i].mutex);
  }
}

/*Initializes mutexes*/
int initialize_mutex(Process *process, unsigned int *total)
{
  unsigned int i;
  for(i = 0; i < *total; i++) {
    unsigned int j;
    if(sem_init(&process[i].next_stage, 0, 0)) {
      printf("Error initializing semaphore.\n");
      for(j = 0; j < i; j++) {
        sem_destroy(&process[j].next_stage);
        pthread_mutex_destroy(&process[j].mutex);
      }
      free(process); process = NULL;
      return 0;
    }
    if(pthread_mutex_init(&process[i].mutex, NULL) != 0) {
      printf("\nError initializing MutEx.\n");
      for(j = 0; j < i; j++) {
        sem_destroy(&process[j].next_stage);
        pthread_mutex_destroy(&process[j].mutex);
      }
      free(process); process = NULL;
      return 0;
    }
  }
  return 1;
}

/*Read the trace file 'tfile' and store its content in the array 'process',
  creating process "objects"*/
Process *read_trace_file(Process *process, char *wd, char *tfile, unsigned int *total)
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
            if(dots > 1) { fclose(fptr); return NULL; }
          }
          else if(is_blank(c) && i > 0) {
            tmp[i] = '\0'; i = 0; item = 2; dots = 0;
            process[j].arrival = atof(tmp);
            continue;
          }
          else { fclose(fptr); return NULL; }
          break;

        case 2: /*Get name*/
          if(!isspace(c)) tmp[i++] = c;
          else if(is_blank(c) && i > 0) {
            tmp[i] = '\0'; i = 0; item = 3;
            strcpy(process[j].name, tmp);
            continue;
          }
          else { fclose(fptr); return NULL; }
          break;

        case 3: /*Get duration*/
          if(isdigit(c) || c == '.') {
            tmp[i++] = c;
            if(c == '.') dots++;
            if(dots > 1) { fclose(fptr); return NULL; }
          }
          else if(is_blank(c) && i > 0) {
            tmp[i] = '\0'; i = 0; item = 4; dots = 0;
            process[j].duration = process[j].remaining = atof(tmp);
            continue;
          }
          else { fclose(fptr); return NULL; }
          break;

        case 4: /*Get deadline*/
          if(isdigit(c) || c == '.') {
            tmp[i++] = c;
            if(c == '.') dots++;
            if(dots > 1) { fclose(fptr); return NULL; }
          }
          else if(is_blank(c) && i > 0) {
            tmp[i] = '\0'; i = 0; item = 5; dots = 0;
            process[j].deadline = atof(tmp);
            continue;
          }
          else { fclose(fptr); return NULL; }
          break;

        case 5: /*Get Priority*/
          if(isdigit(c) || c == '-') {
            tmp[i++] = c;
            if(c == '-') dots++;
            if(dots > 1) { fclose(fptr); return NULL; }
          }
          else if(isspace(c) && i > 0) {
            tmp[i] = '\0'; i = 0; item = 1; dots = 0; *total += 1;
            process[j].priority = atoi(tmp);
            if(process[j].priority < -20 || process[j].priority > 19) {
              fclose(fptr); return NULL;
            }
            process[j].arrived = false;
            process[j].done = false;
            process[j].working = false;
            process[j].failed = false;
            process[j++].coordinator = false;
            if(j == size / 2) {
              process = realloc(process, (size * 2) * sizeof(*process));
              size *= 2;
            }
          }
        else { fclose(fptr); return NULL; }
        break;
      }
    }
    process[j].coordinator = true;
    process[j].total = *total;
    process[j++].process = process;
    fclose(fptr);
    return process = realloc(process, j * sizeof(*process));
  }
  else {
    printf("EP1: error opening requested file '%s'.\n", input);
    return 0;
  }
}

/*Checks if the character is a space or tab*/
int is_blank(char c)
{
  return c == ' ' || c == '\t';
}
