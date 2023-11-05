/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ACTIVITY_COMM_HPP
#define SIMGRID_KERNEL_ACTIVITY_COMM_HPP

#include "src/kernel/activity/ActivityImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/CommObserver.hpp"

namespace simgrid::kernel::activity {

enum class CommImplType { SEND, RECEIVE };

using timeout_action_type = std::unique_ptr<resource::Action, std::function<void(resource::Action*)>>;

class XBT_PUBLIC CommImpl : public ActivityImpl_T<CommImpl> {
  ~CommImpl() override;

  static std::function<void(CommImpl*, void*, size_t)> copy_data_callback_;

  double rate_       = -1.0;
  double size_       = 0.0;
  bool detached_     = false;   /* If detached or not */
  bool copied_       = false;   /* whether the data were already copied */
  MailboxImpl* mbox_ = nullptr; /* Rendez-vous where the comm is queued. nullptr once the comm is matched with both a
                                   sender and receiver */
  long mbox_id_      = -1;      /* ID of the rendez-vous where the comm was first queued (for MC) */
  s4u::Host* from_   = nullptr; /* Pre-determined only for direct host-to-host communications */
  s4u::Host* to_     = nullptr; /* Otherwise, computed at start() time from the actors */
  CommImplType type_ = CommImplType::SEND; /* Type of the communication (SEND or RECEIVE) */

  static unsigned next_id_;        // Next ID to be given (for MC)
  const unsigned id_ = ++next_id_; // ID of this comm (for MC) -- 0 as an ID denotes "invalid/unknown comm"

public:
  CommImpl() = default;

  static void set_copy_data_callback(const std::function<void(CommImpl*, void*, size_t)>& callback);

  CommImpl& set_type(CommImplType type);
  CommImplType get_type() const { return type_; }
  CommImpl& set_source(s4u::Host* from);
  s4u::Host* get_source() const { return from_; }
  CommImpl& set_destination(s4u::Host* to);
  s4u::Host* get_destination() const { return to_; }
  CommImpl& set_size(double size);
  CommImpl& set_src_buff(unsigned char* buff, size_t size);
  CommImpl& set_dst_buff(unsigned char* buff, size_t* size);
  CommImpl& set_rate(double rate);
  CommImpl& set_mailbox(MailboxImpl* mbox);
  CommImpl& detach();

  double get_rate() const { return rate_; }
  MailboxImpl* get_mailbox() const { return mbox_; }
  unsigned get_mailbox_id() const { return mbox_id_; }
  unsigned get_id() const { return id_; }
  bool is_detached() const { return detached_; }
  bool is_assigned() const { return (to_ != nullptr && from_ != nullptr); }

  std::vector<s4u::Link*> get_traversed_links() const;
  void copy_data();

  static ActivityImplPtr isend(actor::CommIsendSimcall* observer);
  static ActivityImplPtr irecv(actor::CommIrecvSimcall* observer);

  bool test(actor::ActorImpl* issuer) override;
  void wait_for(actor::ActorImpl* issuer, double timeout) override;
  static void wait_any_for(actor::ActorImpl* issuer, const std::vector<CommImpl*>& comms, double timeout);

  CommImpl* start();
  void suspend() override;
  void resume() override;
  void cancel() override;
  void set_exception(actor::ActorImpl* issuer) override;
  void finish() override;

  std::function<void(void*)> clean_fun; /* Function to clean the detached src_buf if something goes wrong */
  std::function<bool(void*, void*, CommImpl*)> match_fun; /* Filter function used by the other side. It is used when
looking if a given communication matches my needs. For that, myself must match the
expectations of the other side, too. See  */
  std::function<void(CommImpl*, void*, size_t)> copy_data_fun;

  /* Model actions */
  timeout_action_type src_timeout_{nullptr, [](resource::Action* a) { a->unref(); }}; /* timeout set by the sender */
  timeout_action_type dst_timeout_{nullptr, [](resource::Action* a) { a->unref(); }}; /* timeout set by the receiver */

  actor::ActorImplPtr src_actor_ = nullptr;
  actor::ActorImplPtr dst_actor_ = nullptr;

  /* Data to be transferred */
  unsigned char* src_buff_ = nullptr;
  unsigned char* dst_buff_ = nullptr;
  size_t src_buff_size_    = 0;
  size_t* dst_buff_size_   = nullptr;
  void* payload_           = nullptr; // If dst_buff_ is NULL, the default copy callback puts the data here

  void* src_data_ = nullptr; /* User data associated to the communication */
  void* dst_data_ = nullptr;
};
} // namespace simgrid::kernel::activity

#endif
