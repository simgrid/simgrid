#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void* thread(void* arg)
{
  char* ret = strdup("Hello, world");

  if (ret == NULL) {
    perror("strdup() error");
    exit(2);
  }

  pthread_exit(ret);
}

int main(int argc, char** argv)
{
  pthread_t id;
  void* ret;

  if (pthread_create(&id, NULL, thread, NULL) != 0) {
    perror("pthread_create() error");
    exit(1);
  }

  if (pthread_join(id, &ret) != 0) {
    perror("pthread_join() error");
    exit(3);
  }

  printf("thread exited with '%s'\n", (char*)ret);

  pthread_exit(NULL);
}