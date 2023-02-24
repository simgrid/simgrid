/* Simple test code with no bug  */

#include "src/sthread/sthread.h"
#include <stdio.h>

sthread_mutex_t mutex;

static void* thread_fun(void* val)
{
  sthread_mutex_lock(&mutex);
  sthread_mutex_unlock(&mutex);

  fprintf(stderr, "The thread %d is terminating.\n", *(int*)val);
  return NULL;
}

int main(int argc, char* argv[])
{
  sthread_mutex_init(&mutex, NULL);

  int id[2] = {0, 1};
  sthread_t thread1;
  sthread_t thread2;
  sthread_create(&thread1, NULL, thread_fun, &id[0]);
  sthread_create(&thread2, NULL, thread_fun, &id[1]);
  fprintf(stderr, "All threads are started.\n");
  sthread_join(thread1, NULL);
  sthread_join(thread2, NULL);

  sthread_mutex_destroy(&mutex);

  fprintf(stderr, "User's main is terminating.\n");
  return 0;
}
