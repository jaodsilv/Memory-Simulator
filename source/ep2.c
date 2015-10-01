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
#include "../headers/process.h"

int main(int argc, char **argv)
{
  int spcn = FF, sbsn = NRUP, *spc = &spcn, *sbs = &sbsn;
  float intrvl = 0;
  char *cmd = NULL, *arg = NULL;

  using_history();
  while(true) {

    if((cmd = get_cmd(cmd)) == NULL) {
      printf("Expansion attempt has failed.\n");
      free_pointer((void*)cmd);
      free_pointer((void*)arg);
      continue;
    }

    if(cmd_load(cmd, arg));
    else if(cmd_space(cmd, arg, spc));
    else if(cmd_subst(cmd, arg, sbs));
    else if(cmd_exec(cmd, arg, intrvl));
    else if(cmd_exit(cmd)) break;
    else unrecognized(cmd);

    free_pointer((void*)cmd);
    free_pointer((void*)arg);
  }
  free_pointer((void*)cmd);
  free_pointer((void*)arg);
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
  int tot, vir, t0, tf, b, pn, tn;
  size_t buffer_size = 100;
  int64_t id;
  char * name, * buffer;
  FILE * trace;

  if(strncmp(cmd, "carrega", 7) == 0) {
    if((arg = get_arg(cmd, arg, "carrega")) == NULL)
      printf("Bad argument for 'carrega'.\n");
    else {
      char wd[1024];
      /*Relative path*/
      if(arg[0] != '/') {
        if(getcwd(wd, sizeof(wd)) == NULL)
          printf("Error while retrieving working directory.\n");
        strcat(strcat(wd, "/"), arg);
      }
      /*Absolute Path*/
      else strcpy(wd, arg);

      /* TODO: Validade if there happens any error. */
      trace = fopen(wd, "r");

      fscanf(trace, "%d %d\n", &tot, &vir);
      create_mems(tot, vir);
      name = (char *) malloc(buffer_size*sizeof(char));

      while (fscanf(trace, "%d %s %d %d ", &t0, name, &tf, &b) != EOF) {
        id = create_process(name, b);
        add_event(START, t0, 0, id);
        add_event(END, tf, 0, id);
        getline(&buffer, &buffer_size, trace);
        while (sscanf(buffer, "%d %d", &pn, &tn) != EOF) {
          add_event(ACCESS, tn, pn, id);
        }
      }
      fclose(trace);

      free(buffer);
      free(name);
    }
    return 1;
  }
  return 0;
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
int cmd_exec(char *cmd, char *arg, float intrvl)
{
  Event * e;
  struct timespec start_time, current_time;  /*Real time*/
  int print_time = 0;

  if(strncmp(cmd, "executa", 7) == 0) {
    if((arg = get_arg(cmd, arg, "executa")) == NULL)
      printf("Bad argument for 'executa'.\n");
    else {
      if(sucessful_atof(arg)) {
        intrvl = atof(arg);
        printf("Selected '%s' as the time interval.\n", arg);

        clock_gettime(CLOCK_MONOTONIC, &start_time);
        while((e = get_next_event()) != NULL) {

          /* busy waiting til next event */

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
        }
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
    free(arg);
    return NULL;
  }
  while(!isspace(cmd[i]) && cmd[i] != '\0') arg[j++] = cmd[i++];
  arg[j] = '\0';
  return arg;
}

/*Generic pointer disallocation function*/
void free_pointer(void *pointer)
{
  if(pointer != NULL) free(pointer);
  pointer = NULL;
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
