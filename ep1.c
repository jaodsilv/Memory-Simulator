#include <stdio.h>
#include <stdlib.h>
#include "ep1.h"

int run(char **argv)
{
  unsigned int scheduler;

  switch(scheduler = atoi(argv[0])) {
    case FCFS:
      /*do FCFS*/
      printf("1. FCFS\n");
      break;
    case SJF:
      /*do SJF*/
      printf("2. SJF\n");
      break;
    case SRTN:
      /*do SRTN*/
      printf("3. SRTN\n");
      break;
    case RR:
      /*do RR*/
      printf("4. RR\n");
      break;
    case PS:
      /*do PS*/
      printf("5. PS\n");
      break;
    case RRTS:
      /*do RRTS*/
      printf("6. RRTS\n");
      break;
  }

  printf("Run argument 2 = %s\n", argv[1]);
  printf("Run argument 3 = %s\n", argv[2]);


  return 0;
}
