/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SLEEPSETSTATE_HPP
#define SIMGRID_MC_SLEEPSETSTATE_HPP

#include "src/mc/api/RemoteApp.hpp"
#include "src/mc/api/states/State.hpp"
#include "xbt/log.h"

namespace simgrid::mc {

class XBT_PRIVATE SleepSetState : public State {

  /* Sleep sets are composed of the actor and the corresponding transition that made it being added to the sleep
   * set. With this information, it is check whether it should be removed from it or not when exploring a new
   * transition */
  std::map<aid_t, std::shared_ptr<Transition>> sleep_set_;

public:
  explicit SleepSetState(RemoteApp& remote_app);
  explicit SleepSetState(RemoteApp& remote_app, std::shared_ptr<SleepSetState> parent_state);

  std::unordered_set<aid_t> get_sleeping_actors() const;
  std::vector<aid_t> get_enabled_minus_sleep() const;

  std::map<aid_t, std::shared_ptr<Transition>> const& get_sleep_set() const { return sleep_set_; }
  void add_sleep_set(std::shared_ptr<Transition> t) { sleep_set_.insert_or_assign(t->aid_, std::move(t)); }
  bool is_actor_sleeping(aid_t actor) const;

  friend class DPOR;
};

} // namespace simgrid::mc

#endif
