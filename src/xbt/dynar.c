/* $Id$ */

/* a generic DYNamic ARray                                                  */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2003 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras_private.h"
#include <sys/types.h>

GRAS_LOG_NEW_DEFAULT_SUBCATEGORY(dynar,gros,"Dynamic arrays");

struct gras_dynar_s {
  size_t          size;
  size_t          used;
  size_t          elmsize;
  void           *data;
  void_f_pvoid_t *free;
};

#define __sanity_check_dynar(dynar)       \
           gras_assert0(dynar,           \
			"dynar is NULL")
#define __sanity_check_idx(idx)                \
           gras_assert1(idx >= 0,             \
			"dynar idx(=%d) < 0", \
			(int) (idx))
#define __check_inbound_idx(dynar, idx)                                                \
           gras_assert2(idx < dynar->used,                                             \
			"dynar is not that long. You asked %d, but it's only %lu long", \
			(int) (idx), (unsigned long) dynar->used)
#define __check_sloppy_inbound_idx(dynar, idx)                                         \
           gras_assert2(idx <= dynar->used,                                            \
			"dynar is not that long. You asked %d, but it's only %lu long", \
			(int) (idx), (unsigned long) dynar->used)
#define __check_populated_dynar(dynar)            \
           gras_assert1(dynar->used,              \
			"dynar %p contains nothing",(void*)dynar)

static _GRAS_INLINE 
void _gras_clear_mem(void * const ptr,
		     const size_t length) {
  memset(ptr, 0, length);
}

static _GRAS_INLINE
gras_error_t
_gras_dynar_expand(gras_dynar_t * const dynar,
                   const int            nb) {
  gras_error_t errcode     = no_error;
  const size_t old_size    = dynar->size;

  if (nb > old_size) {
    char * const old_data    = dynar->data;

    const size_t elmsize     = dynar->elmsize;
    const size_t old_length  = old_size*elmsize;

    const size_t used        = dynar->used;
    const size_t used_length = used*elmsize;

    const size_t new_size    = nb > (2*(old_size+1)) ? nb : (2*(old_size+1));
    const size_t new_length  = new_size*elmsize;
    char * const new_data    = gras_malloc0(elmsize*new_size);

    DEBUG3("expend %p from %lu to %d elements", (void*)dynar, (unsigned long)old_size, nb);

    if (old_data) {
      memcpy(new_data, old_data, used_length);
      _gras_clear_mem(old_data, old_length);
      gras_free(old_data);
    }

    _gras_clear_mem(new_data + used_length, new_length - used_length);

    dynar->size = new_size;
    dynar->data = new_data;
  }

  return errcode;
}

static _GRAS_INLINE
void *
_gras_dynar_elm(const gras_dynar_t * const dynar,
                const size_t               idx) {
  char * const data    = dynar->data;
  const size_t elmsize = dynar->elmsize;

  return data + idx*elmsize;
}

static _GRAS_INLINE
void
_gras_dynar_get_elm(void               * const dst,
                    const gras_dynar_t * const dynar,
                    const size_t               idx) {
  void * const elm     = _gras_dynar_elm(dynar, idx);
  const size_t elmsize = dynar->elmsize;

  memcpy(dst, elm, elmsize);
}

static _GRAS_INLINE
void
_gras_dynar_put_elm(const gras_dynar_t * const dynar,
                    const size_t               idx,
                    const void         * const src) {
  void * const elm     = _gras_dynar_elm(dynar, idx);
  const size_t elmsize = dynar->elmsize;

  memcpy(elm, src, elmsize);
}

/**
 * gras_dynar_new:
 * @whereto: pointer to where the dynar should be created
 * @elm_size: size of each element in the dynar
 * @free_func: function to call each time we want to get rid of an element (or NULL if nothing to do).
 *
 * Creates a new dynar. If a free_func is provided, the elements have to be
 * pointer of pointer. That is to say that dynars can contain either base
 * types (int, char, double, etc) or pointer of pointers (struct **).
 */
void
gras_dynar_new(gras_dynar_t   ** const p_dynar,
               const size_t            elmsize,
               void_f_pvoid_t  * const free_func) {
   
  gras_dynar_t *dynar = gras_new0(gras_dynar_t,1);

  dynar->size    = 0;
  dynar->used    = 0;
  dynar->elmsize = elmsize;
  dynar->data    = NULL;
  dynar->free    = free_func;

  *p_dynar = dynar;
}

/**
 * gras_dynar_free_container:
 * @dynar: poor victim
 *
 * kilkil a dynar BUT NOT its content. Ie, the array is freed, but not what
 * its contain points to.
 */
void
gras_dynar_free_container(gras_dynar_t * const dynar) {
  if (dynar) {

    if (dynar->data) {
      _gras_clear_mem(dynar->data, dynar->size);
      gras_free(dynar->data);
    }

    _gras_clear_mem(dynar, sizeof(gras_dynar_t));

    gras_free(dynar);
  }
}

/**
 * gras_dynar_reset:
 * @dynar: who to squeeze
 *
 * Frees the content and set the size to 0
 */
