/* 	$Id$	 */

/* a generic and efficient heap                                             */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "heap_private.h"

#include <stdio.h>


/** @addtogroup XBT_heap
 *  \brief This section describes the API to generic heap with O(log(n)) access.
 */

/**
 * @brief Creates a new heap.
 * \param init_size initial size of the heap
 * \param free_func function to call on each element when you want to free
 *             the whole heap (or NULL if nothing to do).
 *
 * Creates a new heap.
 */
xbt_heap_t xbt_heap_new(int init_size, void_f_pvoid_t const free_func)
{
  xbt_heap_t H = xbt_new0(struct xbt_heap, 1);
  H->size = init_size;
  H->count = 0;
  H->items = (xbt_heapItem_t) xbt_new0(struct xbt_heapItem, init_size);
  H->free = free_func;
  return H;
}

/**
 * @brief Set the update callback function.
 * @param H the heap we're working on
 * \param update_callback function to call on each element to update its index when needed.
 */
void xbt_heap_set_update_callback(xbt_heap_t H,
                                  void (*update_callback) (void *, int))
{
  H->update_callback = update_callback;
}


/**
 * @brief kilkil a heap and its content
 * @param H poor victim
 */
void xbt_heap_free(xbt_heap_t H)
{
  int i;
  if (H->free)
    for (i = 0; i < H->count; i++)
      (*(H->free)) (H->items[i].content);
  free(H->items);
  free(H);
  return;
}

/**
 * @brief returns the number of elements in the heap
 * @param H the heap we're working on
 * @return the number of elements in the heap
 */
int xbt_heap_size(xbt_heap_t H)
{
  return (H->count);
}

/**
 * @brief Add an element into the heap.
 * \param H the heap we're working on
 * \param content the object you want to add to the heap
 * \param key the key associated to this object
 *
 * The element with the smallest key is automatically moved at the top of the heap.
 */
void xbt_heap_push(xbt_heap_t H, void *content, double key)
{
  int count = ++(H->count);

  int size = H->size;
  xbt_heapItem_t item;

  if (count > size) {
    H->size = 2 * size + 1;
    H->items =
      (void *) realloc(H->items, (H->size) * sizeof(struct xbt_heapItem));
  }

  item = &(H->items[count - 1]);
  item->key = key;
  item->content = content;
  xbt_heap_increaseKey(H, count - 1);
  return;
}

/**
 * @brief Extracts from the heap and returns the element with the smallest key.
 * \param H the heap we're working on
 * \return the element with the smallest key
 *
 * Extracts from the heap and returns the element with the smallest
 * key. The element with the next smallest key is automatically moved
 * at the top of the heap.
 */
void *xbt_heap_pop(xbt_heap_t H)
{
  void *max;

  if (H->count == 0)
    return NULL;

  max = CONTENT(H, 0);

  H->items[0] = H->items[(H->count) - 1];
  (H->count)--;
  xbt_heap_maxHeapify(H);
  if (H->count < H->size / 4 && H->size > 16) {
    H->size = H->size / 2 + 1;
    H->items =
      (void *) realloc(H->items, (H->size) * sizeof(struct xbt_heapItem));
  }

  H->update_callback ? H->update_callback(max, -1) : NULL;
  return max;
}

/**
 * @brief Extracts from the heap and returns the element at position i.
 * \param H the heap we're working on
 * \param i	element position
 * \return the element at position i if ok, NULL otherwise
 *
 * Extracts from the heap and returns the element at position i. The head is automatically reorded.
 */
void *xbt_heap_remove(xbt_heap_t H, int i)
{
  if ((i < 0) || (i > H->count - 1))
    return NULL;
  /* put element i at head */
  if (i > 0) {
    KEY(H, i) = MIN_KEY_VALUE;
    xbt_heap_increaseKey(H, i);
  }

  return xbt_heap_pop(H);
}

/**
 * @brief returns the smallest key in the heap (heap unchanged)
 * \param H the heap we're working on
 *
 * \return the smallest key in the heap without modifying the heap.
 */
double xbt_heap_maxkey(xbt_heap_t H)
{
  xbt_assert0(H->count != 0, "Empty heap");
  return KEY(H, 0);
}

/**
 * @brief returns the value associated to the smallest key in the heap (heap unchanged)
 * \param H the heap we're working on
 *
 * \return the value associated to the smallest key in the heap
 * without modifying the heap.
 */
void *xbt_heap_maxcontent(xbt_heap_t H)
{
  xbt_assert0(H->count != 0, "Empty heap");
  return CONTENT(H, 0);
}

/* <<<< private >>>>
 * \param H the heap we're working on
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
      H->update_callback ? H->update_callback(CONTENT(H, i), i) : NULL;
      i = greatest;
    } else {
      H->update_callback ? H->update_callback(CONTENT(H, i), i) : NULL;
      return;
    }
  }
}

/* <<<< private >>>>
 * \param H the heap we're working on
 * \param i an item position in the heap
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
    H->update_callback ? H->update_callback(CONTENT(H, i), i) : NULL;
    i = PARENT(i);
  }
  H->update_callback ? H->update_callback(CONTENT(H, i), i) : NULL;
  return;
}

