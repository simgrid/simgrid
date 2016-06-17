/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_XBT_LIB_HPP
#define SIMGRID_XBT_LIB_HPP

#include <cstddef>
#include <limits>
#include <vector>

namespace simgrid {
namespace xbt {

template<class T, class U> class Extension;
template<class T>          class Extendable;

template<class T, class U>
class Extension {
  static const std::size_t INVALID_ID = std::numeric_limits<std::size_t>::max();
  std::size_t id_;
  friend class Extendable<T>;
  constexpr Extension(std::size_t id) : id_(id) {}
public:
  constexpr Extension() : id_(INVALID_ID) {}
  std::size_t id() const { return id_; }
  bool valid() { return id_ != INVALID_ID; }
};

/** An Extendable is an object that you can extend with external elements.
 *
 * An Extension is one dimension of such extension. They are similar to the concept of mixins, that is, a set of behavior that is injected into a class without derivation.
 *
 * Imagine that you want to write a plugin dealing with the energy in SimGrid.
 * You will have to store some information about each and every host.
 *
 * You could modify the Host class directly (but your code will soon become messy).
 * You could create a class EnergyHost deriving Host, but it is not easily combinable
 *    with a notion of Host extended with another concept (such as mobility).
 * You could completely externalize these data with an associative map Host->EnergyHost.
 *    It would work, provided that you implement this classical feature correctly (and it would induce a little performance penalty).
 * Instead, you should add a new extension to the Host class, that happens to be Extendable.
 *
 */
template<class T>
class Extendable {
private:
  static std::vector<void(*)(void*)> deleters_;
protected:
  std::vector<void*> extensions_;
public:
  static size_t extension_create(void (*deleter)(void*))
  {
    std::size_t res = deleters_.size();
    deleters_.push_back(deleter);
    return res;
  }
  template<class U>
  static Extension<T,U> extension_create(void (*deleter)(void*))
  {
    return Extension<T,U>(extension_create(deleter));
  }
  template<class U> static
  Extension<T,U> extension_create()
  {
    return extension_create([](void* p){ delete static_cast<U*>(p); });
  }
  Extendable() : extensions_(deleters_.size(), nullptr) {}
  ~Extendable()
  {
    /* Call destructors in reverse order of their registrations
     *
     * The rationale for this, is that if an extension B as been added after
     * an extension A, the subsystem of B might depend on the subsystem on A and
     * an extension of B might need to have the extension of A around when executing
     * its cleanup function/destructor. */
    for (std::size_t i = extensions_.size(); i > 0; --i)
      if (extensions_[i - 1] != nullptr)
        deleters_[i - 1](extensions_[i - 1]);
  }

  // Type-unsafe versions of the facet access methods:
  void* extension(std::size_t rank)
  {
    if (rank >= extensions_.size())
      return nullptr;
    else
      return extensions_.at(rank);
  }
  void extension_set(std::size_t rank, void* value, bool use_dtor = true)
  {
    if (rank >= extensions_.size())
      extensions_.resize(rank + 1, nullptr);
    void* old_value = this->extension(rank);
    extensions_.at(rank) = value;
    if (use_dtor && old_value != nullptr && deleters_[rank])
      deleters_[rank](old_value);
  }

  // Type safe versions of the facet access methods:
  template<class U>
  U* extension(Extension<T,U> rank)
  {
    return static_cast<U*>(extension(rank.id()));
  }
  template<class U>
  void extension_set(Extension<T,U> rank, U* value, bool use_dtor = true)
  {
    extension_set(rank.id(), value, use_dtor);
  }

public:
  // Convenience extension access when the type has a associated EXTENSION ID:
  template<class U> U* extension()           { return extension<U>(U::EXTENSION_ID); }
  template<class U> void extension_set(U* p) { extension_set<U>(U::EXTENSION_ID, p); }
};

template<class T>
std::vector<void(*)(void*)> Extendable<T>::deleters_ = {};

}
}

#endif