void
gras_dynar_reset(gras_dynar_t * const dynar) {

  __sanity_check_dynar(dynar);

  DEBUG1("Reset the dynar %p",(void*)dynar);
  if (dynar->free) {
    gras_dynar_map(dynar, dynar->free);
  }

  if (dynar->data) {
    _gras_clear_mem(dynar->data, dynar->size);
    gras_free(dynar->data);
  }

  dynar->size = 0;
  dynar->used = 0;
  dynar->data = NULL;
}

/**
 * gras_dynar_free:
 * @dynar: poor victim
 *
 * kilkil a dynar and its content
 */

void
gras_dynar_free(gras_dynar_t * const dynar) {
  if (dynar) {
    gras_dynar_reset(dynar);
    gras_dynar_free_container(dynar);
  }
}

/**
 * gras_dynar_length:
 * @dynar: the dynar we want to mesure
 *
 * Returns the count of elements in a dynar
 */
unsigned long
gras_dynar_length(const gras_dynar_t * const dynar) {
  return (dynar ? (unsigned long) dynar->used : (unsigned long)0);
}

/**
 * gras_dynar_get:
 * @dynar: information dealer
 * @idx: index of the slot we want to retrive
 * @dst: where to pu the result to.
 *
 * Retrieve the Nth element of a dynar. Warning, the returned value is the actual content of 
 * the dynar. Make a copy before fooling with it.
 */
void
gras_dynar_get(const gras_dynar_t * const dynar,
               const int                  idx,
               void               * const dst) {

  __sanity_check_dynar(dynar);
  __sanity_check_idx(idx);
  __check_inbound_idx(dynar, idx);

  _gras_dynar_get_elm(dst, dynar, idx);
}

/**
 * gras_dynar_set:
 * @dynar:
 * @idx:
 * @src: What will be feeded to the dynar
 *
 * Set the Nth element of a dynar, expanding the dynar if needed, BUT NOT freeing
 * the previous value at this position. If you want to free the previous content,
 * use gras_dynar_remplace().
 */
void
gras_dynar_set(gras_dynar_t * const dynar,
               const int            idx,
               const void   * const src) {

  __sanity_check_dynar(dynar);
  __sanity_check_idx(idx);

  _gras_dynar_expand(dynar, idx+1);

  if (idx >= dynar->used) {
    dynar->used = idx+1;
  }

  _gras_dynar_put_elm(dynar, idx, src);
}

/**
 * gras_dynar_remplace:
 * @dynar:
 * @idx:
 * @object:
 *
 * Set the Nth element of a dynar, expanding the dynar if needed, AND DO
 * free the previous value at this position. If you don't want to free the
 * previous content, use gras_dynar_set().
 */
void
gras_dynar_remplace(gras_dynar_t * const dynar,
                    const int            idx,
                    const void   * const object) {

  __sanity_check_dynar(dynar);
  __sanity_check_idx(idx);

  if (idx < dynar->used && dynar->free) {
    void * const old_object = _gras_dynar_elm(dynar, idx);

    dynar->free(old_object);
  }

  gras_dynar_set(dynar, idx, object);
}

/**
 * gras_dynar_insert_at:
 * @dynar:
 * @idx:
 * @src: What will be feeded to the dynar
 *
 * Set the Nth element of a dynar, expanding the dynar if needed, and
 * moving the previously existing value and all subsequent ones to one
 * position right in the dynar.
 */
void
gras_dynar_insert_at(gras_dynar_t * const dynar,
                     const int            idx,
                     const void   * const src) {

  __sanity_check_dynar(dynar);
  __sanity_check_idx(idx);
  __check_sloppy_inbound_idx(dynar, idx);

  {
    const size_t old_used = dynar->used;
    const size_t new_used = old_used + 1;

    _gras_dynar_expand(dynar, new_used);

    {
      const size_t nb_shift =  old_used - idx;
      const size_t elmsize  =  dynar->elmsize;

      const size_t offset   =  nb_shift*elmsize;

      void * const elm_src  = _gras_dynar_elm(dynar, idx);
      void * const elm_dst  = _gras_dynar_elm(dynar, idx+1);

      memmove(elm_dst, elm_src, offset);
    }

    _gras_dynar_put_elm(dynar, idx, src);
    dynar->used = new_used;
  }
}

/**
 * gras_dynar_remove_at:
 * @dynar: 
 * @idx:
 * @object:
 *
 * Get the Nth element of a dynar, removing it from the dynar and moving
 * all subsequent values to one position left in the dynar.
 */
void
gras_dynar_remove_at(gras_dynar_t * const dynar,
                     const int            idx,
                     void         * const object) {

  __sanity_check_dynar(dynar);
  __sanity_check_idx(idx);
  __check_inbound_idx(dynar, idx);

  if (object)
    _gras_dynar_get_elm(object, dynar, idx);

  {
    const size_t old_used = dynar->used;
    const size_t new_used = old_used - 1;

    const size_t nb_shift =  old_used-1 - idx;
    const size_t elmsize  =  dynar->elmsize;

    const size_t offset   =  nb_shift*elmsize;

    void * const elm_src  = _gras_dynar_elm(dynar, idx+1);
    void * const elm_dst  = _gras_dynar_elm(dynar, idx);

    memmove(elm_dst, elm_src, offset);

    dynar->used = new_used;
  }
}

