cpu_set_t cpu_set;

typedef struct core {
  boolean available;       /*Is this core available?*/
  Process *process;        /*If the core is busy, register the process using this core*/
  struct timespec timer;   /*Time a process can use the CPU*/
} Core;

typedef struct rotation {
  Process *process;        /*Actual process*/
  struct rotation *next;   /*Next process in the circular list*/
} Rotation;
