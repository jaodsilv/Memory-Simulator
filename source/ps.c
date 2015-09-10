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
#include "../headers/ps.h"

/*Priority Sched*/
void *ps(void *args)
{
  Process *process = ((Process*) args);
  process->thread = pthread_self();

  /*Coordinator thread*/
  if(process->coordinator) {
    unsigned int i, count = 0, cores = sysconf(_SC_NPROCESSORS_ONLN);
    float quantum = 0;
    Process *next = NULL;
    Rotation *list, *r;
    Core *core;

    CPU_ZERO(&cpu_set);
    for(count = 0; count < cores; count++) CPU_SET(count, &cpu_set);
    for(count = 0; count <= process->total; count++) {
      int ret;
      while(process->process[count].thread == 0) continue;
      if((ret = pthread_setaffinity_np(process->process[count].thread, sizeof(cpu_set), &cpu_set)) != 0) {
        fprintf(stderr, "epsor: pthread set affinity.\n"); exit(EXIT_FAILURE);
      }
      if((ret = pthread_getaffinity_np(process->process[count].thread, sizeof(cpu_set), &cpu_set)) != 0) {
        fprintf(stderr, "epsor: pthread get affinity.\n"); exit(EXIT_FAILURE);
      }
    }
    core = malloc(cores * sizeof(*core));
    list = malloc(sizeof(*list));
    list->process = NULL; list->next = list;
    initialize_cores_ps(core, cores);

    /*Calculates the quantum.*/
    for(i = 0; i < process->total; i++) quantum += process->process[i].duration;
    if(quantum < 1) quantum = 2.0;
    else quantum = sqrt(quantum)/10;

    count = 0;
    /*Initialize simulator globals*/
    context_changes = 0;
    process->context_changes = &context_changes;
    clock_gettime(CLOCK_MONOTONIC, &start_elapsed_time);
    start_cpu_time = clock();
    /*Start simulation*/
    while(count != process->total) {
      if(process->total - count > cores)
        release_cores_ps(process->process, process->total, core, cores, quantum);
      list = fetch_process_ps(process->process, process->total, list);
      next = select_ps(next, list);

      if(next != NULL) use_core_ps(next, core, cores);
      count = finished_processes_ps(process->process, process->total);
      check_cores_available_ps(core, cores);
    }

    /*Free RR circular list*/
    while(count > 0) {
      r = list->next;
      free(list); list = NULL;
      list = r;
      count--;
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
      /*Perform a taks*/
      completed = do_task_ps(process);
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
void use_core_ps(Process *process, Core *core, unsigned int cores)
{
  unsigned int i = 0;
  while(i < cores) {
    if(core[i].available && !process->working && !process->done) {
      core[i].available = False;
      core[i].process = process;
      core[i].process->working = True;
      if(paramd && CPU_ISSET(i, &cpu_set))
        fprintf(stderr, "Process '%s' assigned to CPU %d\n", core[i].process->name, i);
      sem_post(&(core[i].process->next_stage));
      clock_gettime(CLOCK_MONOTONIC, &core[i].timer);
      break;
    }
    else i++;
  }
}

/*System checks if a CPU that was previously in use is available*/
unsigned int check_cores_available_ps(Core *core, unsigned int cores)
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
unsigned int finished_processes_ps(Process *process, unsigned int total)
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
void initialize_cores_ps(Core *core, unsigned int cores)
{
  unsigned int count;
  for(count = 0; count < cores; count++) {
    core[count].available = True;
    core[count].process = NULL;
  }
}

/*Initiate threads to run ps scheduling*/
void do_ps(pthread_t *threads, Process *process, unsigned int *total)
{
  unsigned int i;
  for(i = 0; i <= *total; i++) {
    if(pthread_create(&threads[i], NULL, ps, &process[i])) {
      printf("Epsor creating thread.\n");
      return;
    }
  }
  for(i = 0; i <= *total; i++) {
    if(pthread_join(threads[i], NULL)) {
      printf("Epsor joining process thread.\n");
      return;
    }
  }
}

/*Running process*/
int do_task_ps(Process *process)
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
Rotation *fetch_process_ps(Process *process, unsigned int total, Rotation *list)
{
  Rotation *next;
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
  next = list;
  for(i = 0; i < total; i++)
    if(sec >= process[i].arrival && !process[i].arrived) {
      if(list->process == NULL) {
        list->process = &process[i];
        list->next = list;
      }
      else {
        Rotation *new;
        new = malloc(sizeof(*new));
        if(list->process->priority < process[i].priority) {
          while(next->next != list) next = next->next;
          new->process = &process[i];
          new->next = list;
          next->next = new;
          list = new;
        }
        else {
          while(next->next->process->priority > process[i].priority && next->next != list) {
            next = next->next;
            continue;
          }
          new->process = &process[i];
          new->next = next->next;
          next->next = new;
        }
      }
      next = list;

      process[i].arrived = True;
      sem_post(&(process[i].next_stage));
      if(paramd) fprintf(stderr, "Process '%s' has arrived (trace file line %u)\n", process[i].name, i + 1);
    }
    return list;
}

/*Selects next process to get a CPU*/
Process *select_ps(Process *next, Rotation *list)
{
  if(next == NULL && list->process != NULL) return list->process;
  else if(next != NULL) {
    Rotation *r;
    for(r = list; (r->process->working || r->process->done) && r->next != list; r = r->next) continue;
    if(r->next == list && r->process->working) return NULL;
    else return r->process;
  }
  return NULL;
}

/*Release cores from process that are exceeding the quantum*/
void release_cores_ps(Process *process, unsigned int total, Core *core, unsigned int cores, float quantum)
{
  float sec;
  unsigned int i;
  struct timespec time;

  for(i = 0; i < cores; i++) {
    clock_gettime(CLOCK_MONOTONIC, &time);
    time.tv_sec = time.tv_sec - core[i].timer.tv_sec;
    if(time.tv_nsec > core[i].timer.tv_nsec)
      time.tv_nsec = time.tv_nsec - core[i].timer.tv_nsec;
    else
      time.tv_nsec = core[i].timer.tv_nsec - time.tv_nsec;

    if(core[i].process != NULL && (sec = (float)time.tv_sec + ((float)time.tv_nsec / 1000000000 )) > quantum*(core[i].process->priority + 21)) {
      int j;
      for(j = 0; j < total; j++) if(core[i].process == &process[j]) {
        process[j].working = False;
        if(paramd && CPU_ISSET(i, &cpu_set))
          fprintf(stderr, "Process '%s' has been removed from CPU %u. Quantum time expired (%fs > %fs)\n", core[i].process->name, i, sec, quantum*(core[i].process->priority + 21));
        context_changes++;
        break;
      }
      core[i].available = True;
      core[i].process = NULL;
    }
  }
}