/**
 * gras_dynar_push:
 * @dynar:
 * @src:
 *
 * Add an element at the end of the dynar
 */
void
gras_dynar_push(gras_dynar_t * const dynar,
                const void   * const src) {
  __sanity_check_dynar(dynar);
  gras_dynar_insert_at(dynar, dynar->used, src);
}

/**
 * gras_dynar_pop:
 * @dynar:
 * @dst:
 *
 * Get and remove the last element of the dynar
 */
void
gras_dynar_pop(gras_dynar_t * const dynar,
               void         * const dst) {
  __sanity_check_dynar(dynar);
  __check_populated_dynar(dynar);
  DEBUG1("Pop %p",(void*)dynar);
  gras_dynar_remove_at(dynar, dynar->used-1, dst);
}

/**
 * gras_dynar_unshift:
 * @dynar:
 * @src:
 *
 * Add an element at the begining of the dynar (rather long, Use
 * gras_dynar_push() when possible)
 */
void
gras_dynar_unshift(gras_dynar_t * const dynar,
                   const void   * const src) {
  __sanity_check_dynar(dynar);
  gras_dynar_insert_at(dynar, 0, src);
}

/**
 * gras_dynar_shift:
 * @dynar:
 * @dst:
 *
 * Get and remove the first element of the dynar (rather long, Use
 * gras_dynar_pop() when possible)
 */
void
gras_dynar_shift(gras_dynar_t * const dynar,
                 void         * const dst) {

  __sanity_check_dynar(dynar);
  __check_populated_dynar(dynar);
  gras_dynar_remove_at(dynar, 0, dst);
}

/**
 * gras_dynar_map:
 * @dynar:
 * @operator:
 *
 * Apply a function to each member of a dynar (this function may change the
 * value of the element itself, but should not mess with the dynar).
 */
void
gras_dynar_map(const gras_dynar_t * const dynar,
               void_f_pvoid_t     * const operator) {

  __sanity_check_dynar(dynar);

  {
    char         elm[64];
    const size_t used = dynar->used;
    size_t       i    = 0;

    for (i = 0; i < used; i++) {
      _gras_dynar_get_elm(elm, dynar, i);
      operator(elm);
    }
  }
}

/**
 * gras_dynar_first:
 *
 * Put the cursor at the begining of the dynar. (actually, one step before
 * the begining, so that you can iterate over the dynar with a for loop).
 *
 * Dynar cursor are as dumb as possible. If you insert or remove elements
 * from the dynar between the creation and end, you'll fuck up your
 * cursors.
 *
 */
void
gras_dynar_cursor_first(const gras_dynar_t * const dynar,
			int                * const cursor) {

  __sanity_check_dynar(dynar);
  DEBUG1("Set cursor on %p to the first position",(void*)dynar);
  *cursor = 0;
}

/**
 * gras_dynar_cursor_step:
 *
 * Move the cursor to the next value (and return true), or return false.
 */
void
gras_dynar_cursor_step(const gras_dynar_t * const dynar,
		       int                * const cursor) {
  
  __sanity_check_dynar(dynar);
  (*cursor)++;
}

/**
 * gras_dynar_cursor_get:
 *
 * Get the current value of the cursor
 */
int
gras_dynar_cursor_get(const gras_dynar_t * const dynar,
		      int                * const cursor,
		      void               * const dst) {

  __sanity_check_dynar(dynar);
  {

    const int idx = *cursor;

    if (idx >= dynar->used) {
      DEBUG1("Cursor on %p already on last elem",(void*)dynar);
      return FALSE;
    }
    DEBUG2("Cash out cursor on %p at %d",(void*)dynar,idx);

    _gras_dynar_get_elm(dst, dynar, idx);
  }
  return TRUE;

}

/**
 * gras_dynar_cursor_rm:
 * @dynar:
 * @cursor:
 *
 * Remove (free) the entry pointed by the cursor, for use in the middle of a foreach
 */
void gras_dynar_cursor_rm(gras_dynar_t * dynar,
			  int          * const cursor) {
  void *dst;

  if (dynar->elmsize > sizeof(void*)) {
    DEBUG0("Elements too big to fit into a pointer");
    if (dynar->free) {
      dst=gras_malloc(dynar->elmsize);
      gras_dynar_remove_at(dynar,(*cursor)--,dst);
      (dynar->free)(dst);
      gras_free(dst);
    } else {
      DEBUG0("Ok, we dont care about the element when no free function");
      gras_dynar_remove_at(dynar,(*cursor)--,NULL);
    }
      
  } else {
    gras_dynar_remove_at(dynar,(*cursor)--,&dst);
    if (dynar->free)
      (dynar->free)(dst);
  }
}
