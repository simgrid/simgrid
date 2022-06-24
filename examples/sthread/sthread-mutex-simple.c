/* Simple test code with no bug  */

#include "src/sthread/sthread.h"
#include <stdio.h>

sthread_mutex_t mutex;

static void* thread1_fun(void* ignore)
{
  sthread_mutex_lock(&mutex);
  sthread_mutex_unlock(&mutex);

  return NULL;
}
static void* thread2_fun(void* ignore)
{
  sthread_mutex_lock(&mutex);
  sthread_mutex_unlock(&mutex);

  return NULL;
}

int main(int argc, char* argv[])
{
  sthread_inside_simgrid = 1;
  sthread_mutex_init(&mutex, NULL);

  sthread_t thread1, thread2;
  sthread_create(&thread1, NULL, thread1_fun, NULL);
  sthread_create(&thread2, NULL, thread2_fun, NULL);
  // pthread_join(thread1, NULL);
  // pthread_join(thread2, NULL);
  fprintf(stderr, "done\n");

  return 0;
}
