void *fcfs(void *);
void initialize_cores_fcfs(Core *, unsigned int);
void fetch_process_fcfs(Process *, unsigned int);
void use_core_fcfs(Process *, Core *, unsigned int);
unsigned int finished_processes_fcfs(Process *, unsigned int);
unsigned int check_cores_available_fcfs(Core *, unsigned int);
void do_fcfs(pthread_t *, Process *, unsigned int *);
Process *select_fcfs(Process *, unsigned int);
void do_task_fcfs(Process *process);
