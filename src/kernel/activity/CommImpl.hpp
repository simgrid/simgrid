/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ACTIVITY_COMM_HPP
#define SIMGRID_KERNEL_ACTIVITY_COMM_HPP

#include "src/kernel/activity/ActivityImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"

namespace simgrid {
namespace kernel {
namespace activity {

class XBT_PUBLIC CommImpl : public ActivityImpl_T<CommImpl> {
  ~CommImpl() override;
  void cleanup_surf();

  static void (*copy_data_callback_)(CommImpl*, void*, size_t);

  double rate_       = 0.0;
  double size_       = 0.0;
  bool detached_     = false;   /* If detached or not */
  bool copied_       = false;   /* whether the data were already copied */
  MailboxImpl* mbox_ = nullptr; /* Rendez-vous where the comm is queued. nullptr once the comm is matched with both a
                                   sender and receiver */
  long mbox_id_      = -1;      /* ID of the rendez-vous where the comm was first queued (for MC) */
  s4u::Host* from_   = nullptr; /* Pre-determined only for direct host-to-host communications */
  s4u::Host* to_     = nullptr; /* Otherwise, computed at start() time from the actors */

public:
  enum class Type { SEND, RECEIVE };

  static void set_copy_data_callback(void (*callback)(CommImpl*, void*, size_t));

  explicit CommImpl(Type type) : type_(type) {}
  CommImpl(s4u::Host* from, s4u::Host* to, double bytes);

  CommImpl& set_size(double size);
  CommImpl& set_src_buff(unsigned char* buff, size_t size);
  CommImpl& set_dst_buff(unsigned char* buff, size_t* size);
  CommImpl& set_rate(double rate);
  CommImpl& set_mailbox(MailboxImpl* mbox);
  CommImpl& detach();

  double get_rate() const { return rate_; }
  MailboxImpl* get_mailbox() const { return mbox_; }
  long get_mailbox_id() const { return mbox_id_; }
  bool detached() const { return detached_; }

  std::vector<s4u::Link*> get_traversed_links() const;
  void copy_data();

  static ActivityImplPtr isend(const actor::CommIsendSimcall* observer);
  static ActivityImplPtr irecv(const actor::CommIrecvSimcall* observer);

  bool test(actor::ActorImpl* issuer) override;
  void wait_for(actor::ActorImpl* issuer, double timeout) override;
  static void wait_any_for(actor::ActorImpl* issuer, const std::vector<CommImpl*>& comms, double timeout);

  CommImpl* start();
  void suspend() override;
  void resume() override;
  void cancel() override;
  void post() override;
  void set_exception(actor::ActorImpl* issuer) override;
  void finish() override;

  const Type type_ = Type::SEND; /* Type of the communication (SEND or RECEIVE) */

#if SIMGRID_HAVE_MC
  MailboxImpl* mbox_cpy = nullptr; /* Copy of the rendez-vous where the comm is queued, MC needs it for DPOR
                                     (comm.mbox set to nullptr when the communication is removed from the mailbox
                                     (used as garbage collector)) */
#endif

  void (*clean_fun)(void*) = nullptr; /* Function to clean the detached src_buf if something goes wrong */
  bool (*match_fun)(void*, void*, CommImpl*) = nullptr; /* Filter function used by the other side. It is used when
looking if a given communication matches my needs. For that, myself must match the
expectations of the other side, too. See  */
  void (*copy_data_fun)(CommImpl*, void*, size_t) = nullptr;

  /* Surf action data */
  resource::Action* src_timeout_ = nullptr; /* Surf's actions to instrument the timeouts */
  resource::Action* dst_timeout_ = nullptr; /* Surf's actions to instrument the timeouts */
  actor::ActorImplPtr src_actor_ = nullptr;
  actor::ActorImplPtr dst_actor_ = nullptr;

  /* Data to be transferred */
  unsigned char* src_buff_ = nullptr;
  unsigned char* dst_buff_ = nullptr;
  size_t src_buff_size_    = 0;
  size_t* dst_buff_size_   = nullptr;

  void* src_data_ = nullptr; /* User data associated to the communication */
  void* dst_data_ = nullptr;
  static xbt::signal<void(CommImpl const&)> on_start;
  static xbt::signal<void(CommImpl const&)> on_completion;
};
} // namespace activity
} // namespace kernel
} // namespace simgrid

#endif
