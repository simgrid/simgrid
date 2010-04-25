/* mallocator - recycle objects to avoid malloc() / free()                  */

/* Copyright (c) 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/mallocator.h"
#include "xbt/asserts.h"
#include "xbt/sysdep.h"
#include "mallocator_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_mallocator, xbt, "Mallocators");

/**
 * \brief Constructor
 * \param size size of the internal stack: number of objects the mallocator
 * will be able to store
 * \param new_f function to allocate a new object of your datatype, called
 * in \a xbt_mallocator_get() when the mallocator is empty
 * \param free_f function to free an object of your datatype, called
 * in \a xbt_mallocator_release() when the stack is full, and when
 * the mallocator is freed.
 * \param reset_f function to reinitialise an object of your datatype, called
 * when you extract an object from the mallocator
 *
 * Create and initialize a new mallocator for a given datatype.
 *
 * \return pointer to the created mallocator
 * \see xbt_mallocator_free()
 */
xbt_mallocator_t xbt_mallocator_new(int size,
                                    pvoid_f_void_t new_f,
                                    void_f_pvoid_t free_f,
                                    void_f_pvoid_t reset_f)
{


  xbt_mallocator_t m;

  xbt_assert0(size > 0, "size must be positive");
  xbt_assert0(new_f != NULL && free_f != NULL
              && reset_f != NULL, "invalid parameter");

  m = xbt_new0(s_xbt_mallocator_t, 1);
  VERB1("Create mallocator %p", m);
  if (XBT_LOG_ISENABLED(xbt_mallocator, xbt_log_priority_verbose))
    xbt_backtrace_display_current();

  m->objects = xbt_new0(void *, size);
  m->max_size = size;
  m->current_size = 0;
  m->new_f = new_f;
  m->free_f = free_f;
  m->reset_f = reset_f;

  return m;
}

/** \brief Destructor
 * \param m the mallocator you want to destroy
 *
 * Destroy the mallocator and all its data. The function
 * free_f is called on each object in the mallocator.
 *
 * \see xbt_mallocator_new()
 */
void xbt_mallocator_free(xbt_mallocator_t m)
{

  int i;
  xbt_assert0(m != NULL, "Invalid parameter");

  VERB3("Frees mallocator %p (size:%d/%d)", m, m->current_size, m->max_size);
  for (i = 0; i < m->current_size; i++) {
    (*(m->free_f)) (m->objects[i]);
  }
  xbt_free(m->objects);
  xbt_free(m);
}

/**
 * \brief Extract an object from a mallocator
 * \param m a mallocator
 *
 * Remove an object from the mallocator and return it.
 * This function is designed to be used instead of malloc().
 * If the mallocator is not empty, an object is
 * extracted from the mallocator and no malloc is done.
 *
 * If the mallocator is empty, a new object is created,
 * by calling the function new_f().
 *
 * In both cases, the function reset_f() is called on the object.
 *
 * \see xbt_mallocator_release()
 */
void *xbt_mallocator_get(xbt_mallocator_t m)
{
  void *object;
  xbt_assert0(m != NULL, "Invalid parameter");

  if (m->current_size > 0) {
    /* there is at least an available object */
    DEBUG3("Reuse an old object for mallocator %p (size:%d/%d)", m,
           m->current_size, m->max_size);
    object = m->objects[--m->current_size];
  } else {
    /* otherwise we must allocate a new object */
    DEBUG3("Create a new object for mallocator %p (size:%d/%d)", m,
           m->current_size, m->max_size);
    object = (*(m->new_f)) ();
  }
  (*(m->reset_f)) (object);
  return object;
}

/** \brief Push an object into a mallocator
 * \param m a mallocator
 * \param object an object you don't need anymore
 *
 * Push into the mallocator an object you don't need anymore.
 * This function is designed to be used instead of free().
 * If the mallocator is not full, your object if stored into
 * the mallocator and no free is done.
 * If the mallocator is full, the object is freed by calling
 * the function free_f().
 *
 * \see xbt_mallocator_get()
 */
void xbt_mallocator_release(xbt_mallocator_t m, void *object)
{
  xbt_assert0(m != NULL && object != NULL, "Invalid parameter");

  if (m->current_size < m->max_size) {
    /* there is enough place to push the object */
    DEBUG3
      ("Store deleted object in mallocator %p for further use (size:%d/%d)",
       m, m->current_size, m->max_size);
    m->objects[m->current_size++] = object;
  } else {
    /* otherwise we don't have a choice, we must free the object */
    DEBUG3("Free deleted object: mallocator %p is full (size:%d/%d)", m,
           m->current_size, m->max_size);
    (*(m->free_f)) (object);
  }
}
