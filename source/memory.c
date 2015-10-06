#include <pthread.h>
#include <time.h>
#include <math.h>
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

  /*Initialize semaphore 'safe_access' to protect memory file access and free lists*/
  if(!initialize_mutex()) return;
  /*Assign roles to threads*/
  assign_thread_roles(args, spc, sbs, intrvl);
  /*Initialize page table*/
  if(virtual % PAGE_SIZE == 0 && total % PAGE_SIZE == 0) initialize_process_table();
  else {
    printf("Error: was expecting both memory sizes to be a multiple of 16. Encountered total = %u and virtual = %u.\n", total, virtual);
    return;
  }
  /*Create memory files*/
  create_memory(PHYSICAL); create_memory(VIRTUAL);
  /*Initialize a free list*/
  initialize_free_list();
  /*Initialize simulator*/
  do_simulation(threads, args);
  printf("\n\n* ~ * ~ * ~ * ~ * ~ * ~ * ~ * ~ *\nSimulation is now over.\nElapsed time: %.1fs\n\n", elapsed_time);
}

/*Initializes page table array, responsible to do the mapping*/
void initialize_process_table()
{
  total_pages = virtual / PAGE_SIZE;
  ptable = malloc(total_pages * sizeof(*ptable));
}

/*Run the simulation*/
void *run(void *args)
{
  Thread *thread = ((Thread*) args);

  /*Manager thread*/
  if(thread->role == MANAGER) {
    float t; unsigned int count = 0;
    /*Wait Timer thread starts the simulation*/
    while(elapsed_time == -1) continue;
    while(simulating) {
      unsigned int i;

      /*Get time*/
      t = elapsed_time;

      /*Fetch arrivals*/
      for(i = 0; i < plength; i++)
        if(process[i].arrival <= t && !process[i].allocated)
          break;

      /*Access list safely*/
      if(i < plength) {
        sem_wait(&safe_access_list);
        if(fit(&process[i], thread->spc)) {
          Free_List *p;
          unsigned int *positions, npos, j;
          process[i].allocated = true;

          /*Must write in the virtual binary file*/
          p = head[j = 0]; while(p != NULL && j < 4) {
            if(process[i].pid == p->process->pid) {
              unsigned int k;
              positions = malloc((npos = p->limit) * sizeof(*positions));
              for(k = p->base; k < p->base + p->limit; k++) positions[k - p->base] = k;
              break;
            }
            if(p->next == NULL && j < 4) { p = head[++j]; continue; }
            p = p->next;
          }
          /*Access memory safely*/
          sem_wait(&safe_access_memory);
          /*Write pid in virtual file*/
          write_to_memory(VIRTUAL, positions, npos, process[i].pid);
          /*Assign to process table*/
          assign_process_to_process_table(p);
          sem_post(&safe_access_memory);
          free(positions); positions = NULL;
        }
        sem_post(&safe_access_list);
      }

      /*Access memory safely*/
      sem_wait(&safe_access_memory);
      /*TODO: check if there is an available page frame and assign a page using the process table to it (= write pid in the right positions)*/

      sem_post(&safe_access_memory);

      /*Fetch earlier finish*/
      for(i = 0; i < plength; i++) {
        float elapsed = process[i].lifetime;
        if(elapsed >= process[i].duration && process[i].allocated && process[i].index == process[i].length)
          break;
      }

      /*Access list safely*/
      if(i < plength) {
        sem_wait(&safe_access_list);
        if(unfit(&process[i])) {
          process[i].done = true;
          count++;
        }
        sem_post(&safe_access_list);
      }

      if(count == plength) simulating = 0;
    }
    if(head[3] != head[0] && head[3] != NULL) free(head[3]); head[3] = NULL;
    if(head[2] != head[0] && head[2] != NULL) free(head[2]); head[2] = NULL;
    if(head[1] != head[0] && head[1] != NULL) free(head[1]); head[1] = NULL;
    if(head[0] != NULL) free(head[0]); head[0] = NULL;
    free(ptable); ptable = NULL;
    free(total_bitmap); total_bitmap = NULL;
    free(virtual_bitmap); virtual_bitmap = NULL;
  }

  /*Printer thread*/
  if(thread->role == PRINTER) {
    FILE *mfile, *mfile_u;
    float last = 0, t = 0, ret;
    int8_t *physical_array, *virtual_array;
    uint8_t *physical_array_u, *virtual_array_u;

    physical_array = malloc(total * sizeof(*physical_array));
    virtual_array = malloc(virtual * sizeof(*virtual_array));
    physical_array_u = malloc(total * sizeof(*physical_array_u));
    virtual_array_u = malloc(virtual * sizeof(*virtual_array_u));
    /*Wait Timer thread starts the simulation*/
    while(elapsed_time == -1) continue;
    while(simulating) {
      /*IDEA: Assign atomically to ret first and then do a local comparission between ret and last to avoid using a semaphore*/
      ret = elapsed_time;
      if(last != ret) {
        last = ret; t += 0.1;
        if(t >= thread->intrvl) {
          Free_List *p;
          unsigned int i; t = 0;

          printf("\nTIME: %.1fs\n", last);
          sem_wait(&safe_access_list);
          printf("Free List state [process name (pid),  base, limit]:\n");
          p = head[i = 0]; while(p != NULL && i < 4) {
            if(i == 0 || (i > 0 && head[i] != head[0])) {
              if(p->process != NULL) printf("[%s (%u), %u, %u]", p->process->name, p->process->pid, p->base, p->limit);
              else printf("[free, %u, %u]", p->base, p->limit);
              if(p->next != NULL) printf(" -> ");
            }
            if(p->next == NULL && i < 4) { p = head[++i]; continue; }
            p = p->next;
          }
          printf("\n");
          sem_post(&safe_access_list);

          /*IDEA: In order to operate considering the initial -1 value, we need to have
          both signed and unsigned integers of size 1b (as the enunciation of the EP commanded),
          this because unsigned goes from [0, 255] and -1 is out of the interval. Thus,
          we need another file pointer as well. Whenever there is a -1 in a position of the
          file, we use the signed values and file pointer to print it, otherwise, we use the
          unsigned ones*/

          sem_wait(&safe_access_memory);
          printf("Physical memory state (binary file):\n");
          /*Read physical binary file*/
          mfile = fopen("/tmp/ep2.mem", "rb");
          mfile_u = fopen("/tmp/ep2.mem", "rb");
          /*Prints physical memory file*/
          for(i = 0; i < total; i++) {
            fread(&physical_array[i], sizeof(physical_array[i]), 1, mfile);
            fread(&physical_array_u[i], sizeof(physical_array_u[i]), 1, mfile_u);
            if(total_bitmap[i] == -1) {
              printf("%d ", physical_array[i]);
              continue;
            }
            else {
              printf("%u ", physical_array_u[i]);
            }
          } printf("\n");

          printf("Virtual memory state (binary file):\n");
          /*Read virtual binary file*/
          mfile = fopen("/tmp/ep2.vir", "rb");
          mfile_u = fopen("/tmp/ep2.vir", "rb");
          /*Prints virtual memory file*/
          for(i = 0; i < virtual; i++) {
            fread(&virtual_array[i], sizeof(virtual_array[i]), 1, mfile);
            fread(&virtual_array_u[i], sizeof(virtual_array_u[i]), 1, mfile_u);
            if(virtual_bitmap[i] == -1) {
              printf("%d ", virtual_array[i]);
              continue;
            }
            else {
              printf("%u ", virtual_array_u[i]);
            }
          } printf("\n");

          fclose(mfile); fclose(mfile_u);
          sem_post(&safe_access_memory);
        }
      }
    }
    free(physical_array); physical_array = NULL;
    free(virtual_array); virtual_array = NULL;
    free(physical_array_u); physical_array_u = NULL;
    free(virtual_array_u); virtual_array_u = NULL;
  }

  /*Timer thread*/
  if(thread->role == TIMER) {
    float t = 0;
    struct timespec now, range;
    /*Starts simulation*/
    elapsed_time = 0; clock_gettime(CLOCK_MONOTONIC, &range);
    while(simulating) {
      clock_gettime(CLOCK_MONOTONIC, &now);
      /*t is incremented by 0.1 every 0.1s*/
      if(abs(now.tv_nsec - range.tv_nsec) > 100000000) {
        unsigned int i;
        /*IDEA: increment a local 't' to assign time global 'elapsed_time' atomically and avoid the use of a semaphore*/
        t += 0.1; elapsed_time = t;
        /*Update allocated processes lifetime*/
        for(i = 0; i < plength; i++) {
          bool allocated = process[i].allocated, done = process[i].done;
          if(allocated && !done) {
            float process_time_elapsed = process[i].lifetime;
            process_time_elapsed += 0.1; process[i].lifetime = process_time_elapsed;
          }
        }
        /*Restart range*/
        clock_gettime(CLOCK_MONOTONIC, &range);
      }
    }
  }
  return NULL;
}

