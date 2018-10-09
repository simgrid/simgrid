/* Copyright (c) 2017-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_EXEC_HPP
#define SIMGRID_S4U_EXEC_HPP

#include <simgrid/forward.h>
#include <simgrid/s4u/Activity.hpp>

#include <atomic>

namespace simgrid {
namespace s4u {

/** Computation #Activity, representing the asynchronous disk access.
 *
 * They are generated from simgrid::s4u::this_actor::exec_init() or simgrid::s4u::Host::execute().
 */

class XBT_PUBLIC Exec : public Activity {
  Exec() : Activity() {}
public:
  friend XBT_PUBLIC void intrusive_ptr_release(simgrid::s4u::Exec * e);
  friend XBT_PUBLIC void intrusive_ptr_add_ref(simgrid::s4u::Exec * e);
  friend XBT_PUBLIC ExecPtr this_actor::exec_init(double flops_amount);

  ~Exec() = default;

  static simgrid::xbt::signal<void(simgrid::s4u::ActorPtr)> on_start;
  static simgrid::xbt::signal<void(simgrid::s4u::ActorPtr)> on_completion;

  Exec* start() override;
  Exec* wait() override;
  Exec* wait_for(double timeout) override;
  Exec* cancel() override;
  bool test() override;

  ExecPtr set_priority(double priority);
  ExecPtr set_bound(double bound);
  ExecPtr set_host(Host* host);
  ExecPtr set_name(std::string name);
  ExecPtr set_tracing_category(std::string category);
  Host* get_host();

  double get_remaining() override;
  double get_remaining_ratio();

#ifndef DOXYGEN
  //////////////// Deprecated functions
  XBT_ATTRIB_DEPRECATED_v324("Please use Exec::wait_for()") void wait(double t) override { wait_for(t); }
  XBT_ATTRIB_DEPRECATED_v323("Please use Exec::set_priority()") ExecPtr setPriority(double priority)
  {
    return set_priority(priority);
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use Exec::set_bound()") ExecPtr setBound(double bound) { return set_bound(bound); }
  XBT_ATTRIB_DEPRECATED_v323("Please use Exec::set_host()") ExecPtr setHost(Host* host) { return set_host(host); }
  XBT_ATTRIB_DEPRECATED_v323("Please use Exec::get_host()") Host* getHost() { return get_host(); }
  XBT_ATTRIB_DEPRECATED_v323("Please use Exec::get_remaining_ratio()") double getRemainingRatio()
  {
    return get_remaining_ratio();
  }
#endif

private:
  Host* host_                   = nullptr;
  double flops_amount_          = 0.0;
  double priority_              = 1.0;
  double bound_                 = 0.0;
  std::string name_             = "";
  std::string tracing_category_ = "";
  std::atomic_int_fast32_t refcount_{0};
}; // class
}
}; // Namespace simgrid::s4u

#endif /* SIMGRID_S4U_EXEC_HPP */
