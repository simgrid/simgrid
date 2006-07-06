/* $Id$ */

/* a generic DYNamic ARray implementation.                                  */

/* Copyright (c) 2003, 2004 Martin Quinson. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "portable.h" /* SIZEOF_MAX */
#include "xbt/misc.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/ex.h"
#include "xbt/dynar.h"
#include <sys/types.h>

#include "xbt/dynar_private.h" /* type definition, which we share with the 
	 		 	  code in charge of sending this across the net */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(dynar,xbt,"Dynamic arrays");


#define __sanity_check_dynar(dynar)       \
           xbt_assert0(dynar,           \
			"dynar is NULL")
#define __sanity_check_idx(idx)                \
           xbt_assert1(idx >= 0,             \
			"dynar idx(=%d) < 0", \
			(int) (idx))
#define __check_inbound_idx(dynar, idx)                                                \
           xbt_assert2(idx < dynar->used,                                             \
			"dynar is not that long. You asked %d, but it's only %lu long", \
			(int) (idx), (unsigned long) dynar->used)
#define __check_sloppy_inbound_idx(dynar, idx)                                         \
           xbt_assert2(idx <= dynar->used,                                            \
			"dynar is not that long. You asked %d, but it's only %lu long (could have been equal to it)", \
			(int) (idx), (unsigned long) dynar->used)
#define __check_populated_dynar(dynar)            \
           xbt_assert1(dynar->used,              \
			"dynar %p contains nothing",(void*)dynar)

static _XBT_INLINE 
void _xbt_clear_mem(void * const ptr,
		     const unsigned long length) {
  memset(ptr, 0, length);
}

static _XBT_INLINE
void
_xbt_dynar_expand(xbt_dynar_t const dynar,
                   const int          nb) {
  const unsigned long old_size    = dynar->size;

  if (nb > old_size) {
    char * const old_data    = (char *) dynar->data;

    const unsigned long elmsize     = dynar->elmsize;
    const unsigned long old_length  = old_size*elmsize;

    const unsigned long used        = dynar->used;
    const unsigned long used_length = used*elmsize;

    const unsigned long new_size    = nb > (2*(old_size+1)) ? nb : (2*(old_size+1));
    const unsigned long new_length  = new_size*elmsize;
    char * const new_data    = (char *) xbt_malloc0(elmsize*new_size);

    DEBUG3("expend %p from %lu to %d elements", (void*)dynar, (unsigned long)old_size, nb);

    if (old_data) {
      memcpy(new_data, old_data, used_length);
      _xbt_clear_mem(old_data, old_length);
      free(old_data);
    }

    _xbt_clear_mem(new_data + used_length, new_length - used_length);

    dynar->size = new_size;
    dynar->data = new_data;
  }
}

static _XBT_INLINE
void *
_xbt_dynar_elm(const xbt_dynar_t  dynar,
		const unsigned long idx) {
  char * const data    = (char*) dynar->data;
  const unsigned long elmsize = dynar->elmsize;

  return data + idx*elmsize;
}

static _XBT_INLINE
void
_xbt_dynar_get_elm(void  * const       dst,
                    const xbt_dynar_t  dynar,
                    const unsigned long idx) {
  void * const elm     = _xbt_dynar_elm(dynar, idx);

  memcpy(dst, elm, dynar->elmsize);
}

static _XBT_INLINE
void
_xbt_dynar_put_elm(const xbt_dynar_t  dynar,
                    const unsigned long idx,
                    const void * const  src) {
  void * const elm     = _xbt_dynar_elm(dynar, idx);
  const unsigned long elmsize = dynar->elmsize;

  memcpy(elm, src, elmsize);
}

void
xbt_dynar_dump(xbt_dynar_t dynar) {
  INFO5("Dynar dump: size=%lu; used=%lu; elmsize=%lu; data=%p; free_f=%p",
	dynar->size, dynar->used, dynar->elmsize, dynar->data, dynar->free_f);
}	

/** @brief Constructor
 * 
 * \param elmsize size of each element in the dynar
 * \param free_f function to call each time we want to get rid of an element (or NULL if nothing to do).
 *
 * Creates a new dynar. If a free_func is provided, the elements have to be
 * pointer of pointer. That is to say that dynars can contain either base
 * types (int, char, double, etc) or pointer of pointers (struct **).
 */
