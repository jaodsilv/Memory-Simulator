#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
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

  switch(thread->role) {
    case MANAGER:
      printf("I am the manager thread!\n");
      break;
    case PRINTER:
      printf("I am the printer thread!\n");
      break;
    case TIMER:
      printf("I am the timer thread!\n");
      break;
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
  for(i = 0; i < 3; i++) {
    switch(i) {
      case MANAGER:
        args[MANAGER].role = MANAGER;
        args[MANAGER].spc = spc;
        args[MANAGER].sbs = sbs;
        break;
      case PRINTER:
        args[PRINTER].role = PRINTER;
        args[PRINTER].intrvl = intrvl;
        break;
      case TIMER:
        args[TIMER].role = TIMER;
        break;
    }
  }
}
