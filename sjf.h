#include "core.h"

void *sjf(void *);
void initialize_cores_sjf(Core *, unsigned int);
void fetch_process_sjf(Process *, unsigned int);
void use_core_sjf(Process *, Core *, unsigned int);
unsigned int finished_processes_sjf(Process *, unsigned int);
unsigned int check_cores_available_sjf(Core *, unsigned int);
void do_sjf(pthread_t *, Process *, unsigned int *);
Process *select_sjf(Process *, unsigned int);
void do_task_sjf(Process *process);