/*Write to the binary file in the selected positions 'npos' (in the '*positions' array)
the process 'pid' to register he is using these positions.*/
void write_to_memory(int type, unsigned int *positions, unsigned int npos, uint8_t pid)
{
  FILE *mfile, *mfile_u;
  int8_t *array;
  uint8_t *array_u;

  switch (type) {
    unsigned int i, j;
    case PHYSICAL:
      array = malloc(total * sizeof(*array));
      array_u = malloc(total * sizeof(*array_u));

      /*Get contents of binary file*/
      mfile = fopen("/tmp/ep2.mem", "rb");
      mfile_u = fopen("/tmp/ep2.mem", "rb");
      fread(array, sizeof(array[i]), total, mfile);
      fread(array_u, sizeof(array_u[i]), total, mfile_u);
      fclose(mfile); fclose(mfile_u);

      /*Modify array of unsigneds*/
      for(i = 0, j = 0; i < total && j < npos; i++)
        if(i == positions[j]) {
          array_u[i] = pid; j++;
          if(total_bitmap[i] == -1) total_bitmap[i] = 1;
        }

      /*Open file in writing mode*/
      mfile = fopen("/tmp/ep2.mem", "wb");
      mfile_u = fopen("/tmp/ep2.mem", "wb");

      /*Modify file*/
      for(i = 0; i < virtual; i++) {
        if(total_bitmap[i] == -1) {
          fwrite(&array[i], sizeof(array[i]), 1, mfile_u);
        }
        else {
          fwrite(&array_u[i], sizeof(array_u[i]), 1, mfile_u);
        }
      }
      fclose(mfile); fclose(mfile_u);
      break;
    case VIRTUAL:
      array = malloc(virtual * sizeof(*array));
      array_u = malloc(virtual * sizeof(*array_u));

      /*Get contents of binary file*/
      mfile = fopen("/tmp/ep2.vir", "rb");
      mfile_u = fopen("/tmp/ep2.vir", "rb");
      fread(array, sizeof(array[i]), virtual, mfile);
      fread(array_u, sizeof(array_u[i]), virtual, mfile_u);
      fclose(mfile); fclose(mfile_u);

      /*Modify array of unsigneds*/
      for(i = 0, j = 0; i < virtual && j < npos; i++)
        if(i == positions[j]) {
          array_u[i] = pid; j++;
          if(virtual_bitmap[i] == -1) virtual_bitmap[i] = 1;
        }

      /*Open file in writing mode*/
      mfile = fopen("/tmp/ep2.vir", "wb");
      mfile_u = fopen("/tmp/ep2.vir", "wb");

      /*Modify file*/
      for(i = 0; i < virtual; i++) {
        if(virtual_bitmap[i] == -1) {
          fwrite(&array[i], sizeof(array[i]), 1, mfile_u);
        }
        else {
          fwrite(&array_u[i], sizeof(array_u[i]), 1, mfile_u);
        }
      }
      fclose(mfile); fclose(mfile_u);
      break;
  }
  free(array); array = NULL; free(array_u); array_u = NULL;
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
      total_bitmap = malloc(total * sizeof(*total_bitmap));
      for(i = 0; i < total; i++) total_bitmap[i] = -1;
      free(n1); n1 = NULL;
      break;
    case VIRTUAL:
      n2 = malloc(virtual * sizeof(*n2));
      for(i = 0; i < virtual; i++) n2[i] = -1;
      mfile = fopen("/tmp/ep2.vir", "wb");
      fwrite(n2, sizeof(n2[0]), virtual * sizeof(n2[0]), mfile);
      virtual_bitmap = malloc(virtual * sizeof(*virtual_bitmap));
      for(i = 0; i < virtual; i++) virtual_bitmap[i] = -1;
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

/*Initiates the free list, returning the first cell.*/
void initialize_free_list()
{
  Free_List *fl;
  fl = malloc(sizeof(*fl));
  fl->process = NULL;
  fl->previous = NULL;
  fl->next = NULL;
  fl->base = 0;
  fl->limit = virtual;
  nf_next = fl;
  head[0] = fl;
  head[1] = head[0];
  head[2] = head[0];
  head[3] = head[0];
}

/*Add process to process table*/
void assign_process_to_process_table(Free_List *fl)
{
  unsigned int i, process_table_index, number_of_pages;

  process_table_index = get_page_number(fl);
  number_of_pages = get_amount_of_pages(fl->process->size);

  for(i = 0; i < number_of_pages; i++) {
    ptable[process_table_index].process = fl->process;
    ptable[process_table_index].page = process_table_index;
    process_table_index++;
  }
}

/*Get amount of pages needed*/
unsigned int get_amount_of_pages(unsigned int size)
{
  unsigned int number_of_pages;
  return (number_of_pages = (size / PAGE_SIZE) + 1);
}

/*Get the page number. The page number is the index in the page table for the process allocated with base fl->base.*/
unsigned int get_page_number(Free_List *fl)
{
  unsigned int page_number;
  return (page_number = fl->base / PAGE_SIZE);
}

/*Allocates memory for a new process in the free_list. Don't call this function!
  Call "fit" function to do the job.*/
int memory_allocation(Free_List *fl, Process *process)
{
  Free_List *new;
  unsigned int old_limit;

  /*Allocated cell*/
  if(fl != NULL && fl->process == NULL) {
    old_limit = fl->limit;
    fl->process = process;
    fl->limit = PAGE_SIZE * get_amount_of_pages(process->size);
  }
  else {
    /*Bad cell selected by fetch algorithm.*/
    if(fl != NULL)
      fprintf(stderr, "Error: bad fetch. Was expecting an available memory space.\n");
    return 0;
  }


  /*Remaining free space. There will be no remaining free space
  from the old cell if fl->limit == process->size*/
  if(old_limit > process->size && (old_limit - fl->limit > 0)) {
    new = malloc(sizeof(*new));
    new->previous = fl;
    new->process = NULL;
    new->next = fl->next;
    if(fl->next != NULL) fl->next->previous = new;
    fl->next = new;
    new->base = fl->base + fl->limit;
    new->limit = old_limit - fl->limit;
  }

  return 1;
}

/*Frees memory from the free_list previously allocated by process *process.
*fl is the pointer to the first cell of the free_list
Note: this function DOES NOT disallocates the process
*/
int unfit(Process *process)
{
  Free_List *p;

  /*Fetch process*/
  if(head[0] == head[1] && head[0] == head[2] && head[0] == head[3])
    for(p = head[0]; p->process != process && p != NULL; p = p->next) continue;
  else {
    if(process->size > 256) for(p = head[3]; p->process != process && p != NULL; p = p->next) continue;
    if(process->size > 64)  for(p = head[2]; p->process != process && p != NULL; p = p->next) continue;
    if(process->size > 16)  for(p = head[1]; p->process != process && p != NULL; p = p->next) continue;
    if(process->size <= 16) for(p = head[0]; p->process != process && p != NULL; p = p->next) continue;
  }

  if(p == NULL) {
    /*Failed free. Process not found*/
    fprintf(stderr, "Error: failed to find allocated process.\n");
    return 0;
  }

  /*Disallocate process P*/
  /*If in the middle of the list (not tail and not head)*/
  if(p->previous != NULL && p->next != NULL) {
    /*Case: process X | process P | process Y*/
    if(p->previous->process != NULL && p->next->process != NULL)
      p->process = NULL;
    /*Case: free | process P | free*/
    else if(p->previous->process == NULL && p->next->process == NULL) {
      Free_List *k;
      if(p->previous == head[0]) {
        if(head[0] == head[3]) head[3] = p;
        if(head[0] == head[2]) head[2] = p;
        if(head[0] == head[1]) head[1] = p;
        head[0] = p;
      }
      else if(p->previous == head[1]) head[1] = p;
      else if(p->previous == head[2]) head[2] = p;
      else if(p->previous == head[3]) head[3] = p;
      if(p->previous == nf_next) nf_next = p;

      p->base = p->previous->base;
      p->limit += (p->previous->limit + p->next->limit);
      p->process = NULL;
      if(p->previous->previous != NULL) p->previous->previous->next = p;
      if(p->next->next != NULL) p->next->next->previous = p;
      k = p->next;
      p->next = p->next->next;
      free(k); k = p->previous;
      p->previous = p->previous->previous;
      free(k); k = NULL;
    }
    /*Case: process X | process P | free*/
    else if(p->previous->process != NULL && p->next->process == NULL) {
      if(nf_next == p) nf_next = p->next;
      p->next->base = p->base;
      p->next->limit += p->limit;
      p->next->previous = p->previous;
      p->previous->next = p->next;
      free(p); p = NULL;
    }
    /*Case: free | process P | Process Y*/
    else if(p->next->process != NULL && p->previous->process == NULL) {
      if(nf_next == p) nf_next = p->previous;
      p->previous->limit += p->limit;
      p->previous->next = p->next;
      p->next->previous = p->previous;
      free(p); p = NULL;
    }
  }
  /*If is the head*/
  else if(p->previous == NULL && p->next != NULL) {
    /*Case: Process P | Process Y*/
    if(p->next->process != NULL)
      p->process = NULL;
    /*Case: Process P | free*/
    else if(p->next->process == NULL) {
      /*Select the next as the new head*/
      if(p == head[0]) {
        if(head[0] == head[3]) head[3] = p->next;
        if(head[0] == head[2]) head[2] = p->next;
        if(head[0] == head[1]) head[1] = p->next;
        head[0] = p->next;
      }
      else if(p == head[1]) head[1] = p->next;
      else if(p == head[2]) head[2] = p->next;
      else if(p == head[3]) head[3] = p->next;
      if(nf_next == p) nf_next = p->next;

      p->next->base = p->base;
      p->next->limit += p->limit;
      p->next->previous = p->previous;
      free(p); p = NULL;
    }
  }
  /*If is the tail*/
  else if(p->previous != NULL && p->next == NULL) {
    /*Case: Process X | Process P*/
    if(p->previous->process != NULL)
      p->process = NULL;
    /*Case: free | Process P*/
    else if(p->previous->process == NULL) {
      if(nf_next == p) nf_next = p->previous;
      p->previous->limit += p->limit;
      p->previous->next = p->next;
      free(p); p = NULL;
    }
  }

  return 1;
}

/*Attempts to fit a process into the free_list using the correct fetch algorithm*/
int fit(Process *process, int fit_number)
{
  int ret;

  switch(fit_number) {
    Free_List *f;
    case FF:
      f = fetch_ff(process->size);
      ret = memory_allocation(f, process);
      break;
    case NF:
      f = fetch_nf(process->size);
      ret = memory_allocation(f, process);
      break;
    case QF:
      f = fetch_qf(process->size);
      ret = memory_allocation(f, process);
      break;
  }
  return ret;
}

/*Returns a free space where the process can fit in, according to FF policy*/
Free_List *fetch_ff(unsigned int size)
{
  Free_List *p;

  for(p = head[0]; p != NULL; p = p->next)
    if(p->process == NULL && p->limit >= size) return p;
  return NULL;
}

/*Returns a free space where the process can fit in, according to NF policy*/
Free_List *fetch_nf(unsigned int size)
{
  Free_List *p; bool tail = false; p = nf_next;
  while(p != nf_next || !tail) {
    if(p->process == NULL && p->limit >= size) return nf_next = p;
    if(p->next == NULL) {
      p = head[0]; tail = true; continue;
    }
    p = p->next;
  }
  return NULL;
}

/*Returns a free space where the process can fit in, according to QF policy*/
Free_List *fetch_qf(unsigned int size)
{
  Free_List *p;

  if(size > 256)
    for(p = head[3];p != NULL; p = p->next)
      if(p->process == NULL && p->limit >= size) return p;
  if(size > 64)
    for(p = head[2]; p != NULL; p = p->next)
      if(p->process == NULL && p->limit >= size) return p;
  if(size > 16)
    for(p = head[1]; p != NULL; p = p->next)
      if(p->process == NULL && p->limit >= size) return p;
  if(size <= 16)
    for(p = head[0]; p != NULL; p = p->next)
      if(p->process == NULL && p->limit >= size) return p;
  return NULL;
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

/*Initialize mutexes*/
int initialize_mutex()
{
  if(sem_init(&safe_access_memory, 0, 1) == -1) {
    printf("Error initializing semaphore.\n");
    return 0;
  }
  if(sem_init(&safe_access_list, 0, 1) == -1) {
    printf("Error initializing semaphore.\n");
    return 0;
  }
  return 1;
}
