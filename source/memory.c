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

  /*Initialize semaphore 'safe_access' to protect memory file access and free lists*/
  if(!initialize_mutex()) return;
  /*Assign roles to threads*/
  assign_thread_roles(args, spc, sbs, intrvl);
  /*Create memory files*/
  create_memory(PHYSICAL); create_memory(VIRTUAL);
  /*Initialize simulator*/
  do_simulation(threads, args);
}

/*Run the simulation*/
void *run(void *args)
{
  Thread *thread = ((Thread*) args);

  /*Manager thread*/
  if(thread->role == MANAGER) {
    printf("I am the manager thread!\n");
    /*Wait Timer thread starts the simulation*/
    while(elapsed_time == -1) continue;
    while(simulating) {

      sem_wait(&safe_access_list);
      sem_post(&safe_access_list);
      sem_wait(&safe_access_memory);
      sem_post(&safe_access_memory);

      /*simulating = 0;*/
    }
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

          sem_wait(&safe_access_list);
          /*TODO: Print free list here*/
          sem_post(&safe_access_list);

          /*IDEA: In order to operate considering the initial -1 value, we need to have
          both signed and unsigned integers of size 1b (as the enunciation of the EP commanded),
          this because unsigned goes from [0, 255] and -1 is out of the interval. Thus,
          we need another file pointer as well. Whenever there is a -1 in a position of the
          file, we use the signed values and file pointer to print it, otherwise, we use the
          unsigned ones*/

          sem_wait(&safe_access_memory);
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

/*Write to the binary file in the selected positions 'npos' (in the '*positions' array)
the process 'pid' to register he is using these positions.*/
/*TODO: Still have to test but might be working as it is similar to create_memory and printer thread*/
void write_to_memory(int type, unsigned int *positions, unsigned int npos, uint8_t pid)
{
  FILE *mfile, *mfile_u;
  int8_t *n;
  uint8_t *n_u;

  switch (type) {
    unsigned int i, j;
    case PHYSICAL:
      n = malloc(total * sizeof(*n));
      n_u = malloc(total * sizeof(*n_u));

      /*Read file contents*/
      mfile = fopen("/tmp/ep2.mem", "rb");
      mfile_u = fopen("/tmp/ep2.mem", "rb");
      fread(n, sizeof(int8_t), (size_t)total, mfile);
      fread(n_u, sizeof(uint8_t), (size_t)total, mfile_u);

      /*Process identified by 'pid' is claiming to register new positions*/
      for(i = 0, j = 0; i < total && j < npos; i++) {
        if(n[i] < 0 && i == positions[j]) { n_u[i] = pid; j++; }
        else if(i == positions[j]) { n_u[i] = pid; j++; }
      }

      /*Write claimed positions*/
      fwrite(n_u, sizeof(uint8_t), (size_t)total, mfile_u);
      break;
    case VIRTUAL:
      n = malloc(virtual * sizeof(*n));
      n_u = malloc(virtual * sizeof(*n_u));

      /*Read file contents*/
      mfile = fopen("/tmp/ep2.vir", "rb");
      mfile_u = fopen("/tmp/ep2.vir", "rb");
      fread(n, sizeof(int8_t), (size_t)virtual, mfile);
      fread(n_u, sizeof(uint8_t), (size_t)virtual, mfile_u);

      /*Process identified by 'pid' is claiming to register new positions*/
      for(i = 0, j = 0; i < virtual && j < npos; i++) {
        if(n[i] < 0 && i == positions[j]) { n_u[i] = pid; j++; }
        else if(i == positions[j]) { n_u[i] = pid; j++; }
      }

      /*Write claimed positions*/
      fwrite(n_u, sizeof(uint8_t), (size_t)virtual, mfile_u);
      break;
  }
  fclose(mfile); fclose(mfile_u);
  free(n); n = NULL; free(n_u); n_u = NULL;
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
