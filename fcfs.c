#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "headers/ep1.h"
#include "headers/core.h"
#include "headers/fcfs.h"

/*First Come First Served*/
void *fcfs(void *args)
{
  Process *process = ((Process*) args);

	if(process->coordinator) {
    unsigned int available_cores, count = 0, cores = sysconf(_SC_NPROCESSORS_ONLN);
    Core *core;

    core = malloc(cores * sizeof(*core));
    initialize_cores_fcfs(core, cores);

    context_changes = 0;
    process->context_changes = &context_changes;
    start = clock();
    while(count != process->total) {
      Process *next = NULL;

      fetch_process_fcfs(process->process, process->total);
      next = select_fcfs(process->process, process->total);
      if(next != NULL && available_cores > 0) use_core_fcfs(next, core, cores);
      count = finished_processes_fcfs(process->process, process->total);
      available_cores = check_cores_available_fcfs(core, cores);
    }
    fprintf(stderr, "Total context changes : %u\n", context_changes);
    fprintf(stderr, "Simulation time: %fs\n", (((float)(clock() - start)) / CLOCKS_PER_SEC));
    free(core); core = NULL;
  }
	else {
    /*Wait the system know the process has arrived*/
    sem_wait(&(process->next_stage));
    /*Wait the system assigns a CPU to the process*/
    sem_wait(&(process->next_stage));
    /*do task here*/
    do_task_fcfs(process);
    /*This thread is done. Mutex to write 'done' safely*/
    pthread_mutex_lock(&(process->mutex));
    process->finish = (((float)(clock() - start)) / CLOCKS_PER_SEC);
    process->done = True;
    pthread_mutex_unlock(&(process->mutex));
  }
	return NULL;
}

/*Assigns a process to a core*/
void use_core_fcfs(Process *process, Core *core, unsigned int cores)
{
  unsigned int i = 0;
  while(i < cores) {
    if(core[i].available) {
      core[i].available = False;
      core[i].process = process;
      core[i].process->working = True;
      if(paramd) fprintf(stderr, "Process '%s' assigned to CPU %d\n", core[i].process->name, i);
      sem_post(&(core[i].process->next_stage));
      break;
    }
    else i++;
  }
}

/*System checks if a CPU that was previously in use is available*/
unsigned int check_cores_available_fcfs(Core *core, unsigned int cores)
{
  unsigned int i, count = 0;
  for(i = 0; i < cores; i++) {
    if(core[i].process != NULL) {
      /*Mutex to read 'done' safely*/
      pthread_mutex_lock(&(core[i].process->mutex));
      if(core[i].process->done) {
        if(paramd) fprintf(stderr, "Process '%s' has released CPU %u.\n", core[i].process->name, i);
        core[i].available = True;
      }
      pthread_mutex_unlock(&(core[i].process->mutex));
      if(core[i].available) core[i].process = NULL;
    }
    if(core[i].available) count++;
  }
  return count;
}

/*Get the number of finished processes*/
unsigned int finished_processes_fcfs(Process *process, unsigned int total)
{
  unsigned int i, count = 0;
  for(i = 0; i < total; i++) {
    /*Mutex to read 'done' safely*/
    pthread_mutex_lock(&(process[i].mutex));
    if(process[i].done) {
      count++;
      if(process[i].working) {
        process[i].working = False;
        if(paramd) fprintf(stderr, "Process '%s' is done. Line '%s %f %f' will be written in the output file.\n",
          process[i].name, process[i].name, process[i].finish, process[i].finish - process[i].arrival);
      }
    }
    pthread_mutex_unlock(&(process[i].mutex));
  }
  return count;
}

/*Initialize cores*/
void initialize_cores_fcfs(Core *core, unsigned int cores)
{
  unsigned int count;
  for(count = 0; count < cores; count++) {
    core[count].available = True;
    core[count].process = NULL;
  }
}

/*Initiate threads to run FCFS scheduling*/
void do_fcfs(pthread_t *threads, Process *process, unsigned int *total)
{
  unsigned int i;
  for(i = 0; i <= *total; i++) {
    if(pthread_create(&threads[i], NULL, fcfs, &process[i])) {
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

/*Running process*/
void do_task_fcfs(Process *process)
{
  float sec, dl;
  clock_t duration = clock();

  while((sec = (((float)(clock() - duration)) / CLOCKS_PER_SEC)) < process->duration)
    if((dl = (((float)(clock() - start)) / CLOCKS_PER_SEC)) > process->deadline)
      process->failed = True;
}

/*Look up for new processes*/
void fetch_process_fcfs(Process *process, unsigned int total)
{
  unsigned int i;
  float sec = ((float)(clock() - start)) / CLOCKS_PER_SEC;

  for(i = 0; i < total; i++)
    if(sec >= process[i].arrival && !process[i].arrived) {
      process[i].arrived = True;
      sem_post(&(process[i].next_stage));
      if(paramd) fprintf(stderr, "Process '%s' has arrived (trace file line %u)\n", process[i].name, i + 1);
    }
}

/*Selects the earlier process */
Process *select_fcfs(Process *process, unsigned int total)
{
  unsigned int i;
  Process *next = NULL;

  for(i = 0; i < total; i++)
    if(process[i].arrived && !process[i].working && !process[i].done) {
      if(next == NULL) next = &process[i];
      else if(process[i].arrival < next->arrival) next = &process[i];
    }
  return next;
}
