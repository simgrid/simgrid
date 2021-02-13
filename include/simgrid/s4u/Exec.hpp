/* Copyright (c) 2017-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_EXEC_HPP
#define SIMGRID_S4U_EXEC_HPP

#include <simgrid/forward.h>
#include <simgrid/s4u/Activity.hpp>
#include <simgrid/s4u/Actor.hpp>
#include <xbt/ex.h>

namespace simgrid {
namespace s4u {

/** Computation Activity, representing the asynchronous executions.
 *
 * @rst
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
  friend kernel::activity::ExecImpl;
  double priority_              = 1.0;
  double bound_                 = 0.0;
  double timeout_               = -1.0; // Infinite timeout by default
  std::vector<double> flops_amounts_;
  std::vector<double> bytes_amounts_;
  std::vector<Host*> hosts_;
  bool parallel_ = false;
  double start_time_ = -1.0;
  double finish_time_ = -1.0;

protected:
  explicit Exec(kernel::activity::ExecImplPtr pimpl);

public:
  ~Exec() override = default;
#ifndef DOXYGEN
  Exec(Exec const&) = delete;
  Exec& operator=(Exec const&) = delete;

#endif
  static xbt::signal<void(Exec const&)> on_start;
  static xbt::signal<void(Exec const&)> on_completion;

  static ExecPtr init();
  Exec* start() override;
  /** @brief On sequential executions, returns the amount of flops that remain to be done; This cannot be used on
   * parallel executions. */
  double get_remaining() const override;
  double get_remaining_ratio() const;
  ExecPtr set_host(Host* host);
  ExecPtr set_hosts(const std::vector<Host*>& hosts);

  ExecPtr set_flops_amount(double flops_amount);
  ExecPtr set_flops_amounts(const std::vector<double>& flops_amounts);
  ExecPtr set_bytes_amounts(const std::vector<double>& bytes_amounts);

  Exec* wait() override;
  Exec* wait_for(double timeout) override;
  /*! take a vector of s4u::ExecPtr and return when one of them is finished.
   * The return value is the rank of the first finished ExecPtr. */
  static int wait_any(std::vector<ExecPtr>* execs) { return wait_any_for(execs, -1); }
  /*! Same as wait_any, but with a timeout. If the timeout occurs, parameter last is returned.*/
  static int wait_any_for(std::vector<ExecPtr>* execs, double timeout);

  ExecPtr set_bound(double bound);
  ExecPtr set_priority(double priority);
  XBT_ATTRIB_DEPRECATED_v329("Please use exec_init(...)->wait_for(timeout)") ExecPtr set_timeout(double timeout);

  Exec* cancel() override;
  Host* get_host() const;
  unsigned int get_host_number() const;
  double get_start_time() const { return start_time_; }
  double get_finish_time() const { return finish_time_; }
  void set_finish_time(double finish_time) { finish_time_ = finish_time; }
  double get_cost() const;
  bool is_parallel() const { return parallel_; }
  bool is_assigned() const override { return not hosts_.empty(); }
};

} // namespace s4u
} // namespace simgrid

#endif /* SIMGRID_S4U_EXEC_HPP */
