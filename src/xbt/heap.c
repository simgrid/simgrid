/* a generic and efficient heap                                             */

/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "heap_private.h"

/**
 * xbt_heap_new:
 * @init_size: initial size of the heap
 * @free_func: function to call on each element when you want to free the whole heap (or NULL if nothing to do).
 *
 * Creates a new heap.
 */
xbt_heap_t xbt_heap_new(int init_size, void_f_pvoid_t * const free_func)
{
  xbt_heap_t H = xbt_new0(struct xbt_heap, 1);
  H->size = init_size;
  H->count = 0;
  H->items =
      (xbt_heapItem_t) xbt_new0(struct xbt_heapItem, init_size);
  H->free = free_func;
  return H;
}

/**
 * xbt_heap_free:
 * @H: poor victim
 *
 * kilkil a heap and its content
 */
void xbt_heap_free(xbt_heap_t H)
{
  int i;
  if (H->free)
    for (i = 0; i < H->size; i++)
      H->free(H->items[i].content);
  xbt_free(H->items);
  xbt_free(H);
  return;
}

/**
 * xbt_heap_push:
 * @H: the heap we're working on
 * @content: the object you want to add to the heap
 * @key: the key associated to this object
 *
 * Add an element int the heap. The element with the smallest key is
 * automatically moved at the top of the heap.
 */
void xbt_heap_push(xbt_heap_t H, void *content, xbt_heap_float_t key)
{
  int count = ++(H->count);
  int size = H->size;
  xbt_heapItem_t item;
  if (count > size) {
    H->size = 2 * size + 1;
    H->items =
	(void *) realloc(H->items,
			 (H->size) * sizeof(struct xbt_heapItem));
  }
  item = &(H->items[count - 1]);
  item->key = key;
  item->content = content;
  xbt_heap_increaseKey(H, count - 1);
  return;
}

/**
 * xbt_heap_pop:
 * @H: the heap we're working on
 *
 * Extracts from the heap and returns the element with the smallest
 * key. The element with the next smallest key is automatically moved
 * at the top of the heap.
 */
void *xbt_heap_pop(xbt_heap_t H)
{
  void *max = CONTENT(H, 0);
  H->items[0] = H->items[(H->count) - 1];
  (H->count)--;
  xbt_heap_maxHeapify(H);
  if (H->count < H->size / 4 && H->size > 16) {
    H->size = H->size / 2 + 1;
    H->items =
	(void *) realloc(H->items,
			 (H->size) * sizeof(struct xbt_heapItem));
  }
  return max;
}

/**
 * xbt_heap_maxkey:
 * @H: the heap we're working on
 *
 * Returns the smallest key in the heap without modifying the heap.
 */
xbt_heap_float_t xbt_heap_maxkey(xbt_heap_t H)
{
  return KEY(H, 0);
}

/**
 * xbt_heap_maxcontent:
 * @H: the heap we're working on
 *
 * Returns the value associated to the smallest key in the heap
 * without modifying the heap.
 */
void *xbt_heap_maxcontent(xbt_heap_t H)
{
  return CONTENT(H, 0);
}

/**
 * xbt_heap_maxcontent:
 * @H: the heap we're working on
 * 
 * Restores the heap property once an element has been deleted.
 */
static void xbt_heap_maxHeapify(xbt_heap_t H)
{
  int i = 0;
  while (1) {
    int greatest = i;
    int l = LEFT(i);
    int r = RIGHT(i);
    int count = H->count;
    if (l < count && KEY(H, l) < KEY(H, i))
      greatest = l;
    if (r < count && KEY(H, r) < KEY(H, greatest))
      greatest = r;
    if (greatest != i) {
      struct xbt_heapItem tmp = H->items[i];
      H->items[i] = H->items[greatest];
      H->items[greatest] = tmp;
      i = greatest;
    } else
      return;
  }
}

/**
 * xbt_heap_maxcontent:
 * @H: the heap we're working on
 * @i: an item position in the heap
 * 
 * Moves up an item at position i to its correct position. Works only
 * when called from xbt_heap_push. Do not use otherwise.
 */
static void xbt_heap_increaseKey(xbt_heap_t H, int i)
{
  while (i > 0 && KEY(H, PARENT(i)) > KEY(H, i)) {
    struct xbt_heapItem tmp = H->items[i];
    H->items[i] = H->items[PARENT(i)];
    H->items[PARENT(i)] = tmp;
    i = PARENT(i);
  }
  return;
}