xbt_dynar_t 
xbt_dynar_new(const unsigned long elmsize,
               void_f_pvoid_t * const free_f) {
   
  xbt_dynar_t dynar = xbt_new0(s_xbt_dynar_t,1);

  dynar->size    = 0;
  dynar->used    = 0;
  dynar->elmsize = elmsize;
  dynar->data    = NULL;
  dynar->free_f    = free_f;

  return dynar;
}

/** @brief Destructor of the structure not touching to the content
 * 
 * \param dynar poor victim
 *
 * kilkil a dynar BUT NOT its content. Ie, the array is freed, but the content
 * is not touched (the \a free_f function is not used)
 */
void
xbt_dynar_free_container(xbt_dynar_t *dynar) {
  if (dynar && *dynar) {

    if ((*dynar)->data) {
      _xbt_clear_mem((*dynar)->data, (*dynar)->size);
      free((*dynar)->data);
    }

    _xbt_clear_mem(*dynar, sizeof(s_xbt_dynar_t));

    free(*dynar);
    *dynar=NULL;
  }
}

/** @brief Frees the content and set the size to 0
 *
 * \param dynar who to squeeze
 */
void
xbt_dynar_reset(xbt_dynar_t const dynar) {

  __sanity_check_dynar(dynar);

  DEBUG1("Reset the dynar %p",(void*)dynar);
  if (dynar->free_f) {
    xbt_dynar_map(dynar, dynar->free_f);
  }
/*
  if (dynar->data)
    free(dynar->data);

  dynar->size = 0;
  */
  dynar->used = 0;
/*  dynar->data = NULL;*/
}

/** @brief Destructor
 * 
 * \param dynar poor victim
 *
 * kilkil a dynar and its content
 */

void
xbt_dynar_free(xbt_dynar_t * dynar) {
  if (dynar && *dynar) {
    xbt_dynar_reset(*dynar);
    xbt_dynar_free_container(dynar);
  }
}
/** \brief free a dynar passed as void* (handy to store dynar in dynars or dict) */
void xbt_dynar_free_voidp(void *d) {
   xbt_dynar_free( (xbt_dynar_t*) d);
}
   
/** @brief Count of dynar's elements
 * 
 * \param dynar the dynar we want to mesure
 */
unsigned long
xbt_dynar_length(const xbt_dynar_t dynar) {
  return (dynar ? (unsigned long) dynar->used : (unsigned long)0);
}

/** @brief Retrieve a copy of the Nth element of a dynar.
 *
 * \param dynar information dealer
 * \param idx index of the slot we want to retrieve
 * \param[out] dst where to put the result to.
 */
void
xbt_dynar_get_cpy(const xbt_dynar_t dynar,
		   const int          idx,
		   void       * const dst) {

  __sanity_check_dynar(dynar);
  __sanity_check_idx(idx);
  __check_inbound_idx(dynar, idx);

  _xbt_dynar_get_elm(dst, dynar, idx);
}

/** @brief Retrieve a pointer to the Nth element of a dynar.
 *
 * \param dynar information dealer
 * \param idx index of the slot we want to retrieve
 * \return the \a idx-th element of \a dynar.
 *
 * \warning The returned value is the actual content of the dynar. 
 * Make a copy before fooling with it.
 */
void*
xbt_dynar_get_ptr(const xbt_dynar_t dynar, const int idx) {

  __sanity_check_dynar(dynar);
  __sanity_check_idx(idx);
  __check_inbound_idx(dynar, idx);

  return _xbt_dynar_elm(dynar, idx);
}

/** @brief Set the Nth element of a dynar (expended if needed). Previous value at this position is NOT freed
 * 
 * \param dynar information dealer
 * \param idx index of the slot we want to modify
 * \param src What will be feeded to the dynar
 *
 * If you want to free the previous content, use xbt_dynar_replace().
 */
