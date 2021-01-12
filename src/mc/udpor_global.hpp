/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UDPOR_GLOBAL_HPP
#define SIMGRID_MC_UDPOR_GLOBAL_HPP

#include <iostream>
#include <queue>

namespace simgrid {
namespace mc {

class UnfoldingEvent;
using EventSet = std::deque<UnfoldingEvent*>;

typedef struct s_evset_in {
  EventSet causuality_events;
  EventSet cause;
  EventSet ancestorSet;
} s_evset_in_t;

class Configuration {
public:
  EventSet events_;
  EventSet maxEvent;         // Events recently added to events_
  EventSet actorMaxEvent;    // maximal events of the actors in current configuration
  UnfoldingEvent* lastEvent; // The last added event

  Configuration plus_config(UnfoldingEvent*) const;
  void createEvts(Configuration C, EventSet& result, const std::string& trans_tag, s_evset_in_t ev_sets, bool chk,
                  UnfoldingEvent* immPreEvt);
  void updateMaxEvent(UnfoldingEvent*);         // update maximal events of the configuration and actors
  UnfoldingEvent* findActorMaxEvt(int actorId); // find maximal event of a Actor whose id = actorId

  UnfoldingEvent* findTestedComm(const UnfoldingEvent* testEvt); // find comm tested by action testTrans

  Configuration()                     = default;
  Configuration(const Configuration&) = default;
  Configuration& operator=(Configuration const&) = default;
  Configuration(Configuration&&) noexcept        = default;
  ~Configuration()                               = default;
};

} // namespace mc
} // namespace simgrid
#endif