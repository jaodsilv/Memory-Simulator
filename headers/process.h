/* Information about process and events */
/* needs #include <stdint.h> on .c */

/* Actions types */
#define START 0
#define END 1
#define ACCESS 2

typedef struct event {
	int eventType;
	int time;
	int position;
	int64_t PID;
	struct event *next;
} Event;

typedef struct process {
	int64_t PID; /* 64 bit PID */
	unsigned int mem_pos_phy;
	unsigned int mem_pos_vir;
	unsigned int size;
	char *nome;
} Process;

process create_process(char* name, int mem_size);
event create_event(int eventType, int time, int position, int PID);
event add_event(event events, event new_event);
