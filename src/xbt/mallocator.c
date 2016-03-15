/* mallocator - recycle objects to avoid malloc() / free()                  */

/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/internal_config.h"
#include "xbt/mallocator.h"
#include "xbt/asserts.h"
#include "xbt/sysdep.h"
#include "mc/mc.h" /* kill mallocators when model-checking is enabled */
#include "mallocator_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_mallocator, xbt, "Mallocators");

/** Implementation note on the mallocators:
 *
 * Mallocators and memory mess introduced by model-checking do not mix well  together: the mallocator will give
 * standard memory when we are using raw memory (so these blocks are killed on restore) and the contrary (so these
 * blocks will leak across restores).
 *
 * In addition, model-checking is activated when the command-line arguments are parsed, at the beginning of main, while
 * most of the mallocators are created during the constructor functions launched from xbt_preinit, before the beginning
 * of the main function.
 *
 * We want the code as fast as possible when they are active while we can deal with a little slow-down when they are
 * inactive. So we start the mallocators as inactive. When they are so, they check at each use whether they should
 * switch to the fast active mode or should stay in inactive mode. Finally, we give external elements a way to switch
 * them  all to the active mode (through xbt_mallocator_initialization_is_done).
 *
 * This design avoids to store all mallocators somewhere for later conversion, which would be hard to achieve provided
 * that all our data structures use some mallocators internally...
 */

/* Value != 0 when the framework configuration is done.  Value > 1 if the
 * mallocators should be protected from concurrent accesses.  */
static int initialization_done = 0;

static inline void lock_reset(xbt_mallocator_t m)
{
  m->lock = 0;
}

static inline void lock_acquire(xbt_mallocator_t m)
{
  if (initialization_done > 1) {
    int *lock = &m->lock;
    while (__sync_lock_test_and_set(lock, 1))
      /* nop */;
  }
}

static inline void lock_release(xbt_mallocator_t m)
{
  if (initialization_done > 1)
    __sync_lock_release(&m->lock);
}

/**
 * This function must be called once the framework configuration is done. If not, mallocators will never get used.
 * Check the implementation notes in src/xbt/mallocator.c for the justification of this.
 *
 * For example, surf_config uses this function to tell to the mallocators that the simgrid configuration is now
 * finished and that it can create them if not done yet */
void xbt_mallocator_initialization_is_done(int protect)
{
  initialization_done = protect ? 2 : 1;
}

/** used by the module to know if it's time to activate the mallocators yet */
static inline int xbt_mallocator_is_active(void) {
#if HAVE_MALLOCATOR
  return initialization_done && !MC_is_active();
#else
  return 0;
#endif
}

/**
 * \brief Constructor
 * \param size size of the internal stack: number of objects the mallocator will be able to store
 * \param new_f function to allocate a new object of your datatype, called in \a xbt_mallocator_get() when the
 *              mallocator is empty
 * \param free_f function to free an object of your datatype, called in \a xbt_mallocator_release() when the stack is
 *                full, and when the mallocator is freed.
 * \param reset_f function to reinitialise an object of your datatype, called when you extract an object from the
 *                mallocator (can be NULL)
 *
 * Create and initialize a new mallocator for a given datatype.
 *
 * \return pointer to the created mallocator
 * \see xbt_mallocator_free()
 */
xbt_mallocator_t xbt_mallocator_new(int size, pvoid_f_void_t new_f, void_f_pvoid_t free_f, void_f_pvoid_t reset_f)
{
  xbt_mallocator_t m;

  xbt_assert(size > 0, "size must be positive");
  xbt_assert(new_f != NULL && free_f != NULL, "invalid parameter");

  m = xbt_new0(s_xbt_mallocator_t, 1);
  XBT_VERB("Create mallocator %p (%s)", m, xbt_mallocator_is_active() ? "enabled" : "disabled");
  m->current_size = 0;
  m->new_f = new_f;
  m->free_f = free_f;
  m->reset_f = reset_f;
  m->max_size = size;

  return m;
}

/** \brief Destructor
 * \param m the mallocator you want to destroy
 *
 * Destroy the mallocator and all its data. The function free_f is called on each object in the mallocator.
 *
 * \see xbt_mallocator_new()
 */
void xbt_mallocator_free(xbt_mallocator_t m)
{
  int i;
  xbt_assert(m != NULL, "Invalid parameter");

  XBT_VERB("Frees mallocator %p (size:%d/%d)", m, m->current_size, m->max_size);
  for (i = 0; i < m->current_size; i++) {
    m->free_f(m->objects[i]);
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
 * If the mallocator is not empty, an object is extracted from the mallocator and no malloc is done.
 *
 * If the mallocator is empty, a new object is created, by calling the function new_f().
 *
 * In both cases, the function reset_f() (if defined) is called on the object.
 *
 * \see xbt_mallocator_release()
 */
void *xbt_mallocator_get(xbt_mallocator_t m)
{
  void *object;

  if (m->objects != NULL) { // this mallocator is active, stop thinking and go for it!
    lock_acquire(m);
    if (m->current_size <= 0) {
      /* No object is ready yet. Create a bunch of them to try to group the
       * mallocs on the same memory pages (to help the cache lines) */

      /* XBT_DEBUG("Create a new object for mallocator %p (size:%d/%d)", m, m->current_size, m->max_size); */
      int i;
      int amount = MIN(m->max_size / 2, 1000);
      for (i = 0; i < amount; i++)
        m->objects[i] = m->new_f();
      m->current_size = amount;
    }

    /* there is at least an available object, now */
    /* XBT_DEBUG("Reuse an old object for mallocator %p (size:%d/%d)", m, m->current_size, m->max_size); */
    object = m->objects[--m->current_size];
    lock_release(m);
  } else {
    if (xbt_mallocator_is_active()) {
      // We have to switch this mallocator from inactive to active (and then get an object)
      m->objects = xbt_new0(void *, m->max_size);
      lock_reset(m);
      return xbt_mallocator_get(m);
    } else {
      object = m->new_f();
    }
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
 * If the mallocator is not full, your object if stored into the mallocator and no free is done.
 * If the mallocator is full, the object is freed by calling the function free_f().
 *
 * \see xbt_mallocator_get()
 */
void xbt_mallocator_release(xbt_mallocator_t m, void *object)
{
  if (m->objects != NULL) { // Go for it
    lock_acquire(m);
    if (m->current_size < m->max_size) {
      /* there is enough place to push the object */
      /* XBT_DEBUG("Store deleted object in mallocator %p for further use (size:%d/%d)",
         m, m->current_size, m->max_size); */
      m->objects[m->current_size++] = object;
      lock_release(m);
    } else {
      lock_release(m);
      /* otherwise we don't have a choice, we must free the object */
      /* XBT_DEBUG("Free deleted object: mallocator %p is full (size:%d/%d)", m, m->current_size, m->max_size); */
      m->free_f(object);
    }
  } else {
    if (xbt_mallocator_is_active()) {
      // We have to switch this mallocator from inactive to active (and then store that object)
      m->objects = xbt_new0(void *, m->max_size);
      lock_reset(m);
      xbt_mallocator_release(m,object);
    } else {
      m->free_f(object);
    }
  }
}
