/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_SYNCHRO_EXEC_HPP
#define SIMIX_SYNCHRO_EXEC_HPP

#include "src/kernel/activity/ActivityImpl.hpp"
#include "src/kernel/context/Context.hpp"
#include "surf/surf.hpp"

namespace simgrid {
namespace kernel {
namespace activity {

class XBT_PUBLIC ExecImpl : public ActivityImpl {
  resource::Action* timeout_detector_ = nullptr;
  ~ExecImpl();

public:
  ExecImpl* start(double flops_amount, double priority, double bound);
  ExecImpl* start(const std::vector<s4u::Host*>& hosts, const std::vector<double>& flops_amounts,
                  const std::vector<double>& bytes_amounts);

  ExecImpl* set_name(const std::string& name);
  ExecImpl* set_tracing_category(const std::string& category);
  ExecImpl* set_host(s4u::Host* host);
  ExecImpl* set_timeout(double timeout);
  void cancel();
  void post() override;
  void finish() override;
  double get_remaining() const;
  double get_seq_remaining_ratio();
  double get_par_remaining_ratio();
  void set_bound(double bound);       // deprecated. To be removed in v3.25
  void set_priority(double priority); // deprecated. To be removed in v3.25
  virtual ActivityImpl* migrate(s4u::Host* to);

  /* The host where the execution takes place. nullptr means this is a parallel exec (and only surf knows the hosts) */
  s4u::Host* host_ = nullptr;

  static xbt::signal<void(ExecImpl&)> on_creation;
  static xbt::signal<void(ExecImpl const&)> on_completion;
  static xbt::signal<void(ExecImpl const&, s4u::Host*)> on_migration;
};
} // namespace activity
} // namespace kernel
} // namespace simgrid
#endif
