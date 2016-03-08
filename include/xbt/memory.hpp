/* Copyright (c) 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRIX_XBT_MEMORY_HPP
#define SIMGRIX_XBT_MEMORY_HPP

#include <memory>

namespace simgrid {
namespace xbt {

/** Delete operator which call a `destroy()` free function */
template<class T>
struct destroy_delete {
  void operator()(T* t) const
  {
    destroy(t);
  }
};

/** A `unique_ptr` which works for SimGrid C types (dynar, swag, automaton, etc.)
 *
 *  It uses an overloaded `destroy()` function to delete the object.
 */
template<class T>
using unique_ptr = std::unique_ptr<T, destroy_delete<T> >;

}
}

#endif
