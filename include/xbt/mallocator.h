/* xbt/mallocator.h -- api to recycle allocated objects                     */

/* Copyright (c) 2006-2007, 2009-2010, 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_MALLOCATOR_H
#define _XBT_MALLOCATOR_H

#include "xbt/function_types.h"
#include "xbt/misc.h"
SG_BEGIN_DECL()

/** @addtogroup XBT_mallocator
 *  @brief The mallocator system
 * 
 *  This section describes the API to a mallocator.
 *  A mallocator allows you to recycle the objects you don't need anymore  instead of freeing them. A mallocator is a
 *  stack which stores the unused objects  or a given type. If you often need to malloc() / free() objects of a certain
 *  type, you should use a mallocator and call \a xbt_mallocator_get() and \a xbt_mallocator_release() instead of
 *  malloc() and free().
 *
 *  When you release an object, it is not freed but it is stored into the mallocator (unless the mallocator is full).
 *  And when you want to get a new object, the object is just extracted from the mallocator. No malloc() is  done,
 *  unless there is no more object in the mallocator.
 */
/** @defgroup XBT_mallocator_cons Mallocator constructor and destructor
 *  @ingroup XBT_mallocator
 *
 *  @{
 */
/** \brief Mallocator data type (opaque structure) */
typedef struct s_xbt_mallocator *xbt_mallocator_t;
XBT_PUBLIC(xbt_mallocator_t) xbt_mallocator_new(int size, pvoid_f_void_t new_f, void_f_pvoid_t free_f,
                                                void_f_pvoid_t reset_f);
XBT_PUBLIC(void) xbt_mallocator_free(xbt_mallocator_t mallocator);
/** @} */

/* object handling */
/** @defgroup XBT_mallocator_objects Mallocator object handling
 *  @ingroup XBT_mallocator
 *
 *  @{
 */
XBT_PUBLIC(void *) xbt_mallocator_get(xbt_mallocator_t mallocator);
XBT_PUBLIC(void) xbt_mallocator_release(xbt_mallocator_t mallocator, void *object);

XBT_PUBLIC(void) xbt_mallocator_initialization_is_done(int protect);
/** @} */

SG_END_DECL()
#endif                          /* _XBT_MALLOCATOR_H */
