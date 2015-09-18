/*Free Space Manager*/
#define FF 1      /*First Fit*/
#define NF 2      /*Next Fit*/
#define QF 3      /*Quick Fit*/

/*Page Swap Manager*/
#define NRUP 1    /*Not Recently Used Page*/
#define FIFO 2    /*First-In, First-Out*/
#define SCP  3    /*Second-Chance Page*/
#define LRUP 4    /*Least Recently Used Page*/

typedef struct process {
  pthread_t thread;            /*The thread this process is*/
  bool coordinator;         /*Is this a coordinator thread?*/
  float   arrival;             /*The time the process takes to reach the system*/
  bool arrived;             /*Have the process arrived?*/
  char   name[64];             /*The name of the process. Works as an identifier*/
  float  duration;             /*CPU's real time consumed by the process in the simulation*/
  float  remaining;            /*Used by a few scheduling policies such as round robin to register remaining time*/
  float  deadline;             /*The maximum amount of time this process can take to finish*/
  float  finish_cpu_time;      /*The simulation cpu time the process finished his task*/
  float  finish_elapsed_time;  /*The simulation elapsed time the process finished his task*/
  int    priority;             /*Priority. An integer in the range [-20, 19]*/
  bool working;             /*Process is running in a CPU?*/
  bool failed;              /*Process failed finishing execution before its deadline?*/
  bool    done;             /*CPU done with the process?*/
  sem_t next_stage;            /*Stages of a process: Arrive -> working -> done*/
  pthread_mutex_t mutex;       /*Safe reading/writing 'done' variable*/
  /*Used only by the coordinator.
    For i processes, the coordinator is at the index i and the rest of the processes
    are at indexes 0 to i - 1 of the array pointes by *process*/
  unsigned int total;          /*Total number of processes running*/
  struct process *process;     /*All the processes*/
  unsigned int *context_changes;
} Process;

/*Simulator Globals*/
clock_t start_cpu_time;       /*Simulator initial cpu time*/
clock_t finish_cpu_time;      /*Simulator final cpu time*/
struct timespec start_elapsed_time;  /*Real time*/
struct timespec finish_elapsed_time; /*Real time*/
boolean paramd;               /*Register if the 4th optional parameter was passed*/
unsigned int context_changes; /*Times the simulator made a context change*/

/*Simulator functions prototypes*/
int run(std::string, int, int, int);
Process *read_trace_file(Process *, char *, char *, unsigned int *);
void write_output(Process *, char *, char *, unsigned int *);
int initialize_mutex(Process *, unsigned int *);
void free_mutex(Process *, unsigned int *);
int is_blank(char c);