void
xbt_dynar_set(xbt_dynar_t         dynar,
               const int            idx,
               const void   * const src) {

  __sanity_check_dynar(dynar);
  __sanity_check_idx(idx);

  _xbt_dynar_expand(dynar, idx+1);

  if (idx >= dynar->used) {
    dynar->used = idx+1;
  }

  _xbt_dynar_put_elm(dynar, idx, src);
}

/** @brief Set the Nth element of a dynar (expended if needed). Previous value is freed
 *
 * \param dynar
 * \param idx
 * \param object
 *
 * Set the Nth element of a dynar, expanding the dynar if needed, AND DO
 * free the previous value at this position. If you don't want to free the
 * previous content, use xbt_dynar_set().
 */
void
xbt_dynar_replace(xbt_dynar_t         dynar,
		   const int            idx,
		   const void   * const object) {

  __sanity_check_dynar(dynar);
  __sanity_check_idx(idx);

  if (idx < dynar->used && dynar->free_f) {
    void * const old_object = _xbt_dynar_elm(dynar, idx);

    dynar->free_f(old_object);
  }

  xbt_dynar_set(dynar, idx, object);
}

/** @brief Make room for a new element, and return a pointer to it
 * 
 * You can then use regular affectation to set its value instead of relying 
 * on the slow memcpy. This is what xbt_dynar_insert_at_as() does.
 */
void *
xbt_dynar_insert_at_ptr(xbt_dynar_t const dynar,
			const int            idx) {
   
  __sanity_check_dynar(dynar);
  __sanity_check_idx(idx);
  __check_sloppy_inbound_idx(dynar, idx);

  {
    const unsigned long old_used = dynar->used;
    const unsigned long new_used = old_used + 1;

    _xbt_dynar_expand(dynar, new_used);

    {
      const unsigned long nb_shift =  old_used - idx;

      if (nb_shift)
	 memmove(_xbt_dynar_elm(dynar, idx+1), 
		 _xbt_dynar_elm(dynar, idx), 
		 nb_shift * dynar->elmsize);
    }

    dynar->used = new_used;
    return _xbt_dynar_elm(dynar,idx);
  }
}

/** @brief Set the Nth dynar's element, expending the dynar and sliding the previous values to the right
 * 
 * Set the Nth element of a dynar, expanding the dynar if needed, and
 * moving the previously existing value and all subsequent ones to one
 * position right in the dynar.
 */
void
xbt_dynar_insert_at(xbt_dynar_t  const dynar,
		    const int            idx,
		    const void   * const src) {

  /* checks done in xbt_dynar_insert_at_ptr */
  memcpy(xbt_dynar_insert_at_ptr(dynar,idx),
	 src,
	 dynar->elmsize);
}

/** @brief Remove the Nth dynar's element, sliding the previous values to the left
 *
 * Get the Nth element of a dynar, removing it from the dynar and moving
 * all subsequent values to one position left in the dynar.
 */
void
xbt_dynar_remove_at(xbt_dynar_t  const dynar,
                     const int            idx,
                     void         * const object) {

  unsigned long nb_shift;
  unsigned long offset;

  __sanity_check_dynar(dynar);
  __sanity_check_idx(idx);
  __check_inbound_idx(dynar, idx);

  if (object) {
    _xbt_dynar_get_elm(object, dynar, idx);
  } else if (dynar->free_f) {
    if (dynar->elmsize <= SIZEOF_MAX) {
       char elm[SIZEOF_MAX];
       _xbt_dynar_get_elm(elm, dynar, idx);
       (*dynar->free_f)(elm);
    } else {
       char *elm=malloc(dynar->elmsize);
       _xbt_dynar_get_elm(elm, dynar, idx);
       (*dynar->free_f)(elm);
       free(elm);
    }
  }

  nb_shift =  dynar->used-1 - idx;
  offset   =  nb_shift * dynar->elmsize;

  memmove(_xbt_dynar_elm(dynar, idx),
          _xbt_dynar_elm(dynar, idx+1), 
          offset);

  dynar->used--;
}

/** @brief Returns the position of the element in the dynar
 *
 * Raises not_found_error if not found.
 */
