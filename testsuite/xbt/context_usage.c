#include "xbt/context.h"
#include "xbt/fifo.h"
#include "stdlib.h"
#include "stdio.h"

xbt_context_t cA = NULL;
xbt_context_t cB = NULL;
xbt_context_t cC = NULL;
xbt_fifo_t fifo = NULL;

void print_args(int argc, char** argv);
void print_args(int argc, char** argv)
{
  int i ; 

  printf("<");
  for(i=0; i<argc; i++) 
    printf("%s ",argv[i]);
  printf(">\n");
}

int fA(int argc, char** argv);
int fA(int argc, char** argv)
{
  printf("fA: ");
  print_args(argc,argv);

  printf("\tA: Yield\n");
  xbt_context_yield();
  printf("\tA: Yield\n");
  xbt_context_yield();
  printf("\tA : bye\n");

  return 0;
}

int fB(int argc, char** argv);
int fB(int argc, char** argv)
{
  printf("fB: ");
  print_args(argc,argv);

  printf("\tB: Yield\n");
  xbt_context_yield();
  xbt_fifo_push(fifo,cA);
  printf("\tB->A\n");
  printf("\tB: Yield\n");
  xbt_context_yield();
  printf("\tB : bye\n");

  return 0;
}

int fC(int argc, char** argv);
int fC(int argc, char** argv)
{
  printf("fC: ");
  print_args(argc,argv);


  printf("\tC: Yield\n");
  xbt_context_yield();


  return 0;
}

int main(int argc, char** argv)
{
  xbt_context_t context = NULL;

  xbt_context_init();

  cA = xbt_context_new(fA, 0, NULL);
  cB = xbt_context_new(fB, 0, NULL);
  cC = xbt_context_new(fC, 0, NULL);

  fifo = xbt_fifo_new();

  xbt_context_start(cA);
  printf("\tO->A\n");xbt_fifo_push(fifo,cA);
  xbt_context_start(cB);
  printf("\tO->B\n");xbt_fifo_push(fifo,cB);
  xbt_context_start(cC);xbt_fifo_push(fifo,cC);
  printf("\tO->C\n");xbt_fifo_push(fifo,cC);

  while((context=xbt_fifo_shift(fifo))) {
    printf("\tO:Yield\n");
    xbt_context_schedule(context);
  }

  xbt_fifo_free(fifo);
  xbt_context_exit();
  
  cA=cB=cC=NULL;
  return 0;
}
