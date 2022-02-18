/* Copyright (c) 2019-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SIMCALL_OBSERVER_HPP
#define SIMGRID_MC_SIMCALL_OBSERVER_HPP

#include "simgrid/forward.h"
#include "src/mc/transition/Transition.hpp"
#include "xbt/asserts.h"

#include <string>

namespace simgrid {
namespace kernel {
namespace actor {

class SimcallObserver {
  ActorImpl* const issuer_;

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
   * If it's 1 (as with send/wait), there is no need to fork the state space exploration on this point.
   * If it's more than one (as with mc_random or waitany), we need to consider this transition several times to start
   * differing branches
   */
  virtual int get_max_consider() { return 1; }

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

  /** Computes the dependency relation */
  virtual bool depends(SimcallObserver* other);

  /** Serialize to the given string buffer */
  virtual void serialize(std::stringstream& stream) const;

  /** Some simcalls may only be observable under some conditions.
   * Most simcalls are not visible from the MC because they don't have an observer at all. */
  virtual bool is_visible() const { return true; }
};

template <class T> class ResultingSimcall : public SimcallObserver {
  T result_;

public:
  ResultingSimcall() = default;
  ResultingSimcall(ActorImpl* actor, T default_result) : SimcallObserver(actor), result_(default_result) {}
  void set_result(T res) { result_ = res; }
  T get_result() const { return result_; }
};

class RandomSimcall : public SimcallObserver {
  const int min_;
  const int max_;
  int next_value_ = 0;

public:
  RandomSimcall(ActorImpl* actor, int min, int max) : SimcallObserver(actor), min_(min), max_(max)
  {
    xbt_assert(min < max);
  }
  void serialize(std::stringstream& stream) const override;
  int get_max_consider() override;
  void prepare(int times_considered) override;
  int get_value() const { return next_value_; }
  bool depends(SimcallObserver* other) override;
};

class MutexSimcall : public SimcallObserver {
  activity::MutexImpl* const mutex_;

public:
  MutexSimcall(ActorImpl* actor, activity::MutexImpl* mutex) : SimcallObserver(actor), mutex_(mutex) {}
  activity::MutexImpl* get_mutex() const { return mutex_; }
  bool depends(SimcallObserver* other) override;
};

class MutexUnlockSimcall : public MutexSimcall {
  using MutexSimcall::MutexSimcall;
};

class MutexLockSimcall : public MutexSimcall {
  const bool blocking_;

public:
  MutexLockSimcall(ActorImpl* actor, activity::MutexImpl* mutex, bool blocking = true)
      : MutexSimcall(actor, mutex), blocking_(blocking)
  {
  }
  bool is_enabled() override;
};

class ConditionWaitSimcall : public ResultingSimcall<bool> {
  activity::ConditionVariableImpl* const cond_;
  activity::MutexImpl* const mutex_;
  const double timeout_;

public:
  ConditionWaitSimcall(ActorImpl* actor, activity::ConditionVariableImpl* cond, activity::MutexImpl* mutex,
                       double timeout = -1.0)
      : ResultingSimcall(actor, false), cond_(cond), mutex_(mutex), timeout_(timeout)
  {
  }
  bool is_enabled() override;
  bool is_visible() const override { return false; }
  activity::ConditionVariableImpl* get_cond() const { return cond_; }
  activity::MutexImpl* get_mutex() const { return mutex_; }
  double get_timeout() const { return timeout_; }
};

class SemAcquireSimcall : public ResultingSimcall<bool> {
  activity::SemaphoreImpl* const sem_;
  const double timeout_;

public:
  SemAcquireSimcall(ActorImpl* actor, activity::SemaphoreImpl* sem, double timeout = -1.0)
      : ResultingSimcall(actor, false), sem_(sem), timeout_(timeout)
  {
  }
  bool is_enabled() override;
  bool is_visible() const override { return false; }
  activity::SemaphoreImpl* get_sem() const { return sem_; }
  double get_timeout() const { return timeout_; }
};

class ActivityTestSimcall : public ResultingSimcall<bool> {
  activity::ActivityImpl* const activity_;

public:
  ActivityTestSimcall(ActorImpl* actor, activity::ActivityImpl* activity)
      : ResultingSimcall(actor, true), activity_(activity)
  {
  }
  bool is_visible() const override { return true; }
  activity::ActivityImpl* get_activity() const { return activity_; }
  void serialize(std::stringstream& stream) const override;
};

class ActivityTestanySimcall : public ResultingSimcall<ssize_t> {
  const std::vector<activity::ActivityImpl*>& activities_;
  std::vector<int> indexes_; // indexes in activities_ pointing to ready activities (=whose test() is positive)
  int next_value_ = 0;

public:
  ActivityTestanySimcall(ActorImpl* actor, const std::vector<activity::ActivityImpl*>& activities);
  bool is_visible() const override { return true; }
  bool is_enabled() override { return true; /* can return -1 if no activity is ready */ }
  void serialize(std::stringstream& stream) const override;
  int get_max_consider() override;
  void prepare(int times_considered) override;
  const std::vector<activity::ActivityImpl*>& get_activities() const { return activities_; }
  int get_value() const { return next_value_; }
};

