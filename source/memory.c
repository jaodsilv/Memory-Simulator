#include <pthread.h>
#include <time.h>
#include <math.h>
#include "../headers/memory.h"

void simulate(int spc, int sbs, float intrvl)
{
  pthread_t threads[3];   /*Manager(position = 0), Printer(position = 1) and Timer(position = 2)*/
  Thread args[3];         /*Arguments of the pthreads*/
  char hand[30];
  total_page_substitutions = 0;

  /*Check if information of the processes are valid*/
  if(!valid_process_information()) return;
  /*Initialize semaphore 'safe_access' to protect memory file access and free lists*/
  if(!initialize_mutex()) return;
  /*Assign roles to threads*/
  assign_thread_roles(args, spc, sbs, intrvl);
  /*Initialize page table*/
  if(virtual % PAGE_SIZE == 0 && total % PAGE_SIZE == 0) initialize_page_table();
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

  printf("\n\n* ~ * ~ * ~ * ~ * ~ * ~ * ~ * ~ *\nSimulation is now over.\nElapsed time: %.2fs.\n", elapsed_time);
  printf("Number of Paginations: %d.\n\n", total_page_substitutions);

  free_heads();
  free(page_table); page_table = NULL;
  free(total_bitmap); total_bitmap = NULL;
  free(virtual_bitmap); virtual_bitmap = NULL;
}

/*Check if the process information from input are correct*/
int valid_process_information()
{
  unsigned int i;
  for(i = 0; i < plength; i++) {
    unsigned int j;
    if(process[i].size > virtual) {
      printf("Error: process '%s' size is greater than the available virtual memory.\n", process[i].name);
      printf("Correct the problem, reload the file with 'carrega' command and then run 'executa' again.\n");
      return 0;
    }
    for(j = 0; j < process[i].length; j++) {
      if(process[i].position[j] >= process[i].size) {
        printf("Error: process '%s' access positions have incorrect value '%u' (must be smaller than process size %u).\n", process[i].name, process[i].position[j], process[i].size);
        printf("Correct the problem, reload the file with 'carrega' command and then run 'executa' again.\n");
        return 0;
      }
      if(process[i].time[j] < process[i].arrival) {
        printf("Error: process '%s' access time have incorrect value '%u' (access time must be greater than the arrival time %u).\n", process[i].name, process[i].time[j], process[i].arrival);
        printf("Correct the problem, reload the file with 'carrega' command and then run 'executa' again.\n");
        return 0;
      }
      if(process[i].time[j] > process[i].finish) {
        printf("Error: process '%s' access time have incorrect value '%u' (access time must be lower than the finishing time %u).\n", process[i].name, process[i].time[j], process[i].finish);
        printf("Correct the problem, reload the file with 'carrega' command and then run 'executa' again.\n");
        return 0;
      }
      if(j > 0 && process[i].time[j - 1] > process[i].time[j]) {
        printf("Error: process '%s' access times are not in ascending ordered.\n", process[i].name);
        printf("Correct the problem, reload the file with 'carrega' command and then run 'executa' again.\n");
        return 0;
      }
      process[i].time[j] -= process[i].arrival;
    }
  }
  return 1;
}

