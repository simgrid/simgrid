/* Copyright (c) 2015-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_VM_HPP
#define SIMGRID_S4U_VM_HPP

#include <simgrid/forward.h>
#include <simgrid/s4u/Host.hpp>

namespace simgrid {
namespace s4u {

/** @ingroup s4u_api
 *
 * @tableofcontents
 *
 * A VM represents a virtual machine (or a container) that hosts actors.
 * The total computing power that the contained actors can get is constrained to the virtual machine size.
 *
 */
class XBT_PUBLIC VirtualMachine : public s4u::Host {
  vm::VirtualMachineImpl* const pimpl_vm_;
  virtual ~VirtualMachine();

public:
  explicit VirtualMachine(const std::string& name, Host* physical_host, int core_amount);
  explicit VirtualMachine(const std::string& name, Host* physical_host, int core_amount, size_t ramsize);

  // No copy/move
  VirtualMachine(VirtualMachine const&) = delete;
  VirtualMachine& operator=(VirtualMachine const&) = delete;

  enum class state {
    CREATED, /**< created, but not yet started */
    RUNNING,
    SUSPENDED, /**< Suspend/resume does not involve disk I/O, so we assume there is no transition states. */
    DESTROYED
  };

  vm::VirtualMachineImpl* get_impl() const { return pimpl_vm_; }
  void start();
  void suspend();
  void resume();
  void shutdown();
  void destroy() override;

  Host* get_pm() const;
  void set_pm(Host* pm);
  size_t get_ramsize() const;
  void set_ramsize(size_t ramsize);
  void set_bound(double bound);

  VirtualMachine::state get_state();
  static xbt::signal<void(VirtualMachine const&)> on_start;
  static xbt::signal<void(VirtualMachine const&)> on_started;
  static xbt::signal<void(VirtualMachine const&)> on_shutdown;
  static xbt::signal<void(VirtualMachine const&)> on_suspend;
  static xbt::signal<void(VirtualMachine const&)> on_resume;
  static xbt::signal<void(VirtualMachine const&)> on_migration_start;
  static xbt::signal<void(VirtualMachine const&)> on_migration_end;
};
} // namespace s4u
} // namespace simgrid

#endif