int
xbt_dynar_search(xbt_dynar_t  const dynar,
		 void        *const elem) {
  int it;
   
  for (it=0; it< dynar->size; it++) 
    if (!memcmp(_xbt_dynar_elm(dynar, it),elem,dynar->elmsize))
      return it;

  THROW2(not_found_error,0,"Element %p not part of dynar %p",elem,dynar);
}

/** @brief Returns a boolean indicating whether the element is part of the dynar */
int
xbt_dynar_member(xbt_dynar_t  const dynar,
		 void        *const elem) {

  xbt_ex_t e;
   
  TRY {
     xbt_dynar_search(dynar,elem);
  } CATCH(e) {
     if (e.category == not_found_error) {
	xbt_ex_free(e);
	return 0;
     }
     RETHROW;
  }
  return 1;
}

/** @brief Make room at the end of the dynar for a new element, and return a pointer to it.
 *
 * You can then use regular affectation to set its value instead of relying 
 * on the slow memcpy. This is what xbt_dynar_push_as() does.
 */
void *
xbt_dynar_push_ptr(xbt_dynar_t  const dynar) {
  return xbt_dynar_insert_at_ptr(dynar, dynar->used);    
}

/** @brief Add an element at the end of the dynar */
void
xbt_dynar_push(xbt_dynar_t  const dynar,
                const void   * const src) {
  /* sanity checks done by insert_at */
  xbt_dynar_insert_at(dynar, dynar->used, src); 
}

/** @brief Mark the last dynar's element as unused and return a pointer to it.
 *
 * You can then use regular affectation to set its value instead of relying 
 * on the slow memcpy. This is what xbt_dynar_pop_as() does.
 */
void *
xbt_dynar_pop_ptr(xbt_dynar_t  const dynar) {

  __check_populated_dynar(dynar);
  DEBUG1("Pop %p",(void*)dynar);
  dynar->used--;
  return _xbt_dynar_elm(dynar,dynar->used);
}

/** @brief Get and remove the last element of the dynar */
void
xbt_dynar_pop(xbt_dynar_t  const dynar,
              void         * const dst) {

  /* sanity checks done by remove_at */
  DEBUG1("Pop %p",(void*)dynar);
  xbt_dynar_remove_at(dynar, dynar->used-1, dst);
}

/** @brief Add an element at the begining of the dynar.
 *
 * This is less efficient than xbt_dynar_push()
 */
void
xbt_dynar_unshift(xbt_dynar_t  const dynar,
                   const void   * const src) {
  
  /* sanity checks done by insert_at */
  xbt_dynar_insert_at(dynar, 0, src);
}

/** @brief Get and remove the first element of the dynar.
 *
 * This is less efficient than xbt_dynar_pop()
 */
void
xbt_dynar_shift(xbt_dynar_t  const dynar,
                 void         * const dst) {

  /* sanity checks done by remove_at */
  xbt_dynar_remove_at(dynar, 0, dst);
}

/** @brief Apply a function to each member of a dynar
 *
 * The mapped function may change the value of the element itself, 
 * but should not mess with the structure of the dynar.
 */
void
xbt_dynar_map(const xbt_dynar_t  dynar,
               void_f_pvoid_t     * const op) {

  __sanity_check_dynar(dynar);

  {
    char         elm[SIZEOF_MAX];
    const unsigned long used = dynar->used;
    unsigned long       i    = 0;

    for (i = 0; i < used; i++) {
      _xbt_dynar_get_elm(elm, dynar, i);
      op(elm);
    }
  }
}

/** @brief Put the cursor at the begining of the dynar.
 *
 * Actually, the cursor is set one step before the begining, so that you
 * can iterate over the dynar with a for loop.
 */
void
xbt_dynar_cursor_first(const xbt_dynar_t dynar,
		       int        * const cursor) {

  DEBUG1("Set cursor on %p to the first position",(void*)dynar);
  *cursor = 0;
}

/** @brief Move the cursor to the next value */
void
xbt_dynar_cursor_step(const xbt_dynar_t dynar,
		       int        * const cursor) {
  
  (*cursor)++;
}

/** @brief Get the data currently pointed by the cursor */
int
xbt_dynar_cursor_get(const xbt_dynar_t dynar,
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

    _xbt_dynar_get_elm(dst, dynar, idx);
  }
  return TRUE;

}

