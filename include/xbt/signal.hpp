/* Copyright (c) 2014-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_XBT_SIGNAL_HPP
#define SIMGRID_XBT_SIGNAL_HPP

#include <functional>
#include <map>
#include <utility>

namespace simgrid {
namespace xbt {

template <class S> class signal;

/** @brief
 * A signal/slot mechanism, where you can attach callbacks to a given signal, and then fire the signal.
 *
 * The template parameter is the function signature of the signal (the return value currently ignored).
 */
template <class R, class... P> class signal<R(P...)> {
  using callback_type = std::function<R(P...)>;
  std::map<unsigned int, callback_type> handlers_;
  unsigned int callback_sequence_id = 0;

public:
  /** Add a new callback to this signal */
  template <class U> unsigned int connect(U slot)
  {
    handlers_.insert({callback_sequence_id, std::move(slot)});
    return callback_sequence_id++;
  }
  /** Fire that signal, invoking all callbacks */
  R operator()(P... args) const
  {
    for (auto const& handler : handlers_)
      handler.second(args...);
  }
  /** Remove a callback */
  void disconnect(unsigned int id) { handlers_.erase(id); }
  /** Remove all callbacks */
  void disconnect_slots() { handlers_.clear(); }
  /** Get the amount of callbacks */
  int get_slot_count() { return handlers_.size(); }
};
}
}

#endif
