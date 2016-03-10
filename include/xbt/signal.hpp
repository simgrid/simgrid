/* Copyright (c) 2014-2015. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_XBT_SIGNAL_HPP
#define SIMGRID_XBT_SIGNAL_HPP

#include "simgrid_config.h"
#if SIMGRID_HAVE_LIBSIG
#include <sigc++/sigc++.h>
#else
#include <boost/signals2.hpp>
#endif

namespace simgrid {
namespace xbt {

#if SIMGRID_HAVE_LIBSIG

  // Wraps sigc++ signals with the interface of boost::signals2:
  template<class T> class signal;
  template<class R, class... P>
  class signal<R(P...)> {
  private:
    sigc::signal<R, P...> sig_;
  public:
    template<class U> XBT_ALWAYS_INLINE
    void connect(U&& slot)
    {
      sig_.connect(std::forward<U>(slot));
    }
    template<class Res, class... Args> XBT_ALWAYS_INLINE
    void connect(Res(*slot)(Args...))
    {
      sig_.connect(sigc::ptr_fun(slot));
    }
    template<class... Args> XBT_ALWAYS_INLINE
    R operator()(Args&&... args) const
    {
      return sig_.emit(std::forward<Args>(args)...);
    }
    void disconnect_all_slots()
    {
      sig_.clear();
    }
  };

#else

  template<class T>
  using signal = ::boost::signals2::signal<T>;

#endif

}
}

#endif
