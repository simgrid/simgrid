/* gs_tools.c */

#include "gs/gs_private.h"

void *
gs_memdup(const void * const ptr,
          const size_t       length) {
  void * new_ptr = NULL;

  new_ptr = malloc(length);

  if (new_ptr) {
    memcpy(new_ptr, ptr, length);
  }

  return new_ptr;
}

