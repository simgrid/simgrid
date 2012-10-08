#include "xbt/mmalloc.h"
#include "xbt.h"
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(test,"this test");

#define BUFFSIZE 204800
#define TESTSIZE 100

int main(int argc, char**argv)
{
  void *heapA;
  void *pointers[TESTSIZE];
  xbt_init(&argc,argv);

  XBT_INFO("Allocating a new heap");
  heapA = xbt_mheap_new(-1, ((char*)sbrk(0)) + BUFFSIZE);
  if (heapA == NULL) {
    perror("attach 1 failed");
    fprintf(stderr, "bye\n");
    exit(1);
  }

  XBT_INFO("HeapA allocated");

  int i, size;
  for (i = 0; i < TESTSIZE; i++) {
    size = ((i % 10)+1)* 100;
    pointers[i] = mmalloc(heapA, size);
    XBT_INFO("%d bytes allocated with offset %lu", size, ((char*)pointers[i])-((char*)heapA));
  }

  for (i = 0; i < TESTSIZE; i++) {
    mfree(heapA, pointers[i]);
  }

  XBT_INFO("Done; bye bye");
  return 0;
}
