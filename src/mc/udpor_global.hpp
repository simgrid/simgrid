/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UDPOR_GLOBAL_HPP
#define SIMGRID_MC_UDPOR_GLOBAL_HPP

#include "src/mc/api/State.hpp"

#include <iostream>
#include <queue>
#include <set>
#include <string_view>

/* TODO: many method declared in this module are not implemented */

namespace simgrid::mc {

class UnfoldingEvent;
class Configuration;

class EventSet {
private:
  std::set<UnfoldingEvent*> events_;

public:
  EventSet()                           = default;
  EventSet(const EventSet&)            = default;
  EventSet& operator=(const EventSet&) = default;
  EventSet(EventSet&&)                 = default;
  EventSet(const Configuration&);

  void remove(UnfoldingEvent* e);
  void subtract(const EventSet& other);
  void subtract(const Configuration& other);
  EventSet subtracting(const EventSet& e) const;
  EventSet subtracting(const UnfoldingEvent* e) const;
  EventSet subtracting(const Configuration* e) const;

  void insert(UnfoldingEvent* e);
  void form_union(const EventSet&);
  void form_union(const Configuration&);
  EventSet make_union(const UnfoldingEvent* e) const;
  EventSet make_union(const EventSet&) const;
  EventSet make_union(const Configuration& e) const;

  bool empty() const;
  bool contains(const UnfoldingEvent* e) const;
  bool is_subset_of(const EventSet& other) const;

  // TODO: What is this used for?
  UnfoldingEvent* find(const UnfoldingEvent* e) const;

  // TODO: What is this used for
  bool depends(const EventSet& other) const;

  // TODO: What is this used for
  bool is_empty_intersection(EventSet evtS) const;
};

struct s_evset_in_t {
  EventSet causuality_events;
  EventSet cause;
  EventSet ancestorSet;
};

class Configuration {
public:
  Configuration()                                = default;
  Configuration(const Configuration&)            = default;
  Configuration& operator=(Configuration const&) = default;
  Configuration(Configuration&&)                 = default;

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
};

class UnfoldingEvent {
public:
  UnfoldingEvent(unsigned int nb_events, std::string const& trans_tag, EventSet const& causes, int sid = -1);
  UnfoldingEvent(const UnfoldingEvent&)            = default;
  UnfoldingEvent& operator=(UnfoldingEvent const&) = default;
  UnfoldingEvent(UnfoldingEvent&&)                 = default;

  EventSet get_history() const;

  bool isConflict(UnfoldingEvent* event, UnfoldingEvent* otherEvent) const;
  bool concernSameComm(const UnfoldingEvent* event, const UnfoldingEvent* otherEvent) const;

  // check otherEvent is in my history ?
  bool inHistory(UnfoldingEvent* otherEvent) const;

  bool isImmediateConflict1(UnfoldingEvent* evt, UnfoldingEvent* otherEvt) const;

  bool conflictWithConfig(UnfoldingEvent* event, Configuration const& config) const;
  bool operator==(const UnfoldingEvent&) const { return false; };
  void print() const;

  inline int get_state_id() const { return state_id; }
  inline void set_state_id(int sid) { state_id = sid; }

  inline std::string get_transition_tag() const { return transition_tag; }
  inline void set_transition_tag(std::string_view tr_tag) { transition_tag = tr_tag; }

private:
  EventSet causes; // used to store directed ancestors of event e
  int id = -1;
  int state_id{-1};
  std::string transition_tag{""}; // The tag of the last transition that lead to creating the event
  bool transition_is_IReceive(const UnfoldingEvent* testedEvt, const UnfoldingEvent* SdRcEvt) const;
  bool transition_is_ISend(const UnfoldingEvent* testedEvt, const UnfoldingEvent* SdRcEvt) const;
  bool check_tr_concern_same_comm(bool& chk1, bool& chk2, UnfoldingEvent* evt1, UnfoldingEvent* evt2) const;
};

class StateManager {
private:
  using Handle = uint64_t;
  std::map<Handle, std::unique_ptr<State>> state_map_;

public:
  Handle record_state(std::unique_ptr<State>&&);
};

} // namespace simgrid::mc
#endif