/** @brief Removes and free the entry pointed by the cursor 
 *
 * This function can be used while traversing without problem.
 */
void xbt_dynar_cursor_rm(xbt_dynar_t dynar,
			  int          * const cursor) {
  void *dst;

  if (dynar->elmsize > sizeof(void*)) {
    DEBUG0("Elements too big to fit into a pointer");
    if (dynar->free_f) {
      dst=xbt_malloc(dynar->elmsize);
      xbt_dynar_remove_at(dynar,(*cursor)--,dst);
      (dynar->free_f)(dst);
      free(dst);
    } else {
      DEBUG0("Ok, we dont care about the element without free function");
      xbt_dynar_remove_at(dynar,(*cursor)--,NULL);
    }
      
  } else {
    xbt_dynar_remove_at(dynar,(*cursor)--,&dst);
    if (dynar->free_f)
      (dynar->free_f)(dst);
  }
}

#ifdef SIMGRID_TEST

#define NB_ELEM 5000

XBT_TEST_SUITE("dynar","Dynar data container");
XBT_LOG_EXTERNAL_CATEGORY(dynar);
XBT_LOG_DEFAULT_CATEGORY(dynar);

XBT_TEST_UNIT("int",test_dynar_int,"Dyars of integers") {
   /* Vars_decl [doxygen cruft] */
   xbt_dynar_t d;
   int i,cpt,cursor;
   int *iptr;
   
   xbt_test_add0("==== Traverse the empty dynar");
   d=xbt_dynar_new(sizeof(int),NULL);
   xbt_dynar_foreach(d,cursor,i){
     xbt_assert0(0,"Damnit, there is something in the empty dynar");
   }
   xbt_dynar_free(&d);
   xbt_dynar_free(&d);

   xbt_test_add1("==== Push %d int, set them again 3 times, traverse them, shift them",
	NB_ELEM);
   /* Populate_ints [doxygen cruft] */
   /* 1. Populate the dynar */
   d=xbt_dynar_new(sizeof(int),NULL);
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     xbt_dynar_push_as(d,int,cpt); /* This is faster (and possible only with scalars) */
     /* xbt_dynar_push(d,&cpt);       This would also work */
     xbt_test_log2("Push %d, length=%lu",cpt, xbt_dynar_length(d));
   }
   
   /* 2. Traverse manually the dynar */
   for (cursor=0; cursor< NB_ELEM; cursor++) {
     iptr=xbt_dynar_get_ptr(d,cursor);
     xbt_test_assert2(cursor == *iptr,
		      "The retrieved value is not the same than the injected one (%d!=%d)",
		      cursor,cpt);
   }
   
   /* 3. Traverse the dynar using the neat macro to that extend */
   xbt_dynar_foreach(d,cursor,cpt){
     xbt_test_assert2(cursor == cpt,
		      "The retrieved value is not the same than the injected one (%d!=%d)",
		      cursor,cpt);
   }
   /* end_of_traversal */
   
   for (cpt=0; cpt< NB_ELEM; cpt++)
     *(int*)xbt_dynar_get_ptr(d,cpt) = cpt;

   for (cpt=0; cpt< NB_ELEM; cpt++) 
     *(int*)xbt_dynar_get_ptr(d,cpt) = cpt;
