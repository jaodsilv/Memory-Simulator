/* Information about process and events */
/* needs #include <stdint.h> on .c */

/* Actions types */
#define START 0
#define END 1
#define ACCESS 2

typedef struct event {
	int event_type;
	int time;
	int position;
	int64_t PID;
	struct event *next;
} Event;

typedef struct process {
	int64_t PID; /* 64 bit PID */
	unsigned int size;
	char *nome;
} Process;

int64_t create_process(char* name, int mem_size);
void add_event(int event_type, int time, int position, int PID);
Event * get_next_event();

void start_process(int64_t PID);
void kill_process(int64_t PID);