/*Initializes page table array, responsible to do the mapping*/
void initialize_page_table()
{
  unsigned int i;
  tick = 0;
  total_pages = virtual / PAGE_SIZE;
  page_table = malloc(total_pages * sizeof(*page_table));
  for(i = 0; i < total_pages; i++) {
    page_table[i].process = NULL;
    page_table[i].page_frame = FRESH;
    page_table[i].present = false;
    page_table[i].referenced = false;
    page_table[i].modified = false;
  }
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
        if(fit(&process[i], thread->spc)) register_allocation(&process[i]);
        sem_post(&safe_access_list);
      }

      /*Do paging. Will check every page in the page table that is trying to make an access*/
      do_paging(thread->sbs);

      /*Fetch earlier finish*/
      for(i = 0; i < plength; i++) {
        float elapsed = process[i].lifetime;
        if(elapsed >= process[i].duration && process[i].allocated && process[i].index == process[i].length && !process[i].done)
          break;
      }

      /*Access list safely*/
      if(i < plength) {
        sem_wait(&safe_access_list);
        if(unfit(&process[i])) {
          unsigned int j;
          for(j = 0; j < total_pages; j++)
            if(page_table[j].process == &process[i])
              page_table[j].present = false;
          process[i].done = true;
          count++;
        }
        sem_post(&safe_access_list);
      }

      if(count == plength) simulating = 0;
    }
  }

  /*Printer thread*/
  if(thread->role == PRINTER) {
    float last = 0, t = 0, ret;

    /*Wait Timer thread starts the simulation*/
    while(elapsed_time == -1) continue;
    while(simulating) {
      /*IDEA: Assign atomically to ret first and then do a local comparission between ret and last to avoid using a semaphore*/
      ret = elapsed_time;
      if(last != ret) {
        last = ret; t += TIME_GRAIN;
        if(t >= thread->intrvl) { t = 0; print_memory(last); }
      }
    }
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
      if(abs(now.tv_nsec - range.tv_nsec) > 1000000000*TIME_GRAIN) {
        /*IDEA: increment a local 't' to assign time global 'elapsed_time' atomically and avoid the use of a semaphore*/
        t += TIME_GRAIN; elapsed_time = t;
        /*Update allocated processes lifetime*/
        update_allocated_processes();
        /*Update page table timers*/
        update_page_table_times();
        /*Restart range*/
        clock_gettime(CLOCK_MONOTONIC, &range);
      }
    }
  }
  return NULL;
}

/*Free head(s)*/
void free_heads()
{
  if(head[3] != head[0] && head[3] != NULL) free(head[3]); head[3] = NULL;
  if(head[2] != head[0] && head[2] != NULL) free(head[2]); head[2] = NULL;
  if(head[1] != head[0] && head[1] != NULL) free(head[1]); head[1] = NULL;
  if(head[0] != NULL) free(head[0]); head[0] = NULL;
}

/*Least Recently Used*/
/*NOTE: This function is identical to FIFO (and its being kept duplicated here just for didatic purpose),
as LRU also maintain a list of pages. The key difference is in a 'if' statement in the 'do_paging' function.
There it will always increment the tick value of an accessed page,
thus working like 'placing in the end of the list'. This solution was based off the following link:
http://www.mathcs.emory.edu/~cheung/Courses/355/Syllabus/9-virtual-mem/LRU-replace.html*/
void lrup(unsigned int page, unsigned int *loaded_pages, unsigned int size)
{
  unsigned int *positions, candidate_frame, leaving_page = FRESH, i, lower_tick = FRESH;

  /*Select candidate frame to contain the new page. The older page is the one with the lower 'logical tick'*/
  for(i = 0; i < size; i++) {
    if(loaded_pages[i] == FRESH) {
      candidate_frame = i; break;
    }
    else {
      if(page_table[loaded_pages[i]].tick < lower_tick) {
        lower_tick = page_table[loaded_pages[i]].tick;
        candidate_frame = i;
        leaving_page = loaded_pages[i];
      }
    }
  }

  positions = malloc(PAGE_SIZE * sizeof(*positions));
  for(i = 0; i < PAGE_SIZE; i++) positions[i] = (candidate_frame * PAGE_SIZE) + i;

  sem_wait(&safe_access_memory);
  write_to_memory(PHYSICAL, positions, PAGE_SIZE, page_table[page].process->pid);
  sem_post(&safe_access_memory);

  /*Update entering and leaving page in the table*/
  update_page_table(page, leaving_page, candidate_frame);

  free(positions); positions = NULL;
}