/*     xbt_dynar_set(d,cpt,&cpt);*/
   
   for (cpt=0; cpt< NB_ELEM; cpt++) 
     *(int*)xbt_dynar_get_ptr(d,cpt) = cpt;
   
   cpt=0;
   xbt_dynar_foreach(d,cursor,i){
     xbt_test_assert2(i == cpt,
		      "The retrieved value is not the same than the injected one (%d!=%d)",
		      i,cpt);
     cpt++;
   }
   xbt_test_assert2(cpt == NB_ELEM,
		    "Cannot retrieve my %d values. Last got one is %d",
		    NB_ELEM, cpt);

   /* shifting [doxygen cruft] */
   /* 4. Shift all the values */
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     xbt_dynar_shift(d,&i);
     xbt_test_assert2(i == cpt,
		      "The retrieved value is not the same than the injected one (%d!=%d)",
		      i,cpt);
     xbt_test_log2("Pop %d, length=%lu",cpt, xbt_dynar_length(d));
   }
   
   /* 5. Free the resources */
   xbt_dynar_free(&d);
   xbt_dynar_free(&d);

   
   xbt_test_add1("==== Unshift/pop %d int",NB_ELEM);
   d=xbt_dynar_new(sizeof(int),NULL);
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     xbt_dynar_unshift(d,&cpt);
     DEBUG2("Push %d, length=%lu",cpt, xbt_dynar_length(d));
   }
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     i=xbt_dynar_pop_as(d,int);
     xbt_test_assert2(i == cpt,
		      "The retrieved value is not the same than the injected one (%d!=%d)",
		      i,cpt);
     xbt_test_log2("Pop %d, length=%lu",cpt, xbt_dynar_length(d));
   }
   xbt_dynar_free(&d);
   xbt_dynar_free(&d);

   
   xbt_test_add1("==== Push %d int, insert 1000 int in the middle, shift everything",NB_ELEM);
   d=xbt_dynar_new(sizeof(int),NULL);
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     xbt_dynar_push_as(d,int,cpt);
     DEBUG2("Push %d, length=%lu",cpt, xbt_dynar_length(d));
   }
   for (cpt=0; cpt< 1000; cpt++) {
     xbt_dynar_insert_at_as(d,2500,int,cpt);
     DEBUG2("Push %d, length=%lu",cpt, xbt_dynar_length(d));
   }

   for (cpt=0; cpt< 2500; cpt++) {
     xbt_dynar_shift(d,&i);
     xbt_test_assert2(i == cpt,
	     "The retrieved value is not the same than the injected one at the begining (%d!=%d)",
	       i,cpt);
     DEBUG2("Pop %d, length=%lu",cpt, xbt_dynar_length(d));
   }
   for (cpt=999; cpt>=0; cpt--) {
     xbt_dynar_shift(d,&i);
     xbt_test_assert2(i == cpt,
           "The retrieved value is not the same than the injected one in the middle (%d!=%d)",
		      i,cpt);
   }
   for (cpt=2500; cpt< NB_ELEM; cpt++) {
     xbt_dynar_shift(d,&i);
      xbt_test_assert2(i == cpt,
           "The retrieved value is not the same than the injected one at the end (%d!=%d)",
		       i,cpt);
   }
   xbt_dynar_free(&d);
   xbt_dynar_free(&d);


   xbt_test_add1("==== Push %d int, remove 2000-4000. free the rest",NB_ELEM);
   d=xbt_dynar_new(sizeof(int),NULL);
   for (cpt=0; cpt< NB_ELEM; cpt++) 
     xbt_dynar_push_as(d,int,cpt);
   
   for (cpt=2000; cpt< 4000; cpt++) {
     xbt_dynar_remove_at(d,2000,&i);
     xbt_test_assert2(i == cpt,
		      "Remove a bad value. Got %d, expected %d",
		      i,cpt);
     DEBUG2("remove %d, length=%lu",cpt, xbt_dynar_length(d));
   }
   xbt_dynar_free(&d);
   xbt_dynar_free(&d);
}

