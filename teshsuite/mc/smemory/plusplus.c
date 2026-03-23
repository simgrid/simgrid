#include "assert.h"
#include "pthread.h"

int i = 0;

static void funcA()
{
  /* on-stack accesses should not be traced */
  int j = 0;
  j++;

  i += j; /* Racy access to i! */
  i++;
}

static void funcB()
{
  funcA();
}

static void* thread_code(void* ignored)
{
  funcB();
  return NULL;
}

int main()
{
  int k = 0;
  k++;

  pthread_t p1, p2;
  pthread_create(&p1, NULL, thread_code, NULL);
  pthread_create(&p2, NULL, thread_code, NULL);
  pthread_join(p1, NULL);
  pthread_join(p2, NULL);
  assert(i == 2);

  assert(k == 1);
  return 0;
}
