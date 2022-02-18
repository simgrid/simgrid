/* Copyright (c) 2019-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SIMCALL_COMM_OBSERVER_HPP
#define SIMGRID_MC_SIMCALL_COMM_OBSERVER_HPP

#include "simgrid/forward.h"
#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/mc/transition/Transition.hpp"
#include "xbt/asserts.h"

#include <string>

namespace simgrid {
namespace kernel {
namespace actor {

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
