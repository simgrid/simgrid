#include "xbt/mallocator.h"
#include "xbt/asserts.h"
#include "xbt/sysdep.h"
#include "mallocator_private.h"

xbt_mallocator_t xbt_mallocator_new(int size,
				    pvoid_f_void_t new_f,
				    void_f_pvoid_t free_f,
				    void_f_pvoid_t reset_f) {
  xbt_assert0(size > 0, "size must be positive");
  xbt_assert0(new_f != NULL && free_f != NULL && reset_f != NULL,
	      "invalid parameter");
  xbt_mallocator_t m = xbt_new0(s_xbt_mallocator_t, 1);

  m->objects = xbt_new0(void*, size);
  m->max_size = size;
  m->current_size = 0;
  m->new_f = new_f;
  m->free_f = free_f;
  m->reset_f = reset_f;

  return m;
}

/* Destroy the mallocator and all its data */
void xbt_mallocator_free(xbt_mallocator_t m) {
  xbt_assert0(m != NULL, "Invalid parameter");

  int i;
  for (i = 0; i < m->current_size; i++) {
    m->free_f(m->objects[i]);
  }
  xbt_free(m->objects);
  xbt_free(m);
}

/* Return an object (use this function instead of malloc) */
void *xbt_mallocator_get(xbt_mallocator_t m) {
  xbt_assert0(m != NULL, "Invalid parameter");

  void *object;
  if (m->current_size > 0) {
    /* there is at least an available object */
    return m->objects[--m->current_size];
  }
  else {
    /* otherwise we must allocate a new object */
    object = m->new_f();
    m->reset_f(object);
    return object;
  }
}

/* Release an object (use this function instead of free) */
void xbt_mallocator_release(xbt_mallocator_t m, void *object) {
  xbt_assert0(m != NULL && object != NULL, "Invalid parameter");

  if (m->current_size < m->max_size) {
    /* there is enough place to push the object */
    m->reset_f(object);
    m->objects[m->current_size++] = object;
  }
  else {
    /* otherwise we don't have a choice, we must free the object */
    m->free_f(object);
  }
}
