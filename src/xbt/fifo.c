/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "fifo_private.h"

/*XBT_LOG_NEW_DEFAULT_SUBCATEGORY(fifo,xbt,"FIFO"); UNUSED SO FAR */

/** \defgroup XBT_fifo A generic workqueue
  *  \brief This section describes the API to generic workqueue. These functions
  *   provide the same kind of functionnality as dynamic arrays but in time O(1). 
  *   However these functions use malloc/free a way too much often.
  */

/** \name Functions 
 *  \ingroup XBT_fifo
 */
/* @{ */

/** Constructor
 * \return a new fifo
 */
xbt_fifo_t xbt_fifo_new(void)
{
  xbt_fifo_t fifo;
  fifo = xbt_new0(struct xbt_fifo,1);
  return fifo;
}

/** Destructor
 * \param l poor victim
 *
 * Free the fifo structure. None of the objects that was in the fifo is however modified.
 */
void xbt_fifo_free(xbt_fifo_t l)
{
  xbt_fifo_item_t b, tmp;

  for (b = xbt_fifo_getFirstitem(l); b;
       tmp = b, b = b->next, xbt_fifo_freeitem(tmp));
  free(l);
  return;
}

/* Push
 * \param l list
 * \param t element
 * \return the bucket that was just added
 *
 * Add an object at the tail of the list
 */
xbt_fifo_item_t xbt_fifo_push(xbt_fifo_t l, void *t)
{
  xbt_fifo_item_t new;

  new = xbt_fifo_newitem();
  new->content = t;

  xbt_fifo_push_item(l,new);
  return new;
}

/** Pop
 * \param l list
 * \returns the object stored at the tail of the list.
 *
 * Removes and returns the object stored at the tail of the list. 
 * Returns NULL if the list is empty.
 */
void *xbt_fifo_pop(xbt_fifo_t l)
{
  xbt_fifo_item_t item;
  void *content;

  if(l==NULL) return NULL;
  if(!(item = xbt_fifo_pop_item(l))) return NULL;

  content = item->content;
  xbt_fifo_freeitem(item);
  return content;
}

/**
 * \param l list
 * \param t element
 * \return the bucket that was just added
 *
 * Add an object at the head of the list
 */
xbt_fifo_item_t xbt_fifo_unshift(xbt_fifo_t l, void *t)
{
  xbt_fifo_item_t new;

  new = xbt_fifo_newitem();
  new->content = t;
  xbt_fifo_unshift_item(l,new);
  return new;
}

/** Shift
 * \param l list
 * \returns the object stored at the head of the list.
 *
 * Removes and returns the object stored at the head of the list. 
 * Returns NULL if the list is empty.
 */
void *xbt_fifo_shift(xbt_fifo_t l)
{
  xbt_fifo_item_t item;
  void *content;

  if(l==NULL) return NULL;
  if(!(item = xbt_fifo_shift_item(l))) return NULL;
  
  content = item->content;
  xbt_fifo_freeitem(item);
  return content;
}

/** Push a bucket
 * \param l list
 * \param new bucket
 *
 * Hook up this bucket at the tail of the list
 */
void xbt_fifo_push_item(xbt_fifo_t l, xbt_fifo_item_t new)
{
  (l->count)++;
  if (l->head == NULL) {
    l->head = new;
    l->tail = new;
    return;
  }
  new->prev = l->tail;
  new->prev->next = new;
  l->tail = new;
}

/** Pop bucket
 * \param l
 * \returns the bucket that was at the tail of the list.
 *
 * Returns NULL if the list was empty.
 */
xbt_fifo_item_t xbt_fifo_pop_item(xbt_fifo_t l)
{
  xbt_fifo_item_t item;

  if (l->tail == NULL)
    return NULL;

  item = l->tail;

  l->tail = item->prev;
  if (l->tail == NULL)
    l->head = NULL;
  else
    l->tail->next = NULL;

  (l->count)--;
  return item;
}

/** Push a bucket
 * \param l list
 * \param new bucket
 *
 * Hook up this bucket at the head of the list
 */
void xbt_fifo_unshift_item(xbt_fifo_t l, xbt_fifo_item_t new)
{
  (l->count)++;
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

/** Shift bucket
 * \param l
 * \returns the bucket that was at the head of the list.
 *
 * Returns NULL if the list was empty.
 */
xbt_fifo_item_t xbt_fifo_shift_item(xbt_fifo_t l)
{
  xbt_fifo_item_t item;

  if (l->head == NULL)
    return NULL;

  item = l->head;

  l->head = item->next;
  if (l->head == NULL)
    l->tail = NULL;
  else
    l->head->prev = NULL;

  (l->count)--;
  return item;
}

/**
 * \param l 
 * \param t an objet
 *
 * removes the first occurence of \a t from \a l. 
 * \warning it will not remove duplicates
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
    xbt_fifo_freeitem(current);
    /* WILL NOT REMOVE DUPLICATES */
    break;
  }
  return;
}

/**
 * \param l a list
 * \param current a bucket
 *
 * removes a the bucket \a current from the list \a l
 */
void xbt_fifo_remove_item(xbt_fifo_t l, xbt_fifo_item_t current)
{
  if (l->head == l->tail) {	/* special case */
      l->head = NULL;
      l->tail = NULL;
      (l->count)--;
      return;
    }

    if (current == l->head) {	/* It's the head */
      l->head = current->next;
      l->head->prev = NULL;
    } else if (current == l->tail) {	/* It's the tail */
      l->tail = current->prev;
      l->tail->next = NULL;
    } else {			/* It's in the middle */
      current->prev->next = current->next;
      current->next->prev = current->prev;
    }
    (l->count)--;
}

/**
 * \param f a list
 * \param content an object
 * \return 1 if \a content is in \a f.
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

/**
 * \param f a list
 * \return a table with the objects stored in \a f.
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

/**
 * \param f a list
 * \return a copy of \a f.
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

/** Constructor
 * \return a new bucket
 */
xbt_fifo_item_t xbt_fifo_newitem(void)
{
  return xbt_new0(struct xbt_fifo_item,1);
}

/**
 * \param i a bucket
 * \param v an object
 *
 * stores \a v in \a i.
 */
void xbt_fifo_set_item_content(xbt_fifo_item_t i , void *v)
{
  xbt_fifo_setItemcontent(i,v);
}

/**
 * \param i a bucket
 * \return the object stored \a i.
 */
void *xbt_fifo_get_item_content(xbt_fifo_item_t i)
{
  return xbt_fifo_getItemcontent(i);
}

/** Destructor
 * \param b poor victim
 *
 * Free the bucket but does not modifies the object (if any) that was stored in it.
 */
void xbt_fifo_freeitem(xbt_fifo_item_t b)
{
  free(b);
  return;
}

/**
 * \param f a list
 * \return the number of buckets in \a f.
 */
int xbt_fifo_size(xbt_fifo_t f)
{
  return f->count;
}

/**
 * \param l a list
 * \return the head of \a l.
 */
xbt_fifo_item_t xbt_fifo_getFirstItem(xbt_fifo_t l)
{
  return l->head;
}

/**
 * \param i a bucket
 * \return the bucket that comes next
 */
xbt_fifo_item_t xbt_fifo_getNextItem(xbt_fifo_item_t i)
{
  if(i) return i->next;
  return NULL;
}

/**
 * \param i a bucket
 * \return the bucket that is just before \a i.
 */
xbt_fifo_item_t xbt_fifo_getPrevItem(xbt_fifo_item_t i)
{
  if(i) return i->prev;
  return NULL;
}
/* @} */


