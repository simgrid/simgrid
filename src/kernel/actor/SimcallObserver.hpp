/* Copyright (c) 2019-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SIMCALL_OBSERVER_HPP
#define SIMGRID_MC_SIMCALL_OBSERVER_HPP

#include "simgrid/forward.h"
#include "src/mc/remote/Channel.hpp"
#include "src/mc/transition/Transition.hpp"
#include "xbt/asserts.h"
#include "xbt/ex.h"

#include <string>
#include <unordered_map>

namespace simgrid::kernel::actor {

class SimcallObserver {
  ActorImpl* const issuer_;

protected:
  ~SimcallObserver() = default;

public:
  explicit SimcallObserver(ActorImpl* issuer) : issuer_(issuer) {}
  ActorImpl* get_issuer() const { return issuer_; }
  /** Whether this transition can currently be taken without blocking.
   *
   * For example, a mutex_lock is not enabled when the mutex is not free.
   * A comm_receive is not enabled before the corresponding send has been issued.
   */
  virtual bool is_enabled() { return true; }

  /** Returns the amount of time that this transition can be used.
   *
   * If it's 0, the transition is not enabled.
   * If it's 1 (as with send/wait), there is no need to fork the state space exploration on this point.
   * If it's more than one (as with mc_random or waitany), we need to consider this transition several times to start
   * differing branches
   */
  virtual int get_max_consider() const { return 1; }

  /** Prepares the simcall to be used.
   *
   * For most simcalls, this does nothing. Once enabled, there is nothing to do to prepare a send().
   *
   * It is useful only for the simcalls that can be used several times, such as waitany() or random().
   * For them, prepare() selects the right outcome for the time being considered.
   *
   * The first time a simcall is considered, times_considered is 0, not 1.
   */
  virtual void prepare(int times_considered)
  { /* Nothing to do by default */
  }

  /** Serialize to the given string buffer, to send over the network */
  virtual void serialize(simgrid::mc::Channel& channel) const = 0;

  /** Used to debug (to display the simcall on which each actor is blocked when displaying it */
  virtual std::string to_string() const = 0;

  /** Whether the MC should see this simcall.
   * Simcall that don't have an observer (ie, most of them) are not visible from the MC, but if there is an observer,
   * they are observable by default. */
  virtual bool is_visible() const { return true; }
};

/** This is the ancestor class of all observers for simcalls that do not answer immediately but only through
 * simcall_blocking */
template <class T> class DelayedSimcallObserver : public SimcallObserver {
  T result_;

protected:
  ~DelayedSimcallObserver() = default;

public:
  DelayedSimcallObserver(ActorImpl* actor, T default_result) : SimcallObserver(actor), result_(default_result) {}
  void set_result(T res) { result_ = res; }
  T get_result() const { return result_; }
};
/* Specialization of the above template for the void return type, that is used for pure synchro simcalls */
template <> class DelayedSimcallObserver<void> : public SimcallObserver {
protected:
  ~DelayedSimcallObserver() = default;

public:
  explicit DelayedSimcallObserver(ActorImpl* actor) : SimcallObserver(actor) {}
  void get_result() const
  { /* Nothing to do */
  }
};

class RandomSimcall final : public SimcallObserver {
  const int min_;
  const int max_;
  int next_value_ = 0;

public:
  RandomSimcall(ActorImpl* actor, int min, int max) : SimcallObserver(actor), min_(min), max_(max)
  {
    xbt_assert(min <= max);
  }
  void serialize(mc::Channel& channel) const override;
  std::string to_string() const override;
  int get_max_consider() const override;
  void prepare(int times_considered) override;
  int get_value() const { return next_value_; }
};

class ActorJoinSimcall final : public DelayedSimcallObserver<void> {
  s4u::ActorPtr const other_; // We need a Ptr to ensure access to the actor after its end, but Ptr requires s4u
  const double timeout_;

public:
  ActorJoinSimcall(ActorImpl* actor, ActorImpl* other, double timeout = -1.0);
  void serialize(mc::Channel& channel) const override;
  std::string to_string() const override;
  bool is_enabled() override;

  s4u::ActorPtr get_other_actor() const { return other_; }
  double get_timeout() const { return timeout_; }
};
class ActorExitSimcall final : public SimcallObserver {
public:
  ActorExitSimcall(ActorImpl* actor);
  void serialize(mc::Channel& channel) const override;
  std::string to_string() const override;
};

/* Dummy observer, not visible by the checker but needed for the template to compile */
class ActorSuspendSimcall final : public DelayedSimcallObserver<void> {
public:
  explicit ActorSuspendSimcall(ActorImpl* actor) : DelayedSimcallObserver<void>(actor) {}
  void serialize(mc::Channel& channel) const override { THROW_UNIMPLEMENTED; }
  std::string to_string() const override { THROW_UNIMPLEMENTED; }
  bool is_visible() const override { return false; }
};

class ActorSleepSimcall final : public DelayedSimcallObserver<void> {

public:
  explicit ActorSleepSimcall(ActorImpl* actor) : DelayedSimcallObserver<void>(actor) {}
  void serialize(mc::Channel& channel) const override;
  std::string to_string() const override;
};

class ActorCreateSimcall final : public SimcallObserver {
  aid_t child_ = -1;

public:
  explicit ActorCreateSimcall(ActorImpl* actor) : SimcallObserver(actor) {}
  void serialize(mc::Channel& channel) const override;
  std::string to_string() const override;

  void set_child(aid_t child) { child_ = child; }
};

class ObjectAccessSimcallObserver final : public SimcallObserver {
  ObjectAccessSimcallItem* const object_;

public:
  ObjectAccessSimcallObserver(ActorImpl* actor, ObjectAccessSimcallItem* object)
      : SimcallObserver(actor), object_(object)
  {
  }
  void serialize(mc::Channel& channel) const override;
  std::string to_string() const override;
  bool is_visible() const override;
  bool is_enabled() override { return true; }

  ActorImpl* get_owner() const;
};

/* Semi private template used by the to_string methods of various observer classes */
template <typename A> static std::string ptr_to_id(A* ptr)
{
  static std::unordered_map<A*, std::string> map({{nullptr, "-"}});
  auto [elm, inserted] = map.try_emplace(ptr);
  if (inserted)
    elm->second = std::to_string(map.size() - 1);
  return elm->second;
}

} // namespace simgrid::kernel::actor

#endif
