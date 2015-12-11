/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_XBT_LIB_HPP
#define SIMGRID_XBT_LIB_HPP

#include <cstddef>

#include <vector>

namespace simgrid {
namespace xbt {

template<class T>
class Lib {
private:
  static std::vector<void(*)(void*)> deleters_;
protected:
  std::vector<void*> facets_;
public:
  static std::size_t add_level(void (*deleter)(void*))
  {
    std::size_t res = deleters_.size();
    deleters_.push_back(deleter);
    return res;
  }
  template<class U> static
  std::size_t add_level()
  {
    return add_level([](void* p){ delete (U*)p; });
  }
  Lib() : facets_(deleters_.size(), nullptr) {}
  ~Lib()
  {
    for (std::size_t i = 0; i < facets_.size(); ++i)
      if (facets_[i] != nullptr)
        deleters_[i](facets_[i]);
  }

  // TODO, make type-safe versions of this
  void* facet(std::size_t level)
  {
    if (level >= facets_.size())
      return nullptr;
    else
      return facets_.at(level);
  }
  void set_facet(std::size_t level, void* value, bool use_dtor = true)
  {
    if (level >= facets_.size())
      facets_.resize(level + 1, nullptr);
    void* old_value = this->facet(level);
    facets_.at(level) = value;
    if (use_dtor && old_value != nullptr && deleters_[level])
      deleters_[level](old_value);
  }
};

template<class T>
std::vector<void(*)(void*)> Lib<T>::deleters_ = {};

}
}

#endif
