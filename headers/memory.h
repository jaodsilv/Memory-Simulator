/* needs #include <stdint.h> on .c */
/* needs #include <stdio.h> on .c */

/* Each PAGE has 16 bytes*/
#define PAGE_SIZE 16 /* Page size in bytes */
/* Actions types */
#define START  0
#define END    1
#define ACCESS 2

/*Structures*/
typedef struct process {
	int64_t pid;          /*64 bit PID*/
	char name[64];        /*Process name*/
	unsigned int size;    /*Process' total space address*/
  unsigned int arrival; /*Time the process arrives*/
  unsigned int finish;  /*Time the process finishes*/
  /*position and time arrays together make up the pair [pn, tn] as [position[i], time[i]]*/
  unsigned int length;  /*Size of both position and time arrays*/
  unsigned int *position;
  unsigned int *time;
} Process;

typedef struct event {
	int64_t pid;
	int event_type;
	int time;
	int position;
	struct event *next;
} Event;

/*Globals*/
unsigned int total;    /*Total physical memory*/
unsigned int virtual;  /*Total virtual memory*/
Process *process;      /*Array with all the processes*/
unsigned int plength;     /*Total number of processes (it is the size of *process array)*/

/* Each subelement of memory strct has 64 bits, so it is 8 bytes
Each element of memory is a page */
typedef int64_t *(memory[PAGE_SIZE/8]);

/*Prototypes*/
bool create_mems(int phySize, int virSize);
bool write_phy_mem(int page, int num_pages, int64_t PID);
bool write_vir_mem(int page, int num_pages, int64_t PID);
void access_memory(int64_t PID, int pos);
void print_memory();
int64_t create_process(char* name, int mem_size);
void add_event(int event_type, int time, int position, int PID);
Event * get_next_event();
void start_process(int64_t PID);
void kill_process(int64_t PID);


/* TODO: Linked list for the memory information, free and ocuppied. */
/* TODO: . */


/* Information about process and events */
/* needs #include <stdint.h> on .c */
