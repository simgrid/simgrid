/* Simple test code with no bug  */

#include <pthread.h>
#include <stdio.h>

pthread_mutex_t mutex;

static void* thread1_fun(void* ignore)
{
  pthread_mutex_lock(&mutex);
  pthread_mutex_unlock(&mutex);

  fprintf(stderr, "The first thread is terminating.\n");
  return NULL;
}
static void* thread2_fun(void* ignore)
{
  pthread_mutex_lock(&mutex);
  pthread_mutex_unlock(&mutex);

  fprintf(stderr, "The second thread is terminating.\n");
  return NULL;
}

int main(int argc, char* argv[])
{
  pthread_mutex_init(&mutex, NULL);

  pthread_t thread1, thread2;
  pthread_create(&thread1, NULL, thread1_fun, NULL);
  pthread_create(&thread2, NULL, thread2_fun, NULL);
  fprintf(stderr, "All threads are started.\n");
  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);

  fprintf(stderr, "User's main is terminating.\n");
  return 0;
}