XBT_TEST_UNIT("double",test_dynar_double,"Dyars of doubles") {
   xbt_dynar_t d;
   int cpt,cursor;
   double d1,d2;
   
   xbt_test_add0("==== Traverse the empty dynar");
   d=xbt_dynar_new(sizeof(int),NULL);
   xbt_dynar_foreach(d,cursor,cpt){
     xbt_test_assert0(FALSE,
	     "Damnit, there is something in the empty dynar");
   }
   xbt_dynar_free(&d);
   xbt_dynar_free(&d);

   xbt_test_add0("==== Push/shift 5000 doubles");
   d=xbt_dynar_new(sizeof(double),NULL);
   for (cpt=0; cpt< 5000; cpt++) {
     d1=(double)cpt;
     xbt_dynar_push(d,&d1);
   }
   xbt_dynar_foreach(d,cursor,d2){
     d1=(double)cursor;
     xbt_test_assert2(d1 == d2,
           "The retrieved value is not the same than the injected one (%f!=%f)",
		  d1,d2);
   }
   for (cpt=0; cpt< 5000; cpt++) {
     d1=(double)cpt;
     xbt_dynar_shift(d,&d2);
     xbt_test_assert2(d1 == d2,
           "The retrieved value is not the same than the injected one (%f!=%f)",
		  d1,d2);
   }
   xbt_dynar_free(&d);
   xbt_dynar_free(&d);


   xbt_test_add0("==== Unshift/pop 5000 doubles");
   d=xbt_dynar_new(sizeof(double),NULL);
   for (cpt=0; cpt< 5000; cpt++) {
     d1=(double)cpt;
     xbt_dynar_unshift(d,&d1);
   }
   for (cpt=0; cpt< 5000; cpt++) {
     d1=(double)cpt;
     xbt_dynar_pop(d,&d2);
     xbt_test_assert2 (d1 == d2,
           "The retrieved value is not the same than the injected one (%f!=%f)",
		   d1,d2);
   }
   xbt_dynar_free(&d);
   xbt_dynar_free(&d);



   xbt_test_add0("==== Push 5000 doubles, insert 1000 doubles in the middle, shift everything");
   d=xbt_dynar_new(sizeof(double),NULL);
   for (cpt=0; cpt< 5000; cpt++) {
     d1=(double)cpt;
     xbt_dynar_push(d,&d1);
   }
   for (cpt=0; cpt< 1000; cpt++) {
     d1=(double)cpt;
     xbt_dynar_insert_at(d,2500,&d1);
   }

   for (cpt=0; cpt< 2500; cpt++) {
     d1=(double)cpt;
     xbt_dynar_shift(d,&d2);
     xbt_test_assert2(d1 == d2,
           "The retrieved value is not the same than the injected one at the begining (%f!=%f)",
		  d1,d2);
     DEBUG2("Pop %d, length=%lu",cpt, xbt_dynar_length(d));
   }
   for (cpt=999; cpt>=0; cpt--) {
     d1=(double)cpt;
     xbt_dynar_shift(d,&d2);
     xbt_test_assert2 (d1 == d2,
           "The retrieved value is not the same than the injected one in the middle (%f!=%f)",
		   d1,d2);
   }
   for (cpt=2500; cpt< 5000; cpt++) {
     d1=(double)cpt;
     xbt_dynar_shift(d,&d2);
     xbt_test_assert2 (d1 == d2,
           "The retrieved value is not the same than the injected one at the end (%f!=%f)",
		   d1,d2);
   }
   xbt_dynar_free(&d);
   xbt_dynar_free(&d);


   xbt_test_add0("==== Push 5000 double, remove 2000-4000. free the rest");
   d=xbt_dynar_new(sizeof(double),NULL);
   for (cpt=0; cpt< 5000; cpt++) {
     d1=(double)cpt;
     xbt_dynar_push(d,&d1);
   }
   for (cpt=2000; cpt< 4000; cpt++) {
     d1=(double)cpt;
     xbt_dynar_remove_at(d,2000,&d2);
     xbt_test_assert2 (d1 == d2,
           "Remove a bad value. Got %f, expected %f",
	       d2,d1);
   }
   xbt_dynar_free(&d);
   xbt_dynar_free(&d);
}


/* doxygen_string_cruft */

/* The function we will use to free the data */
static void free_string(void *d){
  free(*(void**)d);
}

