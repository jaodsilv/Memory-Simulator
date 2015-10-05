#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/*Free space*/
#define FF 1      /*First Fit*/
#define NF 2      /*Next Fit*/
#define QF 3      /*Quick Fit*/

/*Page substitution*/
#define NRUP 1    /*Not Recently Used Page*/
#define FIFO 2    /*First-In, First-Out*/
#define SCP  3    /*Second-Chance Page*/
#define LRUP 4    /*Least Recently Used Page*/

/* Each PAGE has 16 bytes*/
#define PAGE_SIZE 16 /* Page size in bytes */
/* Actions types */
#define START  0
#define END    1
#define ACCESS 2

/*Thread roles*/
#define MANAGER 0      /*The manager is responsible for the simulation*/
#define PRINTER 1      /*The printer is responsible for printing the simulation from time to time*/
#define TIMER   2      /*The timer is the one responsible for writing the simulation time*/

/*Binary files creation*/
#define PHYSICAL 0
#define VIRTUAL  1

/*Structures*/
typedef struct process {
	uint8_t pid;          /*64 bit PID*/
	char name[64];        /*Process name*/
	unsigned int size;    /*Process' total space address*/
  unsigned int arrival; /*Time the process arrives*/
  unsigned int finish;  /*Time the process finishes*/
	unsigned int duration;/*final - arrival*/
	float lifetime;      /*count from 0 to duration. process lifetime*/
  /*position and time arrays together make up the pair [pn, tn] as [position[i], time[i]]*/
  unsigned int length;  /*Size of both position and time arrays*/
  unsigned int *position; /*pns*/
  unsigned int *time;   /*tns*/
	unsigned int index;  /*index of the next [pn, tn] pair to be accessed*/
	/*Control variables*/
	bool done;            /*Processes finished?*/
	bool allocated;       /*Process not finished but it is allocated?*/
} Process;

typedef struct thread {
	int role;          /*Manager = 0, Printer = 1, Chronometer = 2*/
	int spc;           /*Selected algorithm for free space management*/
	int sbs;           /*Selected algorithm for page substitution*/
	float intrvl;      /*Selected printing time interval*/
} Thread;

typedef struct free_list {
  Process *process;             /*If process = NULL, it is an empty space*/
  unsigned int base;            /*Base register. The first space of a contiguous memory this process is occupying*/
  unsigned int limit;           /*Limit register. This is the size of the congiguous space this process is occupying*/
	/*NOTE: base + limit - 1 is the last space of the contiguous memory being occupied. Thus, for base = 3 and limit = 4
	this process occupies spaces 3, 4, 5 and 6 (limit = 4 numbers)*/
  struct free_list *previous;   /*Previous element in the list. If previous = NULL, this is the first element (head)*/
  struct free_list *next;       /*Next element in the list. If element = NULL, this is the last element (tail)*/
} Free_List;

typedef struct event {
	uint8_t pid;
	int event_type;
	int time;
	int position;
	struct event *next;
} Event;

/*Globals*/
unsigned int total;        /*Total physical memory*/
unsigned int virtual;      /*Total virtual memory*/
Process *process;          /*Array with all the processes*/
unsigned int plength;      /*Total number of processes (it is the size of *process array)*/
float elapsed_time;        /*Simulation time*/
int simulating;            /*Simulation finishes when this is set to 0 by the manager thread*/
Free_List *head[4];        /*First cell of the free list*/
Free_List *nf_next;        /*Used by next fit algorithm to save last position*/

/*Mutex*/
sem_t safe_access_memory;  /*Mutex device to safely access memory files*/
sem_t safe_access_list;    /*Mutex device to safely access free lists*/

/* Each subelement of memory strct has 64 bits, so it is 8 bytes
Each element of memory is a page */
typedef int64_t *(memory[PAGE_SIZE/8]);

/*Prototypes*/
/*Simulator setup & initialization prototypes*/
void simulate(int, int, float);
void assign_thread_roles(Thread *, int, int, float);
void do_simulation(pthread_t *, Thread *);
void *run(void *);
int initialize_mutex();
/*Memory files manipulation prototypes*/
void create_memory(int type);
void write_to_memory(int, unsigned int *, unsigned int, uint8_t);
/*Free List manipulation prototypes*/
void initialize_free_list(unsigned int);
int memory_allocation(Free_List *, Process *);
int fit(Process *, int);
int unfit(Process *);
Free_List *fetch_ff(unsigned int);
Free_List *fetch_nf(unsigned int);
Free_List *fetch_qf(unsigned int);

/*TODO: working on these functions (not exactly with this signature)*/
void access_memory(uint8_t PID, int pos);
void print_memory();
int64_t create_process(char* name, int mem_size);
void add_event(int event_type, int time, int position, int PID);
Event * get_next_event();
void start_process(uint8_t PID);
void kill_process(uint8_t PID);


/* TODO: Linked list for the memory information, free and ocuppied. */
/* TODO: . */


/* Information about process and events */
/* needs #include <stdint.h> on .c */
