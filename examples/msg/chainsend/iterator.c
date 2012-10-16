#include "iterator.h"

/* http://stackoverflow.com/a/3348142 */
static int rand_int(int n)
{
  int limit = RAND_MAX - RAND_MAX % n;
  int rnd;

  do {
    rnd = rand();
  } while (rnd >= limit);
  
  return rnd % n;
}

void xbt_dynar_shuffle_in_place(xbt_dynar_t indices_list)
{
  int i, j;

  for (i = xbt_dynar_length(indices_list) - 1; i > 0; i--) {
    j = rand_int(i + 1);
    xbt_dynar_swap_elements(indices_list, int, i, j);
  }
}
/**************************************/

/* Allocates and initializes a new xbt_dynar iterator for list, using criteria_fn as iteration criteria
   criteria_fn: given an array size, it must generate a list containing the indices of every item in some order */
xbt_dynar_iterator_t xbt_dynar_iterator_new(xbt_dynar_t list, xbt_dynar_t (*criteria_fn)(int))
{
  xbt_dynar_iterator_t it = xbt_new(xbt_dynar_iterator_s, 1);
  
  it->list = list;
  it->length = xbt_dynar_length(list);
  it->indices_list = criteria_fn(it->length); //xbt_dynar_new(sizeof(int), NULL);
  it->criteria_fn = criteria_fn;
  it->current = 0;
}

void xbt_dynar_iterator_reset(xbt_dynar_iterator_t it)
{
  xbt_dynar_free_container(&(it->indices_list));
  it->indices_list = it->criteria_fn(it->length);
  it->current = 0;
}

void xbt_dynar_iterator_seek(xbt_dynar_iterator_t it, int pos)
{
  it->current = pos;
}

/* Returns the next element iterated by iterator it, NULL if there are no more elements */
void *xbt_dynar_iterator_next(xbt_dynar_iterator_t it)
{
  int *next;
  //XBT_INFO("%d current\n", next);
  if (it->current >= it->length) {
    //XBT_INFO("Nothing to return!\n");
    return NULL;
  } else {
    next = xbt_dynar_get_ptr(it->indices_list, it->current);
    it->current++;
    return xbt_dynar_get_ptr(it->list, *next);
  }
}

void xbt_dynar_iterator_delete(xbt_dynar_iterator_t it)
{
  xbt_dynar_free_container(&(it->indices_list));
  xbt_free_ref(&it);
}

xbt_dynar_t forward_indices_list(int size)
{
  xbt_dynar_t indices_list = xbt_dynar_new(sizeof(int), NULL);
  int i;
  for (i = 0; i < size; i++)
    xbt_dynar_push_as(indices_list, int, i);
  return indices_list;
}

xbt_dynar_t reverse_indices_list(int size)
{
  xbt_dynar_t indices_list = xbt_dynar_new(sizeof(int), NULL);
  int i;
  for (i = size-1; i >= 0; i--)
    xbt_dynar_push_as(indices_list, int, i);
  return indices_list;
}

xbt_dynar_t random_indices_list(int size)
{
  xbt_dynar_t indices_list = forward_indices_list(size);
  xbt_dynar_shuffle_in_place(indices_list);
  return indices_list;
}
