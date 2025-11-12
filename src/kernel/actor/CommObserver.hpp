/* Copyright (c) 2019-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SIMCALL_COMM_OBSERVER_HPP
#define SIMGRID_MC_SIMCALL_COMM_OBSERVER_HPP

#include "simgrid/forward.h"
#include "simgrid/s4u/Mailbox.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/mc/transition/Transition.hpp"
#include "xbt/asserts.h"

#include <string>
#include <string_view>

namespace simgrid::kernel::actor {

// This is a DelayedSimcallObserver even if its name denotes an async_comm, because in non-MC mode, the recv is not
// split in irecv+wait but executed in one simcall only, with such an observer that then needs to be delayed
class CommIsendSimcall final : public DelayedSimcallObserver<void> {
  activity::MailboxImpl* mbox_;
  double payload_size_;
  double rate_;
  unsigned char* src_buff_;
  size_t src_buff_size_;
  void* match_data_;
  bool detached_;
  activity::CommImpl* comm_ = {};
  int tag_                  = {};

  std::function<bool(void*, void*, activity::CommImpl*)> match_fun_;
  std::function<void(void*)> clean_fun_; // used to free the synchro in case of problem after a detached send
  std::function<void(activity::CommImpl*, void*, size_t)> copy_data_fun_; // used to copy data if not default one

  std::string fun_call_;

public:
  CommIsendSimcall(
      ActorImpl* actor, activity::MailboxImpl* mbox, double payload_size, double rate, unsigned char* src_buff,
      size_t src_buff_size, const std::function<bool(void*, void*, activity::CommImpl*)>& match_fun,
      const std::function<void(void*)>& clean_fun, // used to free the synchro in case of problem after a detached send
      const std::function<void(activity::CommImpl*, void*, size_t)>&
          copy_data_fun, // used to copy data if not default one
      void* match_data, bool detached, std::string_view fun_call)
      : DelayedSimcallObserver<void>(actor)
      , mbox_(mbox)
      , payload_size_(payload_size)
      , rate_(rate)
      , src_buff_(src_buff)
      , src_buff_size_(src_buff_size)
      , match_data_(match_data)
      , detached_(detached)
      , match_fun_(match_fun)
      , clean_fun_(clean_fun)
      , copy_data_fun_(copy_data_fun)
      , fun_call_(fun_call)
  {
  }
  void serialize(mc::Channel& channel) const override;
  std::string to_string() const override;
  activity::MailboxImpl* get_mailbox() const { return mbox_; }
  double get_payload_size() const { return payload_size_; }
  double get_rate() const { return rate_; }
  unsigned char* get_src_buff() const { return src_buff_; }
  size_t get_src_buff_size() const { return src_buff_size_; }
  void* get_match_data() const { return match_data_; }
  bool is_detached() const { return detached_; }
  void set_comm(activity::CommImpl* comm) { comm_ = comm; }
  void set_tag(int tag) { tag_ = tag; }

  auto const& get_match_fun() const { return match_fun_; }
  auto const& get_clean_fun() const { return clean_fun_; }
  auto const& get_copy_data_fun() const { return copy_data_fun_; }
};

// This is a DelayedSimcallObserver even if its name denotes an async_comm, because in non-MC mode, the send is not
// split in isend+wait but executed in one simcall only, with such an observer that then needs to be delayed
class CommIrecvSimcall final : public DelayedSimcallObserver<void> {
  activity::MailboxImpl* mbox_;
  unsigned char* dst_buff_;
  size_t* dst_buff_size_;
  void* match_data_;
  double rate_;
  activity::CommImpl* comm_ = {};
  int tag_                  = {};

  std::function<bool(void*, void*, activity::CommImpl*)> match_fun_;
  std::function<void(activity::CommImpl*, void*, size_t)> copy_data_fun_; // used to copy data if not default one

  std::string fun_call_;

public:
  CommIrecvSimcall(ActorImpl* actor, activity::MailboxImpl* mbox, unsigned char* dst_buff, size_t* dst_buff_size,
                   const std::function<bool(void*, void*, activity::CommImpl*)>& match_fun,
                   const std::function<void(activity::CommImpl*, void*, size_t)>& copy_data_fun, void* match_data,
                   double rate, std::string_view fun_call)
      : DelayedSimcallObserver<void>(actor)
      , mbox_(mbox)
      , dst_buff_(dst_buff)
      , dst_buff_size_(dst_buff_size)
      , match_data_(match_data)
      , rate_(rate)
      , match_fun_(match_fun)
      , copy_data_fun_(copy_data_fun)
      , fun_call_(fun_call)
  {
  }
  void serialize(mc::Channel& channel) const override;
  std::string to_string() const override;
  activity::MailboxImpl* get_mailbox() const { return mbox_; }
  double get_rate() const { return rate_; }
  unsigned char* get_dst_buff() const { return dst_buff_; }
  size_t* get_dst_buff_size() const { return dst_buff_size_; }
  void* get_match_data() const { return match_data_; }
  void set_comm(activity::CommImpl* comm) { comm_ = comm; }
  void set_tag(int tag) { tag_ = tag; }

  auto const& get_match_fun() const { return match_fun_; };
  auto const& get_copy_data_fun() const { return copy_data_fun_; }
};

class IprobeSimcall final : public SimcallObserver {
  activity::MailboxImpl* mbox_;
  s4u::Mailbox::IprobeKind kind_;
  int tag_ = {};
  std::function<bool(void*, void*, activity::CommImpl*)> match_fun_;
  void* match_data_; // Actually, that's the smpi request

public:
  IprobeSimcall(ActorImpl* actor, activity::MailboxImpl* mbox, s4u::Mailbox::IprobeKind kind,
                const std::function<bool(void*, void*, activity::CommImpl*)>& match_fun, void* match_data);

  void serialize(mc::Channel& channel) const override;
  std::string to_string() const override;
  auto const& get_match_fun() const { return match_fun_; }
  void* get_match_data() const { return match_data_; }
};

class MessIputSimcall final : public SimcallObserver {
  activity::MessageQueueImpl* queue_;
  void* payload_;
  activity::MessImpl* mess_ = {};
  bool detached_;
  std::function<void(void*)> clean_fun_; // used to free the synchro in case of problem after a detached send

public:
  MessIputSimcall(
      ActorImpl* actor, activity::MessageQueueImpl* queue,
      const std::function<void(void*)>& clean_fun, // used to free the synchro in case of problem after a detached send
      void* payload, bool detached)
      : SimcallObserver(actor)
      , queue_(queue)
      , payload_(payload)
      , detached_(detached)
      , clean_fun_(clean_fun)
  {
  }
  void serialize(mc::Channel& channel) const override;
  std::string to_string() const override;
  activity::MessageQueueImpl* get_queue() const { return queue_; }
  void* get_payload() const { return payload_; }
  void set_message(activity::MessImpl* mess) { mess_ = mess; }
  bool is_detached() const { return detached_; }
  auto const& get_clean_fun() const { return clean_fun_; }
};

class MessIgetSimcall final : public SimcallObserver {
  activity::MessageQueueImpl* queue_;
  unsigned char* dst_buff_;
  size_t* dst_buff_size_;
  void* payload_;
  activity::MessImpl* mess_ = {};

public:
  MessIgetSimcall(ActorImpl* actor, activity::MessageQueueImpl* queue, unsigned char* dst_buff, size_t* dst_buff_size,
                  void* payload)
      : SimcallObserver(actor)
      , queue_(queue)
      , dst_buff_(dst_buff)
      , dst_buff_size_(dst_buff_size)
      , payload_(payload)
  {
  }
  void serialize(mc::Channel& channel) const override;
  std::string to_string() const override;
  activity::MessageQueueImpl* get_queue() const { return queue_; }
  unsigned char* get_dst_buff() const { return dst_buff_; }
  size_t* get_dst_buff_size() const { return dst_buff_size_; }
  void* get_payload() const { return payload_; }
  void set_message(activity::MessImpl* mess) { mess_ = mess; }
};

} // namespace simgrid::kernel::actor

#endif
