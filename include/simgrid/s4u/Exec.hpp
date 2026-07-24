/* Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.          */

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
  /** @brief Returns the ratio of elements that are still to do
   *
   * The returned value is between 0 (completely done) and 1 (nothing done yet). */
  double get_remaining_ratio() const;
  /** @brief Change the host on which this activity takes place.
   *
   * This cannot be done once the activity is terminated, but it can be done on started executions. */
  ExecPtr set_host(s4u::Host* host);
  /** @brief Change this sequential execution into a parallel one, spread over the given hosts.
   *
   * This turns the execution into a parallel one: use set_flops_amounts() and set_bytes_amounts() to specify the
   * computations and communications involved. Cannot be done once the activity is terminated. */
  ExecPtr set_hosts(const std::vector<s4u::Host*>& hosts);
  /** @brief Reset the execution to a state with no host assigned, so that it can be (re)assigned later on. */
  ExecPtr unset_host();
  /** @brief Reset the execution to a state with no host assigned. Same as unset_host(). */
  ExecPtr unset_hosts() { return unset_host(); }

  /** @brief Set the amount of flops to execute, for sequential executions.
   *
   * Cannot be changed once the exec has started. Not to be used on parallel executions: use set_flops_amounts()
   * instead. */
  ExecPtr set_flops_amount(double flops_amount);
  /** @brief Set the amount of flops to execute on each host, for parallel executions.
   *
   * This vector must have the same size as the vector of hosts given to set_hosts(). See also
   * set_bytes_amounts() to specify the communications happening between these hosts. */
  ExecPtr set_flops_amounts(const std::vector<double>& flops_amounts);
  /** @brief Set the amount of bytes to exchange between each pair of hosts, for parallel executions.
   *
   * This must be a host_count-square matrix, given as a flat vector: `bytes_amounts[i * host_count + j]`
   * specifies the amount of bytes to send from host i to host j. See also set_flops_amounts() to specify the
   * computations happening on each host. */
  ExecPtr set_bytes_amounts(const std::vector<double>& bytes_amounts);

  /** @brief Change the amount of threads that this execution uses on its host.
   *
   * This models a multi-threaded execution, where the given amount of flops is spread over several cores of the
   * host, potentially speeding up the execution when several cores are available. Defaults to 1 (sequential
   * execution). Cannot be changed once the exec started. See also this_actor::thread_execute(). */
  ExecPtr set_thread_count(int thread_count);

  /** @brief change the execution bound
   * This means changing the maximal amount of flops per second that it may consume, regardless of what the host may
   * deliver. Currently, this cannot be changed once the exec started. See also the "cloud-capping" example.  */
  ExecPtr set_bound(double bound);

  /** @brief  Change the execution priority, don't you think?
   * An execution with twice the priority will get twice the amount of flops when the resource is shared.
   * The default priority is 1.
   *
   * Currently, this cannot be changed once the exec started. */
  ExecPtr set_priority(double priority);
  /** @brief Change the execution priority while it is already running, without touching its remaining amount of
   *  flops. See also set_priority(). */
  ExecPtr update_priority(double priority);

  /** @brief Retrieve the host on which this activity takes place.
   *  If it runs on more than one host, only the first host is returned. */
  s4u::Host* get_host() const;
  /** @brief Retrieve the amount of hosts involved in this execution.
   *  This is 1 for sequential executions, and more for parallel ones. */
  unsigned int get_host_number() const;
  /** @brief Retrieve the amount of threads used by this (sequential) execution. See set_thread_count(). */
  int get_thread_count() const;
  /** @brief Retrieve the amount of flops that this execution will use, as specified with set_flops_amount(). */
  double get_cost() const;
  /** @brief Returns whether this execution is a parallel one, i.e. whether it was created with several hosts. */
  bool is_parallel() const { return parallel_; }
  /** @brief Returns whether this execution is assigned to the host(s) that it needs to start. An unassigned
   *  execution cannot start. */
  bool is_assigned() const override;
};

} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_EXEC_HPP */
