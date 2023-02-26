/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ACTIVITY_EXEC_HPP
#define SIMGRID_KERNEL_ACTIVITY_EXEC_HPP

#include <simgrid/s4u/Exec.hpp>

#include "src/kernel/activity/ActivityImpl.hpp"
#include "src/kernel/context/Context.hpp"

namespace simgrid::kernel::activity {

class XBT_PUBLIC ExecImpl : public ActivityImpl_T<ExecImpl> {
  double sharing_penalty_             = 1.0;
  double bound_                       = 0.0;
  std::vector<double> flops_amounts_;
  std::vector<double> bytes_amounts_;
  int thread_count_ = 1;

public:
  ExecImpl();

  ExecImpl& set_bound(double bound);
  ExecImpl& set_sharing_penalty(double sharing_penalty);
  ExecImpl& update_sharing_penalty(double sharing_penalty);

  ExecImpl& set_flops_amount(double flop_amount);
  ExecImpl& set_host(s4u::Host* host);

  ExecImpl& set_flops_amounts(const std::vector<double>& flops_amounts);
  ExecImpl& set_bytes_amounts(const std::vector<double>& bytes_amounts);
  ExecImpl& set_thread_count(int thread_count);
  ExecImpl& set_hosts(const std::vector<s4u::Host*>& hosts);

  unsigned int get_host_number() const { return static_cast<unsigned>(get_hosts().size()); }
  int get_thread_count() const { return thread_count_; }
  double get_seq_remaining_ratio();
  double get_par_remaining_ratio();
  double get_remaining() const override;
  virtual ActivityImpl* migrate(s4u::Host* to);

  ExecImpl* start();
  void set_exception(actor::ActorImpl* issuer) override;
  void finish() override;

  void reset();

  static xbt::signal<void(ExecImpl const&, s4u::Host*)> on_migration;
};
} // namespace simgrid::kernel::activity
#endif
