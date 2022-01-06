/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ACTIVITY_EXEC_HPP
#define SIMGRID_KERNEL_ACTIVITY_EXEC_HPP

#include <simgrid/s4u/Exec.hpp>

#include "src/kernel/activity/ActivityImpl.hpp"
#include "src/kernel/context/Context.hpp"

namespace simgrid {
namespace kernel {
namespace activity {

class XBT_PUBLIC ExecImpl : public ActivityImpl_T<ExecImpl> {
  std::unique_ptr<resource::Action, std::function<void(resource::Action*)>> timeout_detector_{
      nullptr, [](resource::Action* a) { a->unref(); }};
  double sharing_penalty_             = 1.0;
  double bound_                       = 0.0;
  std::vector<s4u::Host*> hosts_;
  std::vector<double> flops_amounts_;
  std::vector<double> bytes_amounts_;
  int cb_id_ = -1; // callback id from Host::on_state_change.connect()

public:
  ExecImpl();

  ExecImpl& set_timeout(double timeout) override;
  ExecImpl& set_bound(double bound);
  ExecImpl& set_sharing_penalty(double sharing_penalty);
  ExecImpl& update_sharing_penalty(double sharing_penalty);

  void set_cb_id(unsigned int cb_id) { cb_id_ = cb_id; }

  ExecImpl& set_flops_amount(double flop_amount);
  ExecImpl& set_host(s4u::Host* host);
  s4u::Host* get_host() const { return hosts_.front(); }
  const std::vector<s4u::Host*>& get_hosts() const { return hosts_; }

  ExecImpl& set_flops_amounts(const std::vector<double>& flops_amounts);
  ExecImpl& set_bytes_amounts(const std::vector<double>& bytes_amounts);
  ExecImpl& set_hosts(const std::vector<s4u::Host*>& hosts);

  unsigned int get_host_number() const { return hosts_.size(); }
  double get_seq_remaining_ratio();
  double get_par_remaining_ratio();
  double get_remaining() const override;
  virtual ActivityImpl* migrate(s4u::Host* to);

  ExecImpl* start();
  void post() override;
  void set_exception(actor::ActorImpl* issuer) override;
  void finish() override;

  void reset();
  static void wait_any_for(actor::ActorImpl* issuer, const std::vector<ExecImpl*>& execs, double timeout);

  static xbt::signal<void(ExecImpl const&, s4u::Host*)> on_migration;
};
} // namespace activity
} // namespace kernel
} // namespace simgrid
#endif
