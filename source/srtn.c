#define _GNU_SOURCE
#define _POSIX_C_SOURCE 199309L  /*clock_gettime*/
#include <math.h>                /*abs*/
#include <time.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "../headers/ep1.h"
#include "../headers/core.h"
#include "../headers/srtn.h"

/*Shortest Remaining Time Next*/
void *srtn(void *args)
{
  Process *process = ((Process*) args);
  process->thread = pthread_self();

  /*Coordinator thread*/
	if(process->coordinator) {
    unsigned int available_cores, count = 0, cores = sysconf(_SC_NPROCESSORS_ONLN);
    Core *core;

    CPU_ZERO(&cpu_set);
    for(count = 0; count < cores; count++) CPU_SET(count, &cpu_set);
    for(count = 0; count <= process->total; count++) {
      int ret;
      if((ret = pthread_setaffinity_np(process->process[count].thread, sizeof(cpu_set), &cpu_set)) != 0) {
        fprintf(stderr, "error: pthread set affinity.\n"); exit(EXIT_FAILURE);
      }
      if((ret = pthread_getaffinity_np(process->process[count].thread, sizeof(cpu_set), &cpu_set)) != 0) {
        fprintf(stderr, "error: pthread get affinity.\n"); exit(EXIT_FAILURE);
      }
    }
    core = malloc(cores * sizeof(*core));
    initialize_cores_srtn(core, cores);
    count = 0;
    /*Initialize simulator globals*/
    context_changes = 0;
    process->context_changes = &context_changes;
    clock_gettime(CLOCK_MONOTONIC, &start_elapsed_time);
    start_cpu_time = clock();
    /*Start simulation*/
    while(count != process->total) {
      Process *next = NULL;

      fetch_process_srtn(process->process, process->total);
      next = select_srtn(process->process, process->total);
      if(next != NULL && available_cores == 0) available_cores = release_core_srtn(next, core, cores);
      if(next != NULL && available_cores > 0) use_core_srtn(next, core, cores);
      count = finished_processes_srtn(process->process, process->total);
      available_cores = check_cores_available_srtn(core, cores);
    }

    /*Get simulation ending time*/
    finish_cpu_time = ((clock() - start_cpu_time));
    clock_gettime(CLOCK_MONOTONIC, &finish_elapsed_time);
    finish_elapsed_time.tv_sec = finish_elapsed_time.tv_sec - start_elapsed_time.tv_sec;
    if(finish_elapsed_time.tv_nsec > start_elapsed_time.tv_nsec)
      finish_elapsed_time.tv_nsec = finish_elapsed_time.tv_nsec - start_elapsed_time.tv_nsec;
    else
      finish_elapsed_time.tv_nsec = start_elapsed_time.tv_nsec - finish_elapsed_time.tv_nsec;

    if(paramd) fprintf(stderr, "Total context changes: %u\n", context_changes);
    free(core); core = NULL;
  }
  /*Other threads*/
	else {
    int completed = 0;
    struct timespec time;
    /*Wait the system know the process has arrived*/
    sem_wait(&(process->next_stage));
    /*Wait the system assigns a CPU to the process*/
    while(!completed) {
      sem_wait(&(process->next_stage));
      /*Perform a task*/
      completed = do_task_srtn(process);
    }
    /*This thread is done. Mutex to write 'done' safely*/
    pthread_mutex_lock(&(process->mutex));
    process->finish_cpu_time = (((float)(clock() - start_cpu_time)) / CLOCKS_PER_SEC);
    clock_gettime(CLOCK_MONOTONIC, &time);
    time.tv_sec = time.tv_sec - start_elapsed_time.tv_sec;
    if(time.tv_nsec > start_elapsed_time.tv_nsec)
      time.tv_nsec = time.tv_nsec - start_elapsed_time.tv_nsec;
    else
      time.tv_nsec = start_elapsed_time.tv_nsec - time.tv_nsec;

    process->finish_elapsed_time = (float)time.tv_sec + ((float)time.tv_nsec / 1000000000 );
    process->done = True;
    pthread_mutex_unlock(&(process->mutex));
  }
	return NULL;
}

/*Assigns a process to a core*/
void use_core_srtn(Process *process, Core *core, unsigned int cores)
{
  unsigned int i = 0;
  while(i < cores) {
    if(core[i].available) {
      core[i].available = False;
      core[i].process = process;
      core[i].process->working = True;
      if(paramd && CPU_ISSET(i, &cpu_set))
        fprintf(stderr, "Process '%s' assigned to CPU %d\n", core[i].process->name, i);
      sem_post(&(core[i].process->next_stage));
      break;
    }
    else i++;
  }
}

