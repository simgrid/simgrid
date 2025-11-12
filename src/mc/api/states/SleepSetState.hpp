/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SLEEPSETSTATE_HPP
#define SIMGRID_MC_SLEEPSETSTATE_HPP

#include "src/mc/api/RemoteApp.hpp"
#include "src/mc/api/states/State.hpp"
#include <string>

namespace simgrid::mc {

class XBT_PRIVATE SleepSetState : public State {
protected:
  /* Sleep sets are composed of the actor and the corresponding transition that made it being added to the sleep
   * set. With this information, it is check whether it should be removed from it or not when exploring a new
   * transition */
  std::map<aid_t, std::shared_ptr<Transition>> sleep_set_;

public:
  explicit SleepSetState(RemoteApp& remote_app);
  explicit SleepSetState(RemoteApp& remote_app, StatePtr parent_state, std::shared_ptr<Transition> incoming_transition,
                         bool set_actor_status = true);

  void add_arbitrary_transition(RemoteApp& remote_app);
  virtual std::unordered_set<aid_t> get_sleeping_actors(aid_t) const;
  std::vector<aid_t> get_enabled_minus_sleep() const;

  std::map<aid_t, std::shared_ptr<Transition>> const& get_sleep_set() const { return sleep_set_; }
  void add_sleep_set(std::shared_ptr<Transition> t) { sleep_set_.insert_or_assign(t->aid_, std::move(t)); }
  void sleep_add_and_mark(std::shared_ptr<Transition> t);
  bool is_actor_sleeping(aid_t actor) const;

  std::string string_of_sleep_set() const
  {
    if (sleep_set_.size() == 0)
      return "[]";
    else
      return std::to_string('[') +
             std::accumulate(std::next(sleep_set_.begin()), sleep_set_.end(),
                             '<' + std::to_string(sleep_set_.begin()->first) + ',' +
                                 sleep_set_.begin()->second->to_string(),
                             [](std::string a, auto b) {
                               return std::move(a) + ';' + '<' + std::to_string(b.first) + ',' + b.second->to_string();
                             }) +
             std::to_string(']');
  }
};

} // namespace simgrid::mc

#endif
