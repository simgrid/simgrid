/* Simple test code with no bug  */

#include <pthread.h>
#include <stdio.h>

pthread_mutex_t mutex;

static void* thread1_fun(void* ignore)
{
  pthread_mutex_lock(&mutex);
  pthread_mutex_unlock(&mutex);

  return NULL;
}
static void* thread2_fun(void* ignore)
{
  pthread_mutex_lock(&mutex);
  pthread_mutex_unlock(&mutex);

  return NULL;
}

int main(int argc, char* argv[])
{
  fprintf(stderr, "User main is starting\n");

  pthread_mutex_init(&mutex, NULL);

  pthread_t thread1, thread2;
  fprintf(stderr, "prout\n");
  pthread_create(&thread1, NULL, thread1_fun, NULL);
  pthread_create(&thread2, NULL, thread2_fun, NULL);
  // pthread_join(thread1, NULL);
  // pthread_join(thread2, NULL);

  fprintf(stderr, "User main is done\n");
  return 0;
}
