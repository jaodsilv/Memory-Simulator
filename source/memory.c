#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "../headers/memory.h"

void simulate(int spc, int sbs, float intrvl)
{
  printf("Aqui o que deve ser usado:\n");
  printf("Algoritmo espaco selecionado: %d\n", spc);
  printf("Algoritmo paginacao selecionado: %d\n", sbs);
  printf("Intervalo de tempo selecionado: %f\n", intrvl);
  printf("Limites memórias: física = %u virtual = %u\n", total, virtual);

  printf("Vetor com os %u processos:\n", plength);
  if(1==1) {
    unsigned int i;
    for(i = 0; i < plength; i++) {
      unsigned int j;
      printf("Processo '%s': chegada = %u saida = %u tamanho = %u \n\tLista de Acessos [pn, tn]: ", process[i].name, process[i].arrival, process[i].finish, process[i].length);
      for(j = 0; j < process[i].length; j++) {
        printf("[%u , %u] ", process[i].position[j], process[i].time[j]);
      }
      printf("\n");
    }
  }
}
