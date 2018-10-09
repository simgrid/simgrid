/* Copyright (c) 2015-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_VM_HPP
#define SIMGRID_S4U_VM_HPP

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
  simgrid::vm::VirtualMachineImpl* pimpl_vm_ = nullptr;
  virtual ~VirtualMachine();

public:
  explicit VirtualMachine(std::string name, s4u::Host* physical_host, int core_amount);
  explicit VirtualMachine(std::string name, s4u::Host* physical_host, int core_amount, size_t ramsize);

  // No copy/move
  VirtualMachine(VirtualMachine const&) = delete;
  VirtualMachine& operator=(VirtualMachine const&) = delete;

  enum class state {
    CREATED, /**< created, but not yet started */
    RUNNING,
    SUSPENDED, /**< Suspend/resume does not involve disk I/O, so we assume there is no transition states. */
    DESTROYED
  };

  simgrid::vm::VirtualMachineImpl* get_impl() { return pimpl_vm_; }
  void start();
  void suspend();
  void resume();
  void shutdown();
  void destroy();

  simgrid::s4u::Host* get_pm();
  void set_pm(simgrid::s4u::Host* pm);
  size_t get_ramsize();
  void set_ramsize(size_t ramsize);
  void set_bound(double bound);

  VirtualMachine::state get_state();
  static simgrid::xbt::signal<void(VirtualMachine&)> on_start;
  static simgrid::xbt::signal<void(VirtualMachine&)> on_started;
  static simgrid::xbt::signal<void(VirtualMachine&)> on_shutdown;
  static simgrid::xbt::signal<void(VirtualMachine&)> on_suspend;
  static simgrid::xbt::signal<void(VirtualMachine&)> on_resume;
  static simgrid::xbt::signal<void(VirtualMachine&)> on_migration_start;
  static simgrid::xbt::signal<void(VirtualMachine&)> on_migration_end;

  // Deprecated methods
  /** @deprecated See VirtualMachine::get_state() */
  XBT_ATTRIB_DEPRECATED_v323("Please use VirtualMachine::get_state()") VirtualMachine::state getState()
  {
    return get_state();
  }
  /** @deprecated See VirtualMachine::get_impl() */
  XBT_ATTRIB_DEPRECATED_v323("Please use VirtualMachine::get_impl()") simgrid::vm::VirtualMachineImpl* getImpl()
  {
    return pimpl_vm_;
  }
  /** @deprecated See VirtualMachine::get_pm() */
  XBT_ATTRIB_DEPRECATED_v323("Please use VirtualMachine::get_pm()") simgrid::s4u::Host* getPm() { return get_pm(); }
  /** @deprecated See VirtualMachine::set_pm() */
  XBT_ATTRIB_DEPRECATED_v323("Please use VirtualMachine::set_pm()") void setPm(simgrid::s4u::Host* pm) { set_pm(pm); }
  /** @deprecated See VirtualMachine::get_ramsize() */
  XBT_ATTRIB_DEPRECATED_v323("Please use VirtualMachine::get_ramsize()") size_t getRamsize() { return get_ramsize(); }
  /** @deprecated See VirtualMachine::set_ramsize() */
  XBT_ATTRIB_DEPRECATED_v323("Please use VirtualMachine::set_ramsize()") void setRamsize(size_t ramsize)
  {
    set_ramsize(ramsize);
  }
  /** @deprecated See VirtualMachine::set_bound() */
  XBT_ATTRIB_DEPRECATED_v323("Please use VirtualMachine::set_bound()") void setBound(double bound) { set_bound(bound); }
};
}
} // namespace simgrid::s4u

#endif
