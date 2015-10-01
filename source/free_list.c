
/*It is expected to have 2 free_lists.
One for virtual memory and another for physical memory*/
typedef struct free_list {
  Process *process;  /*If process = NULL, it is an empty space*/
  unsigned int base;
  unsigned int limit;
  struct free_list *previous;
  struct free_list *next;
} Free_List;

/*Initiates the free list, returning the first cell.*/
Free_List *init_free_list(unsigned int size)
{
  Free_List *fl;
  fl = malloc(sizeof(*fl));
  fl.process = fl.previous = fl.next = NULL;
  fl.base = 0;
  fl.limit = size;
  return fl;
}

/*Allocates memory for a new process in the free_list. Don't call this function!
  Call "fit" function to do the job.*/
int mem_alloc(Free_List *fl, Process *process)
{
  Free_List *new;
  unsigned int old_limit;

  /*Allocated cell*/
  if(fl.process == NULL) {
    old_limit = fl.limit;
    fl.process = process;
    fl.limit = process.size;
  }
  else {
    /*Bad cell selected by fetch algorithm.
    This condition will be used for testing*/
    fprintf(stderr, "Error: bad fetch. Was expecting an available memory space.\n");
    return 0;
  }

  /*Remaining free space. There will be no remaining free space
  from the old cell if fl.limit == process.size*/
  if(fl.limit > process.size) {
    new = malloc(sizeof(*new));
    new.previous = fl;
    new.process = new.next = NULL;
    new.base = fl.base + fl.limit;
    new.limit = old_limit - fl.limit;

    fl.next = new;
  }
  else fl.next = NULL;

  return 1;
}

/*Frees memory from the free_list previously allocated by process *process.
*fl is the pointer to the first cell of the free_list
Note: this function DOES NOT disallocates the process
*/
int unfit(Free_List *fl, Process *process)
{
  Free_List *p;

  /*Fetch process*/
  for(p = fl; p.process != process && p != NULL ;p = p->next) continue;
  if(p == NULL) {
    /*Failed free. Process not found*/
    fprintf(stderr, "Error: failed to find allocated process.\n");
    return 0;
  }

  /*Disallocate*/
  if(p->previous != NULL)
    /*Correct pointers if not the first*/
    p->previous->next = p->next;
  if(p->next != NULL) {
    /*Correct pointers and values if not the last*/
    p->next->previous = p->previous;
    p->next->base = p->base;
    p->next->limit += p->limit;
  }
  free(p); p = NULL;

  return 1;
}

/*Attempts to fit a process into the free_list using the correct fetch algorithm*/
int fit(Free_List *fl, Process *process, int fit_number)
{
  int ret;
  switch(fit_number) {
    Free_List *f;
    case FF:
      f = fetch_ff();
      ret = mem_alloc(f, process);
      break;
    case NF:
      f = fetch_nf();
      ret = mem_alloc(f, process);
      break;
    case QF:
      f = fetch_qf();
      ret = mem_alloc(f, process);
      break;
  }
  return ret;
}

/*TODO:*/
/*Returns a free space where the process can fit in, according to FF policy*/
Free_List *fetch_ff(/*Args*/)
{

}
/*TODO:*/
/*Returns a free space where the process can fit in, according to NF policy*/
Free_List *fetch_nf(/*Args*/)
{

}
/*TODO:*/
/*Returns a free space where the process can fit in, according to QF policy*/
Free_List *fetch_qf(/*Args*/)
{

}


/*TODO: Appropriate header to this file. The contents of this header might be within memory.h*/
