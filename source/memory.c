#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include "../headers/memory.h"

void simulate(int spc, int sbs, float intrvl)
{
  pthread_t threads[3];   /*Manager(position = 0), Printer(position = 1) and Timer(position = 2)*/
  Thread args[3];         /*Arguments of the pthreads*/

  /*********************************/
  /*TODO: A bunch of 'ifs' checking inputs (basically processes information.
  Stuff like making sure no processes have an access time greater than its life time and this
  kind of stuff just for security. Unlike EP1, lets assure that no 'impossible' process can be
  created in order to don't fuck up with the statistics)*/
  /*********************************/
  assign_thread_roles(args, spc, sbs, intrvl);
  /*TODO: Create memories (files) here, before the call to 'do_simulation' function*/
  do_simulation(threads, args);


  /*printf("Aqui o que deve ser usado:\n");
  printf("Algoritmo espaco selecionado: %d\n", spc);
  printf("Algoritmo paginacao selecionado: %d\n", sbs);
  printf("Intervalo de tempo selecionado: %f\n", intrvl);
  printf("Limites memórias: física = %u virtual = %u\n", total, virtual);

  printf("Vetor com os %u processos:\n", plength);
  if(1==1) {
    unsigned int i;
    for(i = 0; i < plength; i++) {
      unsigned int j;
      printf("Processo '%s': chegada = %u saida = %u tamanho = %u \n\tLista de Acessos [pn, tn]: ", process[i].name, process[i].arrival, process[i].finish, process[i].length);
      for(j = 0; j < process[i].length; j++) {
        printf("[%u , %u] ", process[i].position[j], process[i].time[j]);
      }
      printf("\n");
    }
  }
  */
}

/*Run the simulation*/
void *run(void *args)
{
  Thread *thread = ((Thread*) args);

  if(thread->role == MANAGER) {
    printf("I am the manager thread!\n");
    /*TODO: create memory files here*/
    thread->ready = 1;
    /*Wait Tmer thread starts the simulation*/
    while(elapsed_time == -1) continue;
    while(simulating) {

      /*simulating = 0;*/
    }
  }
  if(thread->role == PRINTER) {
    float last = 0, t = 0, ret;
    printf("I am the printer thread!\n");
    /*Wait Manager thread finish simulation preparations. Making a local assignment to avoid concurrency problems*/
    while(true) {
      int ready = (thread - 1)->ready;
      if(ready) break;
    }
    /*Wait Tmer thread starts the simulation*/
    while(elapsed_time == -1) continue;
    while(simulating) {
      /*Idea: Assign atomically to ret first and then do a local comparission between ret and last to avoid using a semaphore*/
      ret = elapsed_time;
      if(last != ret) {
        last = ret; t += 0.1;
        if(t >= thread->intrvl) {
          t = 0;
          printf("Printer: time is %.1f\n", last);
          /*Do grab lock and print_memory here*/
        }
      }
    }
  }
  if(thread->role == TIMER) {
    float t = 0;
    struct timespec now, range;
    printf("I am the timer thread!\n");
    /*Wait Manager thread finish simulation preparations. Making a local assignment to avoid concurrency problems*/
    while(true) {
      int ready = (thread - 2)->ready;
      if(ready) break;
    }
    /*Starts simulation*/
    elapsed_time = 0; clock_gettime(CLOCK_MONOTONIC, &range);
    while(simulating) {
      clock_gettime(CLOCK_MONOTONIC, &now);
      /*t is incremented by 0.1 every 0.1s*/
      if(abs(now.tv_nsec - range.tv_nsec) > 100000000) {
        /*Idea: increment a local 't' to assign time global 'elapsed_time' atomically and avoid the use of a semaphore*/
        t += 0.1; elapsed_time = t;
        /*Restart range*/
        clock_gettime(CLOCK_MONOTONIC, &range);
      }
    }
  }
  return NULL;
}

/*Create the htreads and join them to start the simulation, calling 'run'*/
void do_simulation(pthread_t *threads, Thread *args)
{
  int i;
  for(i = 0; i < 3; i++) {
    if(pthread_create(&threads[i], NULL, run, &args[i])) {
        printf("Error creating threads.\n");
        return;
      }
    }
  for(i = 0; i < 3; i++) {
    if(pthread_join(threads[i], NULL)) {
      printf("Error joining threads.\n");
      return;
    }
  }
}

/*Assign simulation roles to threads*/
void assign_thread_roles(Thread *args, int spc, int sbs, float intrvl)
{
  int i;
  simulating = 1;
  for(i = 0; i < 3; i++) {
    switch(i) {
      case MANAGER:
        args[MANAGER].role = MANAGER;
        args[MANAGER].spc = spc;
        args[MANAGER].sbs = sbs;
        args[MANAGER].ready = 0;
        break;
      case PRINTER:
        args[PRINTER].role = PRINTER;
        args[PRINTER].intrvl = intrvl;
        break;
      case TIMER:
        elapsed_time = -1;
        args[TIMER].role = TIMER;
        break;
    }
  }
}