/*Second Chance Page*/
void scp(unsigned int page, unsigned int *loaded_pages, unsigned int size)
{
  unsigned int *positions, candidate_frame, leaving_page = FRESH, i, j, lower_tick = FRESH;
  unsigned int first_second_chance_candidate_frame = FRESH, first_second_chance_leaving_page;

  for(j = 0; j < size; j++) {
    /*Select candidate frame to contain the new page*/
    for(i = 0; i < size; i++) {
      if(loaded_pages[i] == FRESH) {
        candidate_frame = i; break;
      }
      else {
        if(page_table[loaded_pages[i]].tick < lower_tick) {
          lower_tick = page_table[loaded_pages[i]].tick;
          candidate_frame = i;
          leaving_page = loaded_pages[i];
        }
      }
    }
    /*Check if this page have a second chance checking reference bit*/
    if(loaded_pages[candidate_frame] != FRESH && page_table[loaded_pages[candidate_frame]].referenced) {
      /*Register the first page to get a second chance. If happens that every page get a second chance, the first one will leave
      This is a strategy to avoid a case where an infinite loop can happen*/
      if(first_second_chance_candidate_frame == FRESH) {
        first_second_chance_candidate_frame = candidate_frame;
        first_second_chance_leaving_page = leaving_page;
      }
      page_table[loaded_pages[candidate_frame]].tick = tick++;
      lower_tick = FRESH;
    }
    else break;
  }

  /*If we had every page with referenced bit on, we remove the older one*/
  if(j == size && first_second_chance_candidate_frame != FRESH) {
    candidate_frame = first_second_chance_candidate_frame;
    leaving_page = first_second_chance_leaving_page;
  }

  positions = malloc(PAGE_SIZE * sizeof(*positions));
  for(i = 0; i < PAGE_SIZE; i++) positions[i] = (candidate_frame * PAGE_SIZE) + i;

  sem_wait(&safe_access_memory);
  write_to_memory(PHYSICAL, positions, PAGE_SIZE, page_table[page].process->pid);
  sem_post(&safe_access_memory);

  /*Update entering and leaving page in the table*/
  update_page_table(page, leaving_page, candidate_frame);

  free(positions); positions = NULL;
}

/*First In First Out*/
void fifo(unsigned int page, unsigned int *loaded_pages, unsigned int size)
{
  unsigned int *positions, candidate_frame, leaving_page = FRESH, i, lower_tick = FRESH;

  /*Select candidate frame to contain the new page. The older page is the one with the lower 'logical tick'*/
  for(i = 0; i < size; i++) {
    if(loaded_pages[i] == FRESH) {
      candidate_frame = i; break;
    }
    else {
      if(page_table[loaded_pages[i]].tick < lower_tick) {
        lower_tick = page_table[loaded_pages[i]].tick;
        candidate_frame = i;
        leaving_page = loaded_pages[i];
      }
    }
  }

  positions = malloc(PAGE_SIZE * sizeof(*positions));
  for(i = 0; i < PAGE_SIZE; i++) positions[i] = (candidate_frame * PAGE_SIZE) + i;

  sem_wait(&safe_access_memory);
  write_to_memory(PHYSICAL, positions, PAGE_SIZE, page_table[page].process->pid);
  sem_post(&safe_access_memory);

  /*Update entering and leaving page in the table*/
  update_page_table(page, leaving_page, candidate_frame);

  free(positions); positions = NULL;
}

/*Not Recently Used Page*/
void nrup(unsigned int page, unsigned int *loaded_pages, unsigned int size)
{
  unsigned int *positions, candidate_frame, leaving_page = FRESH, i, class = 4;

  /*Select candidate frame to contain the new page*/
  for(i = 0; i < size; i++) {
    if(loaded_pages[i] == FRESH) {
      candidate_frame = i; break;
    }
    else {
      /*Rank the pages and check final score. The score will determine the leaving page*/
      unsigned int cl = 0;
      if(page_table[loaded_pages[i]].referenced) cl += 2;
      if(page_table[loaded_pages[i]].modified) cl += 1;
      if(cl < class) {
        class = cl;
        candidate_frame = i;
        leaving_page = loaded_pages[i];
      }
      else if(cl == class) {
        float roll;
        srand(time(NULL));
        roll = ((float)(rand() % 101)) / 100;
        /*50% chance to swap candidate_frame in case of a draw*/
        if(roll < 0.5) {
          candidate_frame = i;
          leaving_page = loaded_pages[i];
        }
      }
    }
  }

  positions = malloc(PAGE_SIZE * sizeof(*positions));
  for(i = 0; i < PAGE_SIZE; i++) positions[i] = (candidate_frame * PAGE_SIZE) + i;

  sem_wait(&safe_access_memory);
  write_to_memory(PHYSICAL, positions, PAGE_SIZE, page_table[page].process->pid);
  sem_post(&safe_access_memory);

  /*Update entering and leaving page in the table*/
  update_page_table(page, leaving_page, candidate_frame);

  free(positions); positions = NULL;
}

