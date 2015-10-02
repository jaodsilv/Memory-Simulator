#define START  0
#define END    1
#define ACCESS 2

/* Information about process and events */
/* needs #include <stdint.h> on .c */

/* Actions types */

typedef struct process {
	int64_t PID; /* 64 bit PID */
	unsigned int size;
	char *nome;
} Process;

typedef struct event {
	int event_type;
	int time;
	int position;
	int64_t PID;
	struct event *next;
} Event;

Process *process;    /*Array with all the processes*/
unsigned int pnum;   /*Total number of processes (it is the size of *process array)*/


int64_t create_process(char* name, int mem_size);
void add_event(int event_type, int time, int position, int PID);
Event * get_next_event();

void start_process(int64_t PID);
void kill_process(int64_t PID);
