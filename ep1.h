#define FCFS 1 /*First Come First Served*/
#define SJF  2 /*Shortest Job First*/
#define SRTN 3 /*Shortest Remaining Time Next*/
#define RR   4 /*Round Robin*/
#define PS   5 /*Priority Scheduling*/
#define RRTS 6 /*Rigid Real Time Scheduling (with rigid deadlines)*/

int run(char **, char *);
char **readtfile(char **, unsigned int *, char *, char *);
void removenl(char *);
void freebuff(char **, unsigned int *);
