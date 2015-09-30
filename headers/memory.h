/* needs #include <stdint.h> on .c */
/* needs #include <stdio.h> on .c */

/* Each PAGE has 16 bytes*/
#define PAGE_SIZE 16 /* Page size in bytes */

/* Each subelement of memory strct has 64 bits, so it is 8 bytes */
/* Each element of memory is a page */
typedef int64_t *(memory[PAGE_SIZE/8]);

FILE * create_vir_mem_file(int size);
FILE * create_phy_mem_file(int size);
boolean update_mem_file(FILE * file, memory mem, int pos, int64_t PID);

/* TODO: Linked list for the memory information, free and ocuppied. */
/* TODO: . */