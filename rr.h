#include "core.h"

void *rr(void *);
void initialize_cores_rr(Core *, unsigned int);
void fetch_process_rr(Process *, unsigned int, Rotation *);
void use_core_rr(Process *, Core *, unsigned int);
unsigned int finished_processes_rr(Process *, unsigned int);
unsigned int check_cores_available_rr(Core *, unsigned int);
void do_rr(pthread_t *, Process *, unsigned int *);
Process *select_rr(Process *, Rotation *);
int do_task_rr(Process *);
float calculate_quantum_rr(Rotation *);
void release_cores_rr(Process *, unsigned int, Core *, unsigned int, float);
