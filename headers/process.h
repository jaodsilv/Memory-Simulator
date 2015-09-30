/* Information about process and events */
/* needs #include <stdint.h> on .c */

/* Actions types */
#define START 0
#define END 1
#define ACCESS 2

typedef struct EVENT *event;
typedef struct PROCESS *process;

struct EVENT {
	int eventType;
	int time;
	int position;
	int64_t PID;
	event next;
};

struct PROCESS {
	int64_t PID; /* 64 bit PID */
	int mem_pos_phy;
	int mem_pos_vir;
	int mem_size;
	char *nome;
};

process create_process(char* name, int mem_size);
event create_event(int eventType, int time, int position, int PID);
event add_event(event events, event new_event);
