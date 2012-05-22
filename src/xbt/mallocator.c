/* mallocator - recycle objects to avoid malloc() / free()                  */

/* Copyright (c) 2006-2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/mallocator.h"
#include "xbt/asserts.h"
#include "xbt/sysdep.h"
#include "mc/mc.h" /* kill mallocators when model-checking is enabled */
#include "mallocator_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_mallocator, xbt, "Mallocators");


/* Change to 0 to completely disable mallocators. */
#define MALLOCATOR_IS_WANTED 1

/* Mallocators and memory mess introduced by model-checking do not mix well
 * together: the mallocator will give standard memory when we are using raw
 * memory (so these blocks are killed on restore) and the contrary (so these
 * blocks will leak accross restores).
 */
#define MALLOCATOR_IS_ENABLED (MALLOCATOR_IS_WANTED && !MC_IS_ENABLED)

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
 * when you extract an object from the mallocator (can be NULL)
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

  xbt_assert(size > 0, "size must be positive");
  xbt_assert(new_f != NULL && free_f != NULL, "invalid parameter");

  m = xbt_new0(s_xbt_mallocator_t, 1);
  XBT_VERB("Create mallocator %p", m);
  m->current_size = 0;
  m->new_f = new_f;
  m->free_f = free_f;
  m->reset_f = reset_f;

  if (MALLOCATOR_IS_ENABLED) {
    m->objects = xbt_new0(void *, size);
    m->max_size = size;
    m->mutex = xbt_os_mutex_init();
  } else {
    if (!MALLOCATOR_IS_WANTED) /* Warn to avoid to commit debugging settings */
      XBT_WARN("Mallocator is disabled!");
    m->objects = NULL;
    m->max_size = 0;
    m->mutex = NULL;
  }
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
  xbt_assert(m != NULL, "Invalid parameter");

  XBT_VERB("Frees mallocator %p (size:%d/%d)", m, m->current_size,
        m->max_size);
  for (i = 0; i < m->current_size; i++) {
    m->free_f(m->objects[i]);
  }
  xbt_free(m->objects);
  xbt_os_mutex_destroy(m->mutex);
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
 * In both cases, the function reset_f() (if defined) is called on the object.
 *
 * \see xbt_mallocator_release()
 */
void *xbt_mallocator_get(xbt_mallocator_t m)
{
  void *object;

  if (MALLOCATOR_IS_ENABLED) {
    xbt_os_mutex_acquire(m->mutex);
    if (m->current_size <= 0) {
      /* No object is ready yet. Create a bunch of them to try to group the
       * mallocs on the same memory pages (to help the cache lines) */

      /* XBT_DEBUG("Create a new object for mallocator %p (size:%d/%d)", */
      /*           m, m->current_size, m->max_size); */
      int i;
      int amount = MIN(m->max_size / 2, 1000);
      for (i = 0; i < amount; i++)
        m->objects[i] = m->new_f();
      m->current_size = amount;
    }

    /* there is at least an available object, now */
    /* XBT_DEBUG("Reuse an old object for mallocator %p (size:%d/%d)", */
    /*           m, m->current_size, m->max_size); */
    object = m->objects[--m->current_size];
    xbt_os_mutex_release(m->mutex);
  } else {
    object = m->new_f();
  }

  if (m->reset_f)
    m->reset_f(object);
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
  if (MALLOCATOR_IS_ENABLED) {
    xbt_os_mutex_acquire(m->mutex);
    if (m->current_size < m->max_size) {
      /* there is enough place to push the object */
      /* XBT_DEBUG
         ("Store deleted object in mallocator %p for further use (size:%d/%d)",
         m, m->current_size, m->max_size); */
      m->objects[m->current_size++] = object;
      xbt_os_mutex_release(m->mutex);
    } else {
      xbt_os_mutex_release(m->mutex);
      /* otherwise we don't have a choice, we must free the object */
      /* XBT_DEBUG("Free deleted object: mallocator %p is full (size:%d/%d)", m,
         m->current_size, m->max_size); */
      m->free_f(object);
    }
  } else {
    m->free_f(object);
  }
}
