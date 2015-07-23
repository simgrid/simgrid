/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved. */

#ifndef SIMGRID_UTIL_HTPP
#define SIMGRID_UTIL_HTPP

#include <algorithm>

namespace simgrid {
namespace util {

/** Find a pointer to a value stores in a map (or nullptr) */
template<typename C, typename K>
inline
typename C::mapped_type* find_map_ptr(C& c, K const& k)
{
  typename C::iterator i = c.find(k);
  if (i == c.end())
    return nullptr;
  else
    return &i->second;
}

template<typename C, typename K>
inline
typename C::mapped_type const* find_map_ptr(C const& c, K const& k)
{
  typename C::const_iterator i = c.find(k);
  if (i == c.end())
    return nullptr;
  else
    return &i->second;
}

}
}

#endif
