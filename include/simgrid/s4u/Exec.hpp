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

class XBT_PUBLIC Exec : public Activity {
  Exec() : Activity() {}
public:
  friend XBT_PUBLIC void intrusive_ptr_release(simgrid::s4u::Exec * e);
  friend XBT_PUBLIC void intrusive_ptr_add_ref(simgrid::s4u::Exec * e);
  friend XBT_PUBLIC ExecPtr this_actor::exec_init(double flops_amount);

  ~Exec() = default;

  Activity* start() override;
  Activity* wait() override;
  Activity* wait(double timeout) override;
  bool test();

  ExecPtr set_priority(double priority);
  ExecPtr set_bound(double bound);
  ExecPtr set_host(Host* host);
  Host* get_host();

  double get_remaining() override;
  double getRemainingRatio();

  //////////////// Deprecated functions
  XBT_ATTRIB_DEPRECATED_v323("Please use Exec::set_priority()") ExecPtr setPriority(double priority)
  {
    return set_priority(priority);
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use Exec::set_bound()") ExecPtr setBound(double bound) { return set_bound(bound); }
  XBT_ATTRIB_DEPRECATED_v323("Please use Exec::set_host()") ExecPtr setHost(Host* host) { return set_host(host); }
  XBT_ATTRIB_DEPRECATED_v323("Please use Exec::get_host()") Host* getHost() { return get_host(); }

private:
  Host* host_          = nullptr;
  double flops_amount_ = 0.0;
  double priority_     = 1.0;
  double bound_        = 0.0;
  std::atomic_int_fast32_t refcount_{0};
}; // class
}
}; // Namespace simgrid::s4u

#endif /* SIMGRID_S4U_EXEC_HPP */
