void *srtn(void *);
void initialize_cores_srtn(Core *, unsigned int);
void fetch_process_srtn(Process *, unsigned int);
void use_core_srtn(Process *, Core *, unsigned int);
unsigned int finished_processes_srtn(Process *, unsigned int);
unsigned int check_cores_available_srtn(Core *, unsigned int);
void do_srtn(pthread_t *, Process *, unsigned int *);
Process *select_srtn(Process *, unsigned int);
int do_task_srtn(Process *process);
unsigned int release_core_srtn(Process *, Core *, unsigned int);