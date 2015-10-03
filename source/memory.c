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
  /*Assign roles to threads*/
  assign_thread_roles(args, spc, sbs, intrvl);
  /*Create memory files*/
  create_memory(PHYSICAL); create_memory(VIRTUAL);
  /*Initialize simulator*/
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
    /*Wait Timer thread starts the simulation*/
    while(elapsed_time == -1) continue;
    while(simulating) {

      /*simulating = 0;*/
    }
  }
  if(thread->role == PRINTER) {
    FILE *mfile, *mfile_u;
    float last = 0, t = 0, ret;
    int8_t *physical_array, *virtual_array;
    uint8_t *physical_array_u, *virtual_array_u;

    physical_array = malloc(total * sizeof(*physical_array));
    virtual_array = malloc(virtual * sizeof(*virtual_array));
    physical_array_u = malloc(total * sizeof(*physical_array_u));
    virtual_array_u = malloc(virtual * sizeof(*virtual_array_u));
    printf("I am the printer thread!\n");
    /*Wait Timer thread starts the simulation*/
    while(elapsed_time == -1) continue;
    while(simulating) {
      /*IDEA: Assign atomically to ret first and then do a local comparission between ret and last to avoid using a semaphore*/
      ret = elapsed_time;
      if(last != ret) {
        last = ret; t += 0.1;
        if(t >= thread->intrvl) {
          unsigned int i; t = 0;
          /*TODO: grab mutex here*/

          /*IDEA: In order to operate considering the initial -1 value, we need to have
          both signed and unsigned integers of size 1b (as the enunciation of the EP commanded),
          this because unsigned goes from [0, 255] and -1 is out of the interval. Thus,
          we need another file pointer as well. Whenever there is a -1 in a position of the
          file, we use the signed values and file pointer to print it, otherwise, we use the
          unsigned ones*/

          /*Read physical binary file*/
          mfile = fopen("/tmp/ep2.mem", "rb");
          mfile_u = fopen("/tmp/ep2.mem", "rb");
          fread(physical_array, sizeof(int8_t), (size_t)total, mfile);
          fread(physical_array_u, sizeof(uint8_t), (size_t)total, mfile_u);

          /*Prints physical memory file*/
          for(i = 0; i < total; i++) {
            if(physical_array[i] < 0) printf("-1 ");
            else printf("%u ", physical_array_u[i]);
          } printf("\n");
          fclose(mfile); fclose(mfile_u);

          /*Read virtual binary file*/
          mfile = fopen("/tmp/ep2.vir", "rb");
          mfile_u = fopen("/tmp/ep2.vir", "rb");
          fread(virtual_array, sizeof(int8_t), (size_t)virtual, mfile);
          fread(virtual_array_u, sizeof(uint8_t), (size_t)virtual, mfile_u);

          /*Prints physical memory file*/
          for(i = 0; i < virtual; i++) {
            if(virtual_array[i] < 0) printf("-1 ");
            else printf("%u ", virtual_array_u[i]);
          } printf("\n");
          fclose(mfile); fclose(mfile_u);

          /*TODO: release mutex here*/
        }
      }
    }
    free(physical_array); physical_array = NULL;
    free(virtual_array); virtual_array = NULL;
    free(physical_array_u); physical_array_u = NULL;
    free(virtual_array_u); virtual_array_u = NULL;
  }
  if(thread->role == TIMER) {
    float t = 0;
    struct timespec now, range;
    printf("I am the timer thread!\n");
    /*Starts simulation*/
    elapsed_time = 0; clock_gettime(CLOCK_MONOTONIC, &range);
    while(simulating) {
      clock_gettime(CLOCK_MONOTONIC, &now);
      /*t is incremented by 0.1 every 0.1s*/
      if(abs(now.tv_nsec - range.tv_nsec) > 100000000) {
        /*IDEA: increment a local 't' to assign time global 'elapsed_time' atomically and avoid the use of a semaphore*/
        t += 0.1; elapsed_time = t;
        /*Restart range*/
        clock_gettime(CLOCK_MONOTONIC, &range);
      }
    }
  }
  return NULL;
}

/*Write binary files to represent a memory*/
void create_memory(int type)
{
  FILE *mfile;
  int8_t *n1, *n2;

  switch(type) {
    unsigned int i;
    case PHYSICAL:
      n1 = malloc(total * sizeof(*n1));
      for(i = 0; i < total; i++) n1[i] = -1;
      mfile = fopen("/tmp/ep2.mem", "wb");
      fwrite(n1, sizeof(n1[0]), total * sizeof(n1[0]), mfile);
      free(n1); n1 = NULL;
      break;
    case VIRTUAL:
      n2 = malloc(virtual * sizeof(*n2));
      for(i = 0; i < virtual; i++) n2[i] = -1;
      mfile = fopen("/tmp/ep2.vir", "wb");
      fwrite(n2, sizeof(n2[0]), virtual * sizeof(n2[0]), mfile);
      free(n2); n2 = NULL;
      break;
  }
  fclose(mfile);
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
