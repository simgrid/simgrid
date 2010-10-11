#include "../mmalloc.h"
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define BUFFSIZE 204800
#define TESTSIZE 100

int main()
{
  void *A, *B;
  int fd1, fd2;
  void *heapA, *heapB;
  void *pointers[TESTSIZE];
/*
  unlink("heap1");
  fd1=open("heap1",O_CREAT|O_RDWR,S_IRWXU|S_IRWXG|S_IRWXO);
  assert(fd1>0);
    close(fd1);
    fd1=open("heap1",O_RDWR);
    assert(fd1>0);
  */

  heapA = mmalloc_attach(-1, sbrk(0) + BUFFSIZE);
  if (heapA == NULL) {
    perror("attach 1 failed");
    fprintf(stderr, "bye\n");
    exit(1);
  }

  fprintf(stderr, "HeapA=%p\n", heapA);

  int i, size;
  for (i = 0; i < TESTSIZE; i++) {
    size = rand() % 1000;
    pointers[i] = mmalloc(heapA, size);
    fprintf(stderr, "%d bytes allocated at %p\n", size, pointers[i]);
  }
  char c;
  scanf("%c", &c);

  for (i = 0; i < TESTSIZE; i++) {
    mfree(heapA, pointers[i]);
  }

  fprintf(stderr, "Ok bye bye\n");
  return 0;
}
