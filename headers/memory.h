#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

/*Time Granularity used to measure time*/
#define TIME_GRAIN 0.1

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

/*Never loaded page frame*/
#define FRESH UINT_MAX

/*Structures*/
typedef struct process {
	uint8_t pid;            /*8 bit PID*/
	char name[64];          /*Process name*/
	unsigned int size;      /*Process' total space address*/
  unsigned int arrival;   /*Time the process arrives*/
  unsigned int finish;    /*Time the process finishes*/
	unsigned int duration;  /*final - arrival*/
	float lifetime;         /*count from 0 to duration. process lifetime*/
  /*position and time arrays together make up the pair [pn, tn] as [position[i], time[i]]*/
  unsigned int length;    /*Size of both position and time arrays*/
  unsigned int *position; /*pns*/
  unsigned int *time;     /*tns*/
	unsigned int index;     /*index of the next [pn, tn] pair to be accessed*/
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

typedef struct page_table {
	Process *process;             /*Process*/
	unsigned int page;            /*Index in the process table*/
	unsigned int page_frame;      /*Frame owned by this process*/
	unsigned int tick;
	float time;                   /*Time the process is allocated (= amount of time he is in the virtual memory)*/
	float loaded_time;            /*Register the time this page was loaded into a page frame*/
	bool present;                 /*Present bit*/
	bool referenced;              /*Referenced bit*/
	bool modified;                /*Modified bit. We are considering every access to be a writing action*/
} Page_Table;

/*Globals*/
unsigned int total;        /*Total physical memory*/
unsigned int virtual;      /*Total virtual memory*/
Process *process;          /*Array with all the processes*/
unsigned int plength;      /*Total number of processes (it is the size of *process array)*/
float elapsed_time;        /*Simulation time*/
int simulating;            /*Simulation finishes when this is set to 0 by the manager thread*/
Free_List *head[4];        /*First cell of the free list*/
Free_List *nf_next;        /*Used by next fit algorithm to save last position*/
Page_Table *page_table;    /*Structure to do the mapping from virtual memory to physical memory*/
unsigned int total_pages;  /*Total number of pages*/
unsigned int tick;
unsigned int total_page_substitutions; /* Number of page changes*/
int *virtual_bitmap;
int *total_bitmap;
/*Mutex*/
sem_t safe_access_memory;  /*Mutex device to safely access memory files*/
sem_t safe_access_list;    /*Mutex device to safely access free lists*/

/*Prototypes*/
/*Simulator setup, initialization and general functions prototypes*/
void simulate(int, int, float);
int valid_process_information();
void assign_thread_roles(Thread *, int, int, float);
void do_simulation(pthread_t *, Thread *);
void *run(void *);
int initialize_mutex();
void free_heads();
/*Memory files manipulation prototypes*/
void create_memory(int);
void write_to_memory(int, unsigned int *, unsigned int, uint8_t);
void print_memory(float);
/*Free List manipulation prototypes*/
void initialize_free_list();
int memory_allocation(Free_List *, Process *);
int fit(Process *, int);
int unfit(Process *);
Free_List *ff(unsigned int);
Free_List *nf(unsigned int);
Free_List *qf(unsigned int);
/*Page Table manipulation prototypes*/
void initialize_page_table();
void assign_process_to_page_table(Free_List *);
void register_allocation(Process *);
void update_page_table_times();
void update_allocated_processes();
void update_page_table(unsigned int, unsigned int, unsigned int);
/*Pages and Paging prototypes*/
unsigned int get_amount_of_pages(unsigned int);
unsigned int get_page_number(Free_List *);
void do_paging(int);
void do_page_substitution(unsigned int, int);
void nrup(unsigned int, unsigned int *, unsigned int);
void fifo(unsigned int, unsigned int *, unsigned int);
void scp(unsigned int, unsigned int *, unsigned int);
void lrup(unsigned int, unsigned int *, unsigned int);
