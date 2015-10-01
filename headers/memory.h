/* needs #include <stdint.h> on .c */
/* needs #include <stdio.h> on .c */

/* Each PAGE has 16 bytes*/
#define PAGE_SIZE 16 /* Page size in bytes */

/* Each subelement of memory strct has 64 bits, so it is 8 bytes */
/* Each element of memory is a page */
typedef int64_t *(memory[PAGE_SIZE/8]);

bool create_mems(int phySize, int virSize);
bool write_phy_mem(int page, int num_pages, int64_t PID);
bool write_vir_mem(int page, int num_pages, int64_t PID);

void access_memory(int64_t PID, int pos);
void print_memory();

/* TODO: Linked list for the memory information, free and ocuppied. */
/* TODO: . */