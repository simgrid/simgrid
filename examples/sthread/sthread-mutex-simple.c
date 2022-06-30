/* Simple test code with no bug  */

#include "src/sthread/sthread.h"
#include <stdio.h>

sthread_mutex_t mutex;

static void* thread_fun(void* ignore)
{
  sthread_mutex_lock(&mutex);
  sthread_mutex_unlock(&mutex);

  return NULL;
}

int main(int argc, char* argv[])
{
  sthread_mutex_init(&mutex, NULL);

  sthread_t thread1;
  sthread_t thread2;
  sthread_create(&thread1, NULL, thread_fun, NULL);
  sthread_create(&thread2, NULL, thread_fun, NULL);
  sthread_join(thread1, NULL);
  sthread_join(thread2, NULL);

  sthread_mutex_destroy(&mutex);

  fprintf(stderr, "done\n");
  return 0;
}
