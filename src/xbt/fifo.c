/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "fifo_private.h"

/*
 * xbt_fifo_new()
 */
xbt_fifo_t xbt_fifo_new(void)
{
  xbt_fifo_t fifo;
  fifo = xbt_new0(struct xbt_fifo,1);
  return fifo;
}

/*
 * xbt_fifo_free()
 */
void xbt_fifo_free(xbt_fifo_t l)
{
  xbt_fifo_item_t b, tmp;

  for (b = xbt_fifo_getFirstitem(l); b;
       tmp = b, b = b->next, xbt_fifo_freeitem(tmp));
  free(l);
  return;
}

/*
 * xbt_fifo_push()
 * at the tail
 */
void xbt_fifo_push(xbt_fifo_t l, void *t)
{
  xbt_fifo_item_t new;

  (l->count)++;
  new = xbt_fifo_newitem();
  new->content = t;
  if (l->head == NULL) {
    l->head = new;
    l->tail = new;
    return;
  }
  new->prev = l->tail;
  new->prev->next = new;
  l->tail = new;
  return;
}

/*
 * xbt_fifo_pop()
 * from the tail
 */
void *xbt_fifo_pop(xbt_fifo_t l)
{
  xbt_fifo_item_t item;
  void *content;

  if (l->tail == NULL)
    return NULL;

  item = l->tail;
  content = item->content;

  l->tail = item->prev;
  if (l->tail == NULL)
    l->head = NULL;
  else
    l->tail->next = NULL;

  xbt_fifo_freeitem(item);
  (l->count)--;
  return content;
}

/*
 * xbt_fifo_unshift()
 * at the head
 */
void xbt_fifo_unshift(xbt_fifo_t l, void *t)
{
  xbt_fifo_item_t new;

  (l->count)++;
  new = xbt_fifo_newitem();
  new->content = t;
  if (l->head == NULL) {
    l->head = new;
    l->tail = new;
    return;
  }
  new->next = l->head;
  new->next->prev = new;
  l->head = new;
  return;
}

/*
 * xbt_fifo_shift()
 * from the head
 */
void *xbt_fifo_shift(xbt_fifo_t l)
{
  xbt_fifo_item_t item;
  void *content;

  if (l->head == NULL)
    return NULL;

  item = l->head;
  content = item->content;

  l->head = item->next;
  if (l->head == NULL)
    l->tail = NULL;
  else
    l->head->prev = NULL;

  xbt_fifo_freeitem(item);
  (l->count)--;
  return content;
}

/*
 * xbt_fifo_remove()
 *   removes an xbt_fifo_item_t using its content from the xbt_fifo 
 */
void xbt_fifo_remove(xbt_fifo_t l, void *t)
{
  xbt_fifo_item_t current, current_next;


  for (current = l->head; current; current = current_next) {
    current_next = current->next;
    if (current->content != t)
      continue;
    /* remove the item */
    xbt_fifo_remove_item(l, current);
    /* WILL NOT REMOVE DUPLICATES */
    break;
  }
  return;
}

/*
 * xbt_fifo_remove_item()
 *   removes a given xbt_fifo_item_t from the xbt_fifo
 */
void xbt_fifo_remove_item(xbt_fifo_t l, xbt_fifo_item_t current)
{
  if (l->head == l->tail) {	/* special case */
      l->head = NULL;
      l->tail = NULL;
      xbt_fifo_freeitem(current);
      (l->count)--;
      return;
    }

    if (current == l->head) {	/* It's the head */
      l->head = current->next;
      l->head->prev = NULL;
      xbt_fifo_freeitem(current);
    } else if (current == l->tail) {	/* It's the tail */
      l->tail = current->prev;
      l->tail->next = NULL;
      xbt_fifo_freeitem(current);
    } else {			/* It's in the middle */
      current->prev->next = current->next;
      current->next->prev = current->prev;
      xbt_fifo_freeitem(current);
    }
    (l->count)--;
}

/*
 * xbt_fifo_is_in()
 */
int xbt_fifo_is_in(xbt_fifo_t f, void *content)
{
  xbt_fifo_item_t item = xbt_fifo_getFirstitem(f);
  while (item) {
    if (item->content == content)
      return 1;
    item = item->next;
  }
  return 0;
}

/*
 * xbt_fifo_to_array()
 */
void **xbt_fifo_to_array(xbt_fifo_t f)
{
  void **array;
  xbt_fifo_item_t b;
  int i;

  if (f->count == 0)
    return NULL;
  else
    array = xbt_new0(void *, f->count);

  for (i = 0, b = xbt_fifo_getFirstitem(f); b; i++, b = b->next) {
    array[i] = b->content;
  }
  return array;
}

/*
 * xbt_fifo_Copy()
 */
xbt_fifo_t xbt_fifo_copy(xbt_fifo_t f)
{
  xbt_fifo_t copy = NULL;
  xbt_fifo_item_t b;

  copy = xbt_fifo_new();

  for (b = xbt_fifo_getFirstitem(f); b; b = b->next) {
    xbt_fifo_push(copy, b->content);
  }
  return copy;
}

/*
 * xbt_fifo_newitem()
 */
xbt_fifo_item_t xbt_fifo_newitem(void)
{
  return xbt_new0(struct xbt_fifo_item,1);
}

void xbt_fifo_set_item_content(xbt_fifo_item_t i , void *v)
{
  xbt_fifo_setItemcontent(i,v);
}

void *xbt_fifo_get_item_content(xbt_fifo_item_t i)
{
  return xbt_fifo_getItemcontent(i);
}

/*
 * xbt_fifo_freeitem()
 */
void xbt_fifo_freeitem(xbt_fifo_item_t b)
{
  free(b);
  return;
}

int xbt_fifo_size(xbt_fifo_t f)
{
  return f->count;
}