/*Updates page table entries for the page leaving a frame and the page entering a frame*/
void update_page_table(unsigned int page, unsigned int leaving_page, unsigned int candidate_frame)
{
  /*If there is a leaving page*/
  if(leaving_page != FRESH) {
    page_table[leaving_page].page_frame = FRESH;
    page_table[leaving_page].loaded_time = 0;
    page_table[leaving_page].present = false;
    page_table[leaving_page].referenced = false;
    page_table[leaving_page].modified = false;
  }
  /*Now the new loaded page*/
  page_table[page].page_frame = candidate_frame;
  page_table[page].loaded_time = elapsed_time;
  page_table[page].tick = tick++;
  page_table[page].present = true;
  page_table[page].referenced = false;
  page_table[page].modified = false;
}

/*Select page substitution algorithm and substitute a page from the physical memory
for one from the virtual*/
void do_page_substitution(unsigned int page, int substitution_number)
{
  unsigned int *loaded_pages, size, i;

  loaded_pages = malloc((size = total / PAGE_SIZE) * sizeof(*loaded_pages));
  for(i = 0; i < size; i++) loaded_pages[i] = FRESH;

  for(i = 0; i < total_pages; i++)
    if(page_table[i].present)
      loaded_pages[page_table[i].page_frame] = i;

  switch (substitution_number) {
    case NRUP:
        nrup(page, loaded_pages, size);
      break;
    case FIFO:
        fifo(page, loaded_pages, size);
      break;
    case SCP:
        scp(page, loaded_pages, size);
      break;
    case LRUP:
        lrup(page, loaded_pages, size);
      break;
  }

  ++total_page_substitutions;
  free(loaded_pages); loaded_pages = NULL;
}

/*Do paging if an access attempt is detected*/
void do_paging(int substitution_number)
{
  unsigned int i;
  for(i = 0; i < total_pages; i++) {
    if(page_table[i].process == NULL) continue;
    else {
      float page_time = page_table[i].time;
      unsigned int access_time = (page_table[i].process)->time[(page_table[i].process)->index];
      float process_elapsed_time = (page_table[i].process)->lifetime;
      /*Check if the process wants to acess a page frame*/
      if(access_time <= process_elapsed_time && !((page_table[i].process)->done)) {
        /*PAGE FAULT! It must be loaded into a page frame*/
        if(!page_table[i].present) {
          /*Page substitution*/
          do_page_substitution(i, substitution_number);
        }
        /*Seeks for the content in the page frame*/
        if(page_table[i].present) {
          unsigned int positions[1];
          unsigned int wanted_position = (page_table[i].process)->position[(page_table[i].process)->index];

          /*Compute the position in the physical memory (map from virtual to physical)*/
          positions[0] = (page_table[i].page_frame * PAGE_SIZE) + wanted_position;
          /*Access the wanted positions*/
          sem_wait(&safe_access_memory);
          write_to_memory(PHYSICAL, positions, 1, (page_table[i].process)->pid);
          sem_post(&safe_access_memory);

          /*IDEA: Decide if the action is write or read using a rand call. If write, modified = true, else, modified = false*/
          page_table[i].modified = true;
          page_table[i].referenced = true;
          /*Next action*/
          if((page_table[i].process)->index < (page_table[i].process)->length)
            (page_table[i].process)->index += 1;
          /*reset timer*/
          page_table[i].time = 0;
          /*LRU algorithm increments 'logical tick' in every reference, due to the assumption that a page
          being used at present will be used again in the next future*/
          if(substitution_number == LRUP) page_table[i].tick = tick++;
        }
      }
      /*Have a long time already since this page isn't being referenced*/
      if(page_time >= 3) page_table[i].referenced = false;
    }
  }
}

/*Update page timer for allocated process*/
void update_page_table_times()
{
  unsigned int i;
  for(i = 0; i < total_pages; i++) {
    Process *process = page_table[i].process;
    if(process != NULL) {
      float page_time_elapsed = page_table[i].time;
      page_time_elapsed += TIME_GRAIN; page_table[i].time = page_time_elapsed;
    }
  }
}

/*Update allocated processes lifetime*/
void update_allocated_processes()
{
  unsigned int i;
  for(i = 0; i < plength; i++) {
    bool allocated = process[i].allocated, done = process[i].done;
    if(allocated && !done) {
      float process_time_elapsed = process[i].lifetime;
      process_time_elapsed += TIME_GRAIN; process[i].lifetime = process_time_elapsed;
    }
  }
}