XBT_TEST_UNIT("string",test_dynar_string,"Dyars of strings") {
   xbt_dynar_t d;
   int cpt;
   char buf[1024];
   char *s1,*s2;
   
   xbt_test_add0("==== Traverse the empty dynar");
   d=xbt_dynar_new(sizeof(char *),&free_string);
   xbt_dynar_foreach(d,cpt,s1){
     xbt_test_assert0(FALSE,
		  "Damnit, there is something in the empty dynar");
   }
   xbt_dynar_free(&d);
   xbt_dynar_free(&d);

   xbt_test_add1("==== Push %d strings, set them again 3 times, shift them",NB_ELEM);
   /* Populate_str [doxygen cruft] */
   d=xbt_dynar_new(sizeof(char*),&free_string);
   /* 1. Populate the dynar */
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     xbt_dynar_push(d,&s1);
   }
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     xbt_dynar_replace(d,cpt,&s1);
   }
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     xbt_dynar_replace(d,cpt,&s1);
   }
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     xbt_dynar_replace(d,cpt,&s1);
   }
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     sprintf(buf,"%d",cpt);
     xbt_dynar_shift(d,&s2);
     xbt_test_assert2 (!strcmp(buf,s2),
	    "The retrieved value is not the same than the injected one (%s!=%s)",
		   buf,s2);
     free(s2);
   }
   xbt_dynar_free(&d);
   xbt_dynar_free(&d);


   xbt_test_add1("==== Unshift, traverse and pop %d strings",NB_ELEM);
   d=xbt_dynar_new(sizeof(char**),&free_string);
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     xbt_dynar_unshift(d,&s1);
   }
   /* 2. Traverse the dynar with the macro */
   xbt_dynar_foreach(d,cpt,s1) {
     sprintf(buf,"%d",NB_ELEM - cpt -1);
     xbt_test_assert2 (!strcmp(buf,s1),
           "The retrieved value is not the same than the injected one (%s!=%s)",
	       buf,s1);
   }
   /* 3. Traverse the dynar with the macro */
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     sprintf(buf,"%d",cpt);
     xbt_dynar_pop(d,&s2);
     xbt_test_assert2 (!strcmp(buf,s2),
           "The retrieved value is not the same than the injected one (%s!=%s)",
	       buf,s2);
     free(s2);
   }
   /* 4. Free the resources */
   xbt_dynar_free(&d);
   xbt_dynar_free(&d);


   xbt_test_add2("==== Push %d strings, insert %d strings in the middle, shift everything",NB_ELEM,NB_ELEM/5);
   d=xbt_dynar_new(sizeof(char*),&free_string);
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     xbt_dynar_push(d,&s1);
   }
   for (cpt=0; cpt< NB_ELEM/5; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     xbt_dynar_insert_at(d,NB_ELEM/2,&s1);
   }

   for (cpt=0; cpt< NB_ELEM/2; cpt++) {
     sprintf(buf,"%d",cpt);
     xbt_dynar_shift(d,&s2);
     xbt_test_assert2(!strcmp(buf,s2),
           "The retrieved value is not the same than the injected one at the begining (%s!=%s)",
	       buf,s2);
      free(s2);
   }
   for (cpt=(NB_ELEM/5)-1; cpt>=0; cpt--) {
     sprintf(buf,"%d",cpt);
     xbt_dynar_shift(d,&s2);
     xbt_test_assert2 (!strcmp(buf,s2),
           "The retrieved value is not the same than the injected one in the middle (%s!=%s)",
	       buf,s2);
     free(s2);
   }
   for (cpt=NB_ELEM/2; cpt< NB_ELEM; cpt++) {
     sprintf(buf,"%d",cpt);
     xbt_dynar_shift(d,&s2);
     xbt_test_assert2 (!strcmp(buf,s2),
           "The retrieved value is not the same than the injected one at the end (%s!=%s)",
	       buf,s2);
     free(s2);
   }
   xbt_dynar_free(&d);
   xbt_dynar_free(&d);


   xbt_test_add3("==== Push %d strings, remove %d-%d. free the rest",NB_ELEM,2*(NB_ELEM/5),4*(NB_ELEM/5));
   d=xbt_dynar_new(sizeof(char*),&free_string);
   for (cpt=0; cpt< NB_ELEM; cpt++) {
     sprintf(buf,"%d",cpt);
     s1=strdup(buf);
     xbt_dynar_push(d,&s1);
   }
   for (cpt=2*(NB_ELEM/5); cpt< 4*(NB_ELEM/5); cpt++) {
     sprintf(buf,"%d",cpt);
     xbt_dynar_remove_at(d,2*(NB_ELEM/5),&s2);
     xbt_test_assert2(!strcmp(buf,s2),
		  "Remove a bad value. Got %s, expected %s",
		  s2,buf);
      free(s2);
   }
   xbt_dynar_free(&d); /* end_of_doxygen */
}
#endif /* SIMGRID_TEST */
