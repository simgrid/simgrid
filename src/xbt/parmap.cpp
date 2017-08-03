/* Copyright (c) 2004-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/parmap.h"
#include "xbt/log.h"
#include "xbt/parmap.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_parmap, xbt, "parmap: parallel map");

/**
 * \brief Parallel map structure
 */
typedef struct s_xbt_parmap {
  simgrid::xbt::Parmap<void*> p;
} s_xbt_parmap_t;

/**
 * \brief Creates a parallel map object
 * \param num_workers number of worker threads to create
 * \param mode how to synchronize the worker threads
 * \return the parmap created
 */
xbt_parmap_t xbt_parmap_new(unsigned int num_workers, e_xbt_parmap_mode_t mode)
{
  return reinterpret_cast<xbt_parmap_t>(new simgrid::xbt::Parmap<void*>(num_workers, mode));
}

/**
 * \brief Destroys a parmap
 * \param parmap the parmap to destroy
 */
void xbt_parmap_destroy(xbt_parmap_t parmap)
{
  delete parmap;
}

/**
 * \brief Applies a list of tasks in parallel.
 * \param parmap a parallel map object
 * \param fun the function to call in parallel
 * \param data each element of this dynar will be passed as an argument to fun
 */
void xbt_parmap_apply(xbt_parmap_t parmap, void_f_pvoid_t fun, xbt_dynar_t data)
{
  if (!xbt_dynar_is_empty(data)) {
    void** pdata = (void**)xbt_dynar_get_ptr(data, 0);
    std::vector<void*> vdata(pdata, pdata + xbt_dynar_length(data));
    parmap->p.apply(fun, vdata);
  }
}

/**
 * \brief Returns a next task to process.
 *
 * Worker threads call this function to get more work.
 *
 * \return the next task to process, or nullptr if there is no more work
 */
void* xbt_parmap_next(xbt_parmap_t parmap)
{
  try {
    return parmap->p.next();
  } catch (std::out_of_range) {
    return nullptr;
  }
}