/*Register allocated process information, writing information to virtual memory and updating page table*/
void register_allocation(Process *process)
{
  Free_List *p;
  unsigned int *positions, npos, j;
  process->allocated = true;

  /*Must write in the virtual binary file*/
  p = head[j = 0]; while(p != NULL && j < 4) {
    if(p->process != NULL && process->pid == p->process->pid) {
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
  write_to_memory(VIRTUAL, positions, npos, process->pid);
  /*Assign to process table*/
  assign_process_to_page_table(p);
  sem_post(&safe_access_memory);
  free(positions); positions = NULL;
}

/*Write to the binary file in the selected positions 'npos' (in the '*positions' array)
the process 'pid' to register he is using these positions.*/
void write_to_memory(int type, unsigned int *positions, unsigned int npos, uint8_t pid)
{
  unsigned int i, j;
  FILE *mfile, *mfile_u;
  int8_t *array;
  uint8_t *array_u;

  switch (type) {
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
      for(i = 0; i < total; i++) {
        if(total_bitmap[i] == -1) {
          fwrite(&array[i], sizeof(array[i]), (size_t)1, mfile_u);
        }
        else {
          fwrite(&array_u[i], sizeof(array_u[i]), (size_t)1, mfile_u);
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
      fclose(mfile);
      break;
    case VIRTUAL:
      n2 = malloc(virtual * sizeof(*n2));
      for(i = 0; i < virtual; i++) n2[i] = -1;
      mfile = fopen("/tmp/ep2.vir", "wb");
      fwrite(n2, sizeof(n2[0]), virtual * sizeof(n2[0]), mfile);
      virtual_bitmap = malloc(virtual * sizeof(*virtual_bitmap));
      for(i = 0; i < virtual; i++) virtual_bitmap[i] = -1;
      free(n2); n2 = NULL;
      fclose(mfile);
      break;
  }
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
void assign_process_to_page_table(Free_List *fl)
{
  unsigned int i, page_table_index, number_of_pages;

  page_table_index = get_page_number(fl);
  number_of_pages = get_amount_of_pages(fl->process->size);

  for(i = 0; i < number_of_pages; i++) {
    page_table[page_table_index].process = fl->process;
    page_table[page_table_index].page = page_table_index;
    page_table[page_table_index].page_frame = FRESH;
    page_table[page_table_index].time = 0;
    page_table[page_table_index].loaded_time = -1;
    page_table[page_table_index].present = false;
    page_table[page_table_index].referenced = false;
    page_table[page_table_index].modified = false;
    page_table_index++;
  }
}

/*Get amount of pages needed*/
unsigned int get_amount_of_pages(unsigned int size)
{
  unsigned int number_of_pages;
  return (number_of_pages = ((size - 1) / PAGE_SIZE) + 1);
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
    for(p = head[0]; p != NULL && p->process != process; p = p->next) continue;
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
      f = ff(process->size);
      ret = memory_allocation(f, process);
      break;
    case NF:
      f = nf(process->size);
      ret = memory_allocation(f, process);
      break;
    case QF:
      f = qf(process->size);
      ret = memory_allocation(f, process);
      break;
  }
  return ret;
}

/*Returns a free space where the process can fit in, according to FF policy*/
Free_List *ff(unsigned int size)
{
  Free_List *p;

  for(p = head[0]; p != NULL; p = p->next)
    if(p->process == NULL && p->limit >= size) return p;
  return NULL;
}

/*Returns a free space where the process can fit in, according to NF policy*/
Free_List *nf(unsigned int size)
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
Free_List *qf(unsigned int size)
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


/*Print memory and linked list. This is basically everything the printer thread does*/
void print_memory(float last)
{
  Free_List *p;
  unsigned int i;
  FILE *mfile, *mfile_u;
  int8_t *physical_array, *virtual_array;
  uint8_t *physical_array_u, *virtual_array_u;

  physical_array = malloc(total * sizeof(*physical_array));
  virtual_array = malloc(virtual * sizeof(*virtual_array));
  physical_array_u = malloc(total * sizeof(*physical_array_u));
  virtual_array_u = malloc(virtual * sizeof(*virtual_array_u));

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
  fclose(mfile); fclose(mfile_u);

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

  free(physical_array); physical_array = NULL;
  free(virtual_array); virtual_array = NULL;
  free(physical_array_u); physical_array_u = NULL;
  free(virtual_array_u); virtual_array_u = NULL;
}
