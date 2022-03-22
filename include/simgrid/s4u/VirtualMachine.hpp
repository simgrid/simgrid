/* Copyright (c) 2015-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_VM_HPP
#define SIMGRID_S4U_VM_HPP

#include <simgrid/forward.h>
#include <simgrid/s4u/Host.hpp>
#include <xbt/utility.hpp>

namespace simgrid {
namespace s4u {

/** @brief Host extension for the VMs */
class VmHostExt {
public:
  static xbt::Extension<s4u::Host, VmHostExt> EXTENSION_ID;

  sg_size_t ramsize = 0;    /* available ramsize (0= not taken into account) */
  bool overcommit   = true; /* Whether the host allows overcommiting more VM than the avail ramsize allows */
  static void ensureVmExtInstalled();
};

/** @ingroup s4u_api
 *
 * @tableofcontents
 *
 * A VM represents a virtual machine (or a container) that hosts actors.
 * The total computing power that the contained actors can get is constrained to the virtual machine size.
 *
 */
class XBT_PUBLIC VirtualMachine : public s4u::Host {
  kernel::resource::VirtualMachineImpl* const pimpl_vm_;

  /* Signals about the life cycle of the VM */
  static xbt::signal<void(VirtualMachine&)> on_creation;
  static xbt::signal<void(VirtualMachine const&)> on_start;
  static xbt::signal<void(VirtualMachine const&)> on_started;
  static xbt::signal<void(VirtualMachine const&)> on_shutdown;
  static xbt::signal<void(VirtualMachine const&)> on_suspend;
  static xbt::signal<void(VirtualMachine const&)> on_resume;
  static xbt::signal<void(VirtualMachine const&)> on_migration_start;
  static xbt::signal<void(VirtualMachine const&)> on_migration_end;
  static xbt::signal<void(VirtualMachine const&)> on_destruction;

public:
  explicit VirtualMachine(const std::string& name, Host* physical_host, int core_amount, size_t ramsize = 1024);

#ifndef DOXYGEN
  // No copy/move
  VirtualMachine(VirtualMachine const&) = delete;
  VirtualMachine& operator=(VirtualMachine const&) = delete;
#endif

  // enum class State { ... }
  XBT_DECLARE_ENUM_CLASS(State,
    CREATED, /**< created, but not yet started */
    RUNNING,
    SUSPENDED, /**< Suspend/resume does not involve disk I/O, so we assume there is no transition states. */
    DESTROYED
  );

  kernel::resource::VirtualMachineImpl* get_vm_impl() const { return pimpl_vm_; }
  void start();
  void suspend();
  void resume();
  void shutdown();
  void destroy() override;

  Host* get_pm() const;
  VirtualMachine* set_pm(Host* pm);
  size_t get_ramsize() const;
  VirtualMachine* set_ramsize(size_t ramsize);
  VirtualMachine* set_bound(double bound);
  void start_migration() const;
  void end_migration() const;

  State get_state() const;

  /* Callbacks on signals */
  static void on_creation_cb(const std::function<void(VirtualMachine&)>& cb) { on_creation.connect(cb); }
  static void on_start_cb(const std::function<void(VirtualMachine const&)>& cb) { on_start.connect(cb); }
  static void on_started_cb(const std::function<void(VirtualMachine const&)>& cb) { on_started.connect(cb); }
  static void on_shutdown_cb(const std::function<void(VirtualMachine const&)>& cb) { on_shutdown.connect(cb); }
  static void on_suspend_cb(const std::function<void(VirtualMachine const&)>& cb) { on_suspend.connect(cb); }
  static void on_resume_cb(const std::function<void(VirtualMachine const&)>& cb) { on_resume.connect(cb); }
  static void on_destruction_cb(const std::function<void(VirtualMachine const&)>& cb) { on_destruction.connect(cb); }
  static void on_migration_start_cb(const std::function<void(VirtualMachine const&)>& cb)
  {
    on_migration_start.connect(cb);
  }
  static void on_migration_end_cb(const std::function<void(VirtualMachine const&)>& cb)
  {
    on_migration_end.connect(cb);
  }
};
} // namespace s4u
} // namespace simgrid

#endif
