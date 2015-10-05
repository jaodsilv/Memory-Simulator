/*EP2 prompt prototypes*/
char *get_cmd(char *);
int expand(char *);
int cmd_load(char *, char *, int *);
int cmd_space(char *, char *, int *);
int cmd_subst(char *, char *, int *);
int cmd_exec(char *, char *, float *, int *, int *, int *);
int cmd_exit(char *);
char *get_arg(char *, char *, char *);
void unrecognized(char *);
void free_pointer(void *);
int sucessful_atoi(char *);
int sucessful_atof(char *);
int read_trace_file(char *);
void reset();
