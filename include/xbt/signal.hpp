/* Copyright (c) 2014-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_XBT_SIGNAL_HPP
#define SIMGRID_XBT_SIGNAL_HPP

#include <functional>
#include <map>
#include <utility>

namespace simgrid {
namespace xbt {

  template<class S> class signal;

  /** A signal/slot mechanism
  *
  *  S is expected to be the function signature of the signal.
  *  I'm not sure we need a return value (it is currently ignored).
  *  If we don't we might use `signal<P1, P2, ...>` instead.
  */
  template<class R, class... P>
  class signal<R(P...)> {
    using callback_type = std::function<R(P...)>;
    std::map<unsigned int, callback_type> handlers_;
    unsigned int callback_sequence_id = 0;

  public:
    template <class U> unsigned int connect(U slot)
    {
      handlers_.insert({callback_sequence_id, std::move(slot)});
      return callback_sequence_id++;
    }
    R operator()(P... args) const
    {
      for (auto const& handler : handlers_)
        handler.second(args...);
    }
    void disconnect(unsigned int id) { handlers_.erase(id); }
    void disconnect_slots() { handlers_.clear(); }
    int get_slot_count() { return handlers_.size(); }
  };

}
}

#endif
