#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "xbt.h"
#include "xbt/context.h"
#include "portable.h"           /* To know whether we're using threads or context */
#include "xbt/fifo.h"

xbt_context_t cA = NULL;
xbt_context_t cB = NULL;
xbt_context_t cC = NULL;
xbt_fifo_t fifo = NULL;

void print_args(int argc, char **argv);
void print_args(int argc, char **argv)
{
  int i;

  printf("args=<");
  for (i = 0; i < argc; i++)
    printf("%s ", argv[i]);
  printf(">\n");
}

int fA(int argc, char **argv);
int fA(int argc, char **argv)
{
  printf("Here is fA: ");
  print_args(argc, argv);

/*   printf("\tContext A: Yield\n"); */
/*   xbt_context_yield(); // FIXME: yielding to itself fails, no idea why */

  printf("\tContext A: Push context B\n");
  xbt_fifo_push(fifo, cB);

  printf("\tContext A: Yield\n");
  xbt_context_yield();

  printf("\tContext A: bye\n");

  return 0;
}

int fB(int argc, char **argv);
int fB(int argc, char **argv)
{
  printf("Here is fB: ");
  print_args(argc, argv);

  printf("\tContext B: Yield\n");
  xbt_context_yield();

  printf("\tContext B: Push context A\n");
  xbt_fifo_push(fifo, cA);

  printf("\tContext B: Yield\n");
  xbt_context_yield();

  printf("\tContext B: bye\n");

  return 0;
}

int fC(int argc, char **argv);
int fC(int argc, char **argv)
{
  printf("Here is fC: ");
  print_args(argc, argv);

  printf("\tContext C: Yield\n");
  xbt_context_yield();

  printf("\tContext C: bye\n");

  return 0;
}

#ifdef __BORLANDC__
#pragma argsused
#endif

int main(int argc, char **argv)
{
  xbt_context_t context = NULL;

  printf("XXX Test the simgrid context API\n");
#if CONTEXT_THREADS
  printf("XXX Using threads as a backend.\n");
#else /* use SUSv2 contexts */
  printf("XXX Using SUSv2 contexts as a backend.\n");
  printf
    ("    If it fails, try another context backend.\n    For example, to force the pthread backend, use:\n       ./configure --with-context=pthread\n\n");
#endif

  xbt_init(&argc, argv);

  cA = xbt_context_new("A", fA, NULL, NULL, NULL, NULL, 0, NULL);
  cB = xbt_context_new("B", fB, NULL, NULL, NULL, NULL, 0, NULL);
  cC = xbt_context_new("C", fC, NULL, NULL, NULL, NULL, 0, NULL);

  fifo = xbt_fifo_new();

  printf("Here is context 'main'\n");
  printf("\tPush context 'A' (%p) from context 'main'\n", cA);
  xbt_fifo_push(fifo, cA);
  xbt_context_start(cA);

  printf("\tPush context 'B' (%p) from context 'main'\n", cB);
  xbt_fifo_push(fifo, cB);
  xbt_context_start(cB);

  printf("\tPush context 'C' (%p) from context 'main'\n", cC);
  xbt_fifo_push(fifo, cC);
  xbt_context_start(cC);

  while ((context = xbt_fifo_shift(fifo))) {
    printf("Context main: schedule\n");
    xbt_context_schedule(context);
  }

  printf("\tFreeing Fifo\n");
  xbt_fifo_free(fifo);
  printf("\tExit & cleaning living threads\n");
  xbt_exit();

  cA = cB = cC = NULL;
  printf("Context main: Bye\n");
  return 0;
}