/*System checks if a CPU that was previously in use is available*/
unsigned int check_cores_available_srtn(Core *core, unsigned int cores)
{
  unsigned int i, count = 0;
  for(i = 0; i < cores; i++) {
    if(core[i].process != NULL) {
      /*Mutex to read 'done' safely*/
      pthread_mutex_lock(&(core[i].process->mutex));
      if(core[i].process->done) {
        if(paramd && CPU_ISSET(i, &cpu_set))
          fprintf(stderr, "Process '%s' has released CPU %u\n", core[i].process->name, i);
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
unsigned int finished_processes_srtn(Process *process, unsigned int total)
{
  unsigned int i, count = 0;
  for(i = 0; i < total; i++) {
    /*Mutex to read 'done' safely*/
    pthread_mutex_lock(&(process[i].mutex));
    if(process[i].done) {
      count++;
      if(process[i].working) {
        process[i].working = False;
        if(paramd) fprintf(stderr, "Process '%s' is done. Line '%s %f %f' will be written to the output file\n",
        process[i].name, process[i].name, process[i].finish_elapsed_time, process[i].finish_elapsed_time - process[i].arrival);
      }
    }
    pthread_mutex_unlock(&(process[i].mutex));
  }
  return count;
}

/*Initialize cores*/
void initialize_cores_srtn(Core *core, unsigned int cores)
{
  unsigned int count;
  for(count = 0; count < cores; count++) {
    core[count].available = True;
    core[count].process = NULL;
  }
}

/*Initiate threads to run srtn scheduling*/
void do_srtn(pthread_t *threads, Process *process, unsigned int *total)
{
  unsigned int i;
  for(i = 0; i <= *total; i++) {
    if(pthread_create(&threads[i], NULL, srtn, &process[i])) {
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
int do_task_srtn(Process *process)
{
  struct timespec duration, now;
  clock_gettime(CLOCK_MONOTONIC, &duration);

  while(process->remaining > 0) {
    clock_gettime(CLOCK_MONOTONIC, &now);
    if(abs(now.tv_nsec - duration.tv_nsec) > 25000000) {
      process->remaining -= 0.025;
      clock_gettime(CLOCK_MONOTONIC, &duration);
    }
    if(!process->working) return 0;
  }

  clock_gettime(CLOCK_MONOTONIC, &duration);
  duration.tv_sec = duration.tv_sec - start_elapsed_time.tv_sec;
  if(duration.tv_nsec > start_elapsed_time.tv_nsec)
    duration.tv_nsec = duration.tv_nsec - start_elapsed_time.tv_nsec;
  else
    duration.tv_nsec = start_elapsed_time.tv_nsec - duration.tv_nsec;

  if((float)duration.tv_sec + ((float)duration.tv_nsec / 1000000000 ) > process->deadline)
    process->failed = True;

  return 1;
}

/*Look up for new processes*/
void fetch_process_srtn(Process *process, unsigned int total)
{
  float sec;
  unsigned int i;
  struct timespec time;

  clock_gettime(CLOCK_MONOTONIC, &time);
  time.tv_sec = time.tv_sec - start_elapsed_time.tv_sec;
  if(time.tv_nsec > start_elapsed_time.tv_nsec)
    time.tv_nsec = time.tv_nsec - start_elapsed_time.tv_nsec;
  else
    time.tv_nsec = start_elapsed_time.tv_nsec - time.tv_nsec;

  sec = (float)time.tv_sec + ((float)time.tv_nsec / 1000000000 );
  for(i = 0; i < total; i++)
    if(sec >= process[i].arrival && !process[i].arrived) {
      process[i].arrived = True;
      sem_post(&(process[i].next_stage));
      if(paramd) fprintf(stderr, "Process '%s' has arrived (trace file line %u)\n", process[i].name, i + 1);
    }
}

/*Selects the shortest process */
Process *select_srtn(Process *process, unsigned int total)
{
  unsigned int i;
  Process *next = NULL;

  for(i = 0; i < total; i++)
    if(process[i].arrived && !process[i].working && !process[i].done) {
      if(next == NULL) next = &process[i];
      else if(process[i].remaining < next->remaining) next = &process[i];
    }
  return next;
}

unsigned int release_core_srtn(Process *next, Core *core, unsigned int cores)
{
  unsigned int i, j = 0;
  float higher = core[j].process->remaining;

  for(i = 1; i < cores; i++) if(higher < core[i].process->remaining) {
    j = i;
    higher = core[j].process->remaining;
  }

  if(next->remaining < higher) {
    core[j].process->working = False;
    core[j].available = True;
    if(paramd && CPU_ISSET(j, &cpu_set))
      fprintf(stderr, "Process '%s' (remaining time: %f) has been removed from CPU %u\n", core[j].process->name, higher, j);
    context_changes++;
    core[j].process = NULL;
    return 1;
  }
  return 0;
}
