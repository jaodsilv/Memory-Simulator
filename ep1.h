#define FCFS 1    /*First Come First Served*/
#define SJF  2    /*Shortest Job First*/
#define SRTN 3    /*Shortest Remaining Time Next*/
#define RR   4    /*Round Robin*/
#define PS   5    /*Priority Scheduling*/
#define RRTS 6    /*Rigid Real Time Scheduling (with rigid deadlines)*/

typedef struct process {
  float arrival;  /*The time the process takes to reach the system*/
  char  name[64]; /*The name of the process. Works as an identifier*/
  float duration; /*CPU's real time consumed by the process in the simulation*/
  float deadline; /*The maximum amount of time this process can take to finish*/
  int   priority; /*Priority. An integer in the range [-20, 19]*/
} Process;

int run(char **, char *);
Process *readtfile(Process *, char *, char *, unsigned int *);
void removenl(char *);
void freebuff(char **, unsigned int *);
int isblank(char c);
