/* Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_EXEC_HPP
#define SIMGRID_S4U_EXEC_HPP

#include <simgrid/forward.h>
#include <simgrid/s4u/Activity.hpp>
#include <simgrid/s4u/Actor.hpp>
#include <xbt/ex.h>

namespace simgrid::s4u {

/** Computation Activity, representing the asynchronous executions.
 *
 * @beginrst
 * Most of them are created with :cpp:func:`simgrid::s4u::this_actor::exec_init()` or
 * :cpp:func:`simgrid::s4u::Host::execute()`, and represent a classical (sequential) execution. This can be used to
 * simulate some computation occurring in another thread when the calling actor is not blocked during the execution.
 *
 * You can also use :cpp:func:`simgrid::s4u::this_actor::parallel_execute()` to create *parallel* executions. These
 * objects represent distributed computations involving computations on several hosts and communications between them.
 * Such objects can for example represent a matrix multiplication done with ScaLAPACK on a real system. Once created,
 * parallel Exec are very similar to the sequential ones. The only difference is that you cannot migrate them, and their
 * remaining amount of work can only be defined as a ratio. See the doc of :cpp:func:`simgrid::s4u::Exec::get_remaining`
 * and :cpp:func:`simgrid::s4u::Exec::get_remaining_ratio` for more info.
 * @endrst
 */
class XBT_PUBLIC Exec : public Activity_T<Exec> {
#ifndef DOXYGEN
  friend kernel::activity::ExecImpl;
  friend kernel::EngineImpl; // Auto-completes the execs of maestro (in simDAG)
#endif

  bool parallel_ = false;

protected:
  explicit Exec(kernel::activity::ExecImplPtr pimpl);
  Exec* do_start() override;

  void reset() const;

public:
#ifndef DOXYGEN
  Exec(Exec const&) = delete;
  Exec& operator=(Exec const&) = delete;
#endif
  /*! \static Initiate the creation of an Exec. Setters have to be called afterwards */
  static ExecPtr init();

  /** @brief On sequential executions, returns the amount of flops that remain to be done; This cannot be used on
   * parallel executions. */
  double get_remaining() const override;
  double get_remaining_ratio() const;
  ExecPtr set_host(Host* host);
  ExecPtr set_hosts(const std::vector<Host*>& hosts);
  ExecPtr unset_host();
  ExecPtr unset_hosts() { return unset_host(); }

  ExecPtr set_flops_amount(double flops_amount);
  ExecPtr set_flops_amounts(const std::vector<double>& flops_amounts);
  ExecPtr set_bytes_amounts(const std::vector<double>& bytes_amounts);

  ExecPtr set_thread_count(int thread_count);

  ExecPtr set_bound(double bound);
  ExecPtr set_priority(double priority);
  ExecPtr update_priority(double priority);

  Host* get_host() const;
  unsigned int get_host_number() const;
  int get_thread_count() const;
  double get_cost() const;
  bool is_parallel() const { return parallel_; }
  bool is_assigned() const override;

#ifndef DOXYGEN
  static ssize_t deprecated_wait_any_for(const std::vector<ExecPtr>& execs, double timeout); // XBT_ATTRIB_DEPRECATED_v339

  XBT_ATTRIB_DEPRECATED_v339("Please use ActivitySet instead") static ssize_t
      wait_any(const std::vector<ExecPtr>& execs) { return deprecated_wait_any_for(execs, -1); }
  XBT_ATTRIB_DEPRECATED_v339("Please use ActivitySet instead") static ssize_t
      wait_any_for(const std::vector<ExecPtr>& execs, double timeout) { return deprecated_wait_any_for(execs, timeout); }
#endif
};

} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_EXEC_HPP */