class ActivityWaitSimcall : public ResultingSimcall<bool> {
  activity::ActivityImpl* activity_;
  const double timeout_;

public:
  ActivityWaitSimcall(ActorImpl* actor, activity::ActivityImpl* activity, double timeout)
      : ResultingSimcall(actor, false), activity_(activity), timeout_(timeout)
  {
  }
  void serialize(std::stringstream& stream) const override;
  bool is_visible() const override { return true; }
  bool is_enabled() override;
  activity::ActivityImpl* get_activity() const { return activity_; }
  void set_activity(activity::ActivityImpl* activity) { activity_ = activity; }
  double get_timeout() const { return timeout_; }
};

class ActivityWaitanySimcall : public ResultingSimcall<ssize_t> {
  const std::vector<activity::ActivityImpl*>& activities_;
  std::vector<int> indexes_; // indexes in activities_ pointing to ready activities (=whose test() is positive)
  const double timeout_;
  int next_value_ = 0;

public:
  ActivityWaitanySimcall(ActorImpl* actor, const std::vector<activity::ActivityImpl*>& activities, double timeout);
  bool is_enabled() override;
  void serialize(std::stringstream& stream) const override;
  bool is_visible() const override { return true; }
  void prepare(int times_considered) override;
  int get_max_consider() override;
  const std::vector<activity::ActivityImpl*>& get_activities() const { return activities_; }
  double get_timeout() const { return timeout_; }
  int get_value() const { return next_value_; }
};

class CommIsendSimcall : public SimcallObserver {
  activity::MailboxImpl* mbox_;
  double payload_size_;
  double rate_;
  unsigned char* src_buff_;
  size_t src_buff_size_;
  void* payload_;
  bool detached_;
  activity::CommImpl* comm_;
  int tag_;

  bool (*match_fun_)(void*, void*, activity::CommImpl*);
  void (*clean_fun_)(void*); // used to free the synchro in case of problem after a detached send
  void (*copy_data_fun_)(activity::CommImpl*, void*, size_t); // used to copy data if not default one

public:
  CommIsendSimcall(ActorImpl* actor, activity::MailboxImpl* mbox, double payload_size, double rate,
                   unsigned char* src_buff, size_t src_buff_size, bool (*match_fun)(void*, void*, activity::CommImpl*),
                   void (*clean_fun)(void*), // used to free the synchro in case of problem after a detached send
                   void (*copy_data_fun)(activity::CommImpl*, void*, size_t), // used to copy data if not default one
                   void* payload, bool detached)
      : SimcallObserver(actor)
      , mbox_(mbox)
      , payload_size_(payload_size)
      , rate_(rate)
      , src_buff_(src_buff)
      , src_buff_size_(src_buff_size)
      , payload_(payload)
      , detached_(detached)
      , match_fun_(match_fun)
      , clean_fun_(clean_fun)
      , copy_data_fun_(copy_data_fun)
  {
  }
  void serialize(std::stringstream& stream) const override;
  bool is_visible() const override { return true; }
  activity::MailboxImpl* get_mailbox() const { return mbox_; }
  double get_payload_size() const { return payload_size_; }
  double get_rate() const { return rate_; }
  unsigned char* get_src_buff() const { return src_buff_; }
  size_t get_src_buff_size() const { return src_buff_size_; }
  void* get_payload() const { return payload_; }
  bool is_detached() const { return detached_; }
  void set_comm(activity::CommImpl* comm) { comm_ = comm; }
  void set_tag(int tag) { tag_ = tag; }

  auto get_match_fun() const { return match_fun_; }
  auto get_clean_fun() const { return clean_fun_; }
  auto get_copy_data_fun() const { return copy_data_fun_; }
};

class CommIrecvSimcall : public SimcallObserver {
  activity::MailboxImpl* mbox_;
  unsigned char* dst_buff_;
  size_t* dst_buff_size_;
  void* payload_;
  double rate_;
  int tag_;
  activity::CommImpl* comm_;

  bool (*match_fun_)(void*, void*, activity::CommImpl*);
  void (*copy_data_fun_)(activity::CommImpl*, void*, size_t); // used to copy data if not default one

public:
  CommIrecvSimcall(ActorImpl* actor, activity::MailboxImpl* mbox, unsigned char* dst_buff, size_t* dst_buff_size,
                   bool (*match_fun)(void*, void*, activity::CommImpl*),
                   void (*copy_data_fun)(activity::CommImpl*, void*, size_t), void* payload, double rate)
      : SimcallObserver(actor)
      , mbox_(mbox)
      , dst_buff_(dst_buff)
      , dst_buff_size_(dst_buff_size)
      , payload_(payload)
      , rate_(rate)
      , match_fun_(match_fun)
      , copy_data_fun_(copy_data_fun)
  {
  }
  void serialize(std::stringstream& stream) const override;
  bool is_visible() const override { return true; }
  activity::MailboxImpl* get_mailbox() const { return mbox_; }
  double get_rate() const { return rate_; }
  unsigned char* get_dst_buff() const { return dst_buff_; }
  size_t* get_dst_buff_size() const { return dst_buff_size_; }
  void* get_payload() const { return payload_; }
  void set_comm(activity::CommImpl* comm) { comm_ = comm; }
  void set_tag(int tag) { tag_ = tag; }

  auto get_match_fun() const { return match_fun_; };
  auto get_copy_data_fun() const { return copy_data_fun_; }
};

} // namespace actor
} // namespace kernel
} // namespace simgrid

#endif
