/*Free Space Manager*/
#define FF 1      /*First Fit*/
#define NF 2      /*Next Fit*/
#define QF 3      /*Quick Fit*/

/*Page Swap Manager*/
#define NRUP 1    /*Not Recently Used Page*/
#define FIFO 2    /*First-In, First-Out*/
#define SCP  3    /*Second-Chance Page*/
#define LRUP 4    /*Least Recently Used Page*/

typedef int boolean;
#define False 0
#define True 1

#define PAGE_SIZE 16 /* Page size in bytes */

char *get_cmd(char *);
int expand(char *);
int cmd_load(char *, char *);
int cmd_space(char *, char *, int *);
int cmd_subst(char *, char *, int *);
int cmd_exec(char *, char *, float *);
int cmd_exit(char *);
char *get_arg(char *, char *, char *);
void unrecognized(char *);
void free_pointer(void *);
int sucessful_atoi(char *);
int sucessful_atof(char *);
