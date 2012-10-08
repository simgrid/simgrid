#include "xbt/mmalloc.h"
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
  void *heapA;
  void *pointers[TESTSIZE];
  srand(0); // we need the test to be reproducible

  heapA = xbt_mheap_new(-1, ((char*)sbrk(0)) + BUFFSIZE);
  if (heapA == NULL) {
    perror("attach 1 failed");
    fprintf(stderr, "bye\n");
    exit(1);
  }

  fprintf(stderr, "HeapA=%p\n", heapA);
   fflush(stderr);
  int i, size;
  for (i = 0; i < TESTSIZE; i++) {
    size = rand() % 1000;
    pointers[i] = mmalloc(heapA, size);
    fprintf(stderr, "%d bytes allocated with offset %li\n", size, ((char*)pointers[i])-((char*)heapA));
  }

  for (i = 0; i < TESTSIZE; i++) {
    mfree(heapA, pointers[i]);
  }

  fprintf(stderr, "Ok bye bye\n");
  return 0;
}
