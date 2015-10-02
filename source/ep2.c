#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "../headers/ep2.h"
#include "../headers/memory.h"

int main()
{
  int spcn = FF, sbsn = NRUP, *spc = &spcn, *sbs = &sbsn, file_loaded = 0, *load = &file_loaded;
  float intrvln = 10, *intrvl = &intrvln;
  char *cmd = NULL, *arg = NULL;

  using_history();
  while(true) {
    if((cmd = get_cmd(cmd)) == NULL) {
      printf("Expansion attempt has failed.\n");
      free(cmd); cmd = NULL;
      free(arg); arg = NULL;
      continue;
    }

    if((*load = cmd_load(cmd, arg)));
    else if(cmd_space(cmd, arg, spc));
    else if(cmd_subst(cmd, arg, sbs));
    else if(cmd_exec(cmd, arg, intrvl, load));
    else if(cmd_exit(cmd)) break;
    else unrecognized(cmd);

    free(cmd); cmd = NULL;
    free(arg); arg = NULL;
  }
  free(cmd); cmd = NULL;
  free(arg); arg = NULL;
  return 0;
}

/*Get user command*/
char *get_cmd(char *cmd)
{
  cmd = readline("[EP2]: ");
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

/*Execute 'carrega' command*/
int cmd_load(char *cmd, char *arg)
{
  if(strncmp(cmd, "carrega", 7) == 0) {
    char wd[1024];
    if((arg = get_arg(cmd, arg, "carrega")) == NULL) {
      printf("Bad argument for 'carrega'.\n");
      return 0;
    }
    else {
      /*Relative path*/
      if(arg[0] != '/') {
        if(getcwd(wd, sizeof(wd)) == NULL) {
          printf("Error while retrieving working directory.\n");
          return 0;
        }
        strcat(strcat(wd, "/"), arg);
      }
      /*Absolute Path*/
      else strcpy(wd, arg);
    }
    read_trace_file(wd);
    return 1;
  }
  return 0;
}

/*Reads the trace file, checking input and creates the array with all the processes.*/
int read_trace_file(char *fname)
{
  FILE *trace;
  int64_t pid;
  unsigned int p = 0, i, spaces, lists[2048]; /*Maximum of 1024 pairs [pn, tn]*/
  char c, buffer[4096], temp[1024], *data = buffer;
  Process *temporary;

  /*Allocate processes array*/
  plength = 64;
  process = malloc(plength * sizeof(*process));

  /*Opens trace file*/
  printf("Reading trace file '%s'...\n", fname);
  trace = fopen(fname, "r");

  /*Gets 'total' and 'virtual'*/
  fgets(buffer, 1024, trace);
  spaces = sscanf(buffer, "%u %u\n", &total, &virtual);
  if(spaces < 2) {
    printf("Error: was expecting to find pattern '%cu %cu' (line 0).\n", 37, 37);
    fclose(trace); return 0;
  }

  while(fgets(buffer, 1024, trace) != NULL) {
    unsigned int j = 0, k = 0;
    /*Get 't0 name tf b'*/
    spaces = sscanf(buffer, "%u %s %u %u ", &process[p].arrival, process[p].name, &process[p].finish, &process[p].size);
    if(spaces < 4) {
      printf("Error: Failed retrieving 't0 name tf b'. Was expecting to find pattern '%cu %cs %cu %cu' (line %u).\n", 37, 37, 37, 37, p + 1);
      if(p > 0) {
        for(j = 0; j < p; j++) {
          free(process[j].position); process[j].position = NULL;
          free(process[j].time); process[j].time = NULL;
        }
      }
      fclose(trace); return 0;
    }
    /*Fix pointer position*/
    for(c = buffer[i = spaces = 0]; spaces < 4; c = buffer[++i])
      if(c == ' ') spaces++;
    strcpy(buffer, data + i - 1);

    /*Retrieve all 'pn tn' pairs*/
    for(i = 0; i < strlen(buffer); i++) {
      if(isdigit(buffer[i])) { temp[j++] = buffer[i];}
      else if(j > 0 && (buffer[i] == ' ' || buffer[i] == '\n')) {
        temp[j] = '\0'; lists[k++] = atoi(temp); j = 0;
      }
    }
    if(k % 2 != 0) {
      printf("Error. Missing one 'tn' for process '%s' (line %u).\n", process[p].name, p + 1);
      if(p > 0) {
        for(j = 0; j < p; j++) {
          free(process[j].position); process[j].position = NULL;
          free(process[j].time); process[j].time = NULL;
        }
      }
      fclose(trace); return 0;
    }
    if(k == 0) {
      printf("Error. Missing 'pn tn' pairs for process '%s' (line %u).\n", process[p].name, p + 1);
      if(p > 0) {
        for(j = 0; j < p; j++) {
          free(process[j].position); process[j].position = NULL;
          free(process[j].time); process[j].time = NULL;
        }
      }
      fclose(trace); return 0;
    }
    process[p].length = k / 2;
    process[p].position = malloc(process[p].length * sizeof(int));
    process[p].time = malloc(process[p].length * sizeof(int));
    for(i = 0; i < k; i++) {
      /*Get pn*/
      if(i % 2 == 0) process[p].position[i / 2] = lists[i];
      /*Get tn*/
      else process[p].time[i / 2] = lists[i];
    }
    p++;
    /*Must realloc processes array*/
    if(p == plength / 2) {
      unsigned int z;
      temporary = malloc((2 * plength) * sizeof(*temporary));
      for(z = 0; z < p; z++) {
        strcpy(temporary[z].name, process[z].name);
        temporary[z].size = process[z].size;
        temporary[z].arrival = process[z].arrival;
        temporary[z].finish = process[z].finish;
        temporary[z].length = process[z].length;
        temporary[z].position = realloc(process[z].position, process[z].length);
        temporary[z].time = realloc(process[z].time, process[z].length);
      }
      free(process); process = temporary; plength *= 2;
    }
  }
  fclose(trace);
  /*Realloc processes array to a size that matches the number of processes*/
  process = realloc(process, p);
  plength = p;

  printf("Done!\n");
  return 1;
}



/*Execute 'espaco' command*/
int cmd_space(char *cmd, char *arg, int *spc)
{
  if(strncmp(cmd, "espaco", 6) == 0) {
    if((arg = get_arg(cmd, arg, "espaco")) == NULL)
      printf("Bad argument for 'espaco'.\n");
    else {
      if(sucessful_atoi(arg))
        switch (*spc = atoi(arg)) {
          case FF:
            printf("Selected FF as the free memory space management algorithm.\n");
            break;
          case NF:
            printf("Selected NF as the free memory space management algorithm.\n");
            break;
          case QF:
            printf("Selected QF as the free memory space management algorithm.\n");
            break;
          default:
            printf("Invalid option '%d' for free memory space management algorithm.\n", *spc);
            break;
        }
        else
          printf("Invalid option '%s' for free memory space management algorithm.\n", arg);
    }
    return 1;
  }
  return 0;
}

/*Execute 'substitui' command*/
int cmd_subst(char *cmd, char *arg, int *sbs)
{
  if(strncmp(cmd, "substitui", 9) == 0) {
    if((arg = get_arg(cmd, arg, "substitui")) == NULL)
      printf("Bad argument for 'substitui'.\n");
    else {
      if(sucessful_atoi(arg))
        switch (*sbs = atoi(arg)) {
          case NRUP:
            printf("Selected NRUP as the page substitution algorithm.\n");
            break;
          case FIFO:
            printf("Selected FIFO as the page substitution algorithm.\n");
            break;
          case SCP:
            printf("Selected SCP as the page substitution algorithm.\n");
            break;
          case LRUP:
            printf("Selected LRUP as the page substitution algorithm.\n");
            break;
          default:
            printf("Invalid option '%d' for page substitution algorithm.\n", *sbs);
            break;
        }
        else
          printf("Invalid option '%s' for page substitution algorithm.\n", arg);
    }
    return 1;
  }
  return 0;
}

/*Execute 'executa' command*/
int cmd_exec(char *cmd, char *arg, float *intrvl, int *load)
{
  Event * e;
  struct timespec start_time, current_time;  /*Real time*/
  int print_time = 0;

  if(strncmp(cmd, "executa", 7) == 0) {
    if((arg = get_arg(cmd, arg, "executa")) == NULL)
      printf("Bad argument for 'executa'.\n");
    else {
      if(sucessful_atof(arg)) {
        *intrvl = atof(arg);
        printf("Selected '%s' as the time interval.\n", arg);

        if(!(*load)) {
          printf("You must load a trace file using command 'carrega' before executing the simulator.\n");
          return 0;
        }

        /*
                clock_gettime(CLOCK_MONOTONIC, &start_time);
                while((e = get_next_event()) != NULL) {

                  do {
                    clock_gettime(CLOCK_MONOTONIC, &current_time);
                    current_time.tv_sec = current_time.tv_sec - start_time.tv_sec;
                    if (print_time <= current_time.tv_sec) {
                      print_memory();
                      print_time += intrvl;
                    }
                  } while (e->time > current_time.tv_sec);

                  if(e->event_type == START) {
                    start_process(e->PID);
                  } else if(e->event_type == END) {
                    kill_process(e->PID);
                  } else {
                    access_memory(e->PID, e->position);
                  }
                }*/
      }
      else
        printf("Invalid option '%s' for interval.\n", arg);
    }
    return 1;
  }
  return 0;
}

/*Terminate session*/
int cmd_exit(char *cmd)
{
  if(strcmp(cmd, "sai") == 0 && strlen(cmd) == 3) return 1;
  return 0;
}

/*User invoked unknown command to ep1sh*/
void unrecognized(char *cmd)
{
  printf("Unrecognized command \"%s\".\n", cmd);
}

/*Get the argument for requested command 'rqst'*/
char *get_arg(char *cmd, char *arg, char *rqst)
{
  int i, j = 0;

  arg = (char*) malloc(strlen(cmd) * sizeof(*arg));
  for(i = strlen(rqst); isspace(cmd[i]); i++) continue;
  if(cmd[i] == '\0') {
    free(arg); arg = NULL;
    return NULL;
  }
  while(!isspace(cmd[i]) && cmd[i] != '\0') arg[j++] = cmd[i++];
  arg[j] = '\0';
  return arg;
}

/*Catch atoi function exception*/
int sucessful_atoi(char *number)
{
  int i;
  for(i = 0; number[i] != '\0'; i++) if(!isdigit(number[i])) return 0;
  return 1;
}

/*Catch atof function exception*/
int sucessful_atof(char *number)
{
  int i, j = 0;
  for(i = 0; number[i] != '\0'; i++) {
    if(number[i] == '.' && j == 0) { j++; continue; }
    if(!isdigit(number[i]) && j > 0) return 0;
  }
  return 1;
}


/*
Notas:
- Mudado para C pq não vejo necessidade de usar OOP
- TODO: load file
- TODO: execute ep2

Observações:
- Não tem saídas para serem escritas neste EP (I), e o arquivo de trace é localizado
interativamente com o comando 'carrega' (II). Por causa de (I), não há necessidade de usarmos
a arquitetura com diretório de 'outputs' e por causa de (II), não há necessidade de usarmos
a arquitetura com diretório 'inputs'.
*/
