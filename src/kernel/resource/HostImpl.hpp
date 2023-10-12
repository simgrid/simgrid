/* Copyright (c) 2004-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_HOST_INTERFACE_HPP
#define SIMGRID_KERNEL_HOST_INTERFACE_HPP

#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/resource/CpuImpl.hpp"
#include "src/kernel/resource/DiskImpl.hpp"
#include <xbt/PropertyHolder.hpp>

#include <vector>

namespace simgrid::kernel::resource {
/*********
 * Model *
 *********/

/** @ingroup Model_host_interface
 * @brief Host model interface class
 * @details A model is an object which handle the interactions between its Resources and its Actions
 */
class XBT_PRIVATE HostModel : public Model {
public:
  using Model::Model;
  virtual Action* execute_thread(const s4u::Host* host, double flops_amount, int thread_count) = 0;

  virtual Action* execute_parallel(const std::vector<s4u::Host*>& host_list, const double* flops_amount,
                                   const double* bytes_amount, double rate) = 0;
  Action* io_stream(s4u::Host* src_host, DiskImpl* src_disk, s4u::Host* dst_host, DiskImpl* dst_disk,
                            double size);
};

/************
 * Resource *
 ************/
/** @ingroup Model_host_interface
 * @brief Host interface class
 * @details A host represents a machine with an aggregation of a Cpu, a RoutingEdge and Disk(s)
 */
class XBT_PRIVATE HostImpl : public xbt::PropertyHolder, public actor::ObjectAccessSimcallItem {
  using ActorList =
      boost::intrusive::list<actor::ActorImpl,
                             boost::intrusive::member_hook<actor::ActorImpl, boost::intrusive::list_member_hook<>,
                                                           &actor::ActorImpl::host_actor_list_hook>>;

  ActorList actor_list_;
  std::vector<actor::ProcessArg*> actors_at_boot_;
  s4u::Host piface_;
  std::map<std::string, DiskImplPtr, std::less<>> disks_;
  std::map<std::string, VirtualMachineImpl*, std::less<>> vms_;
  std::string name_{"noname"};
  routing::NetZoneImpl* englobing_zone_ = nullptr;
  bool sealed_                          = false;

protected:
  virtual ~HostImpl(); // Use destroy() instead of this destructor.

public:
  friend VirtualMachineImpl;
  explicit HostImpl(const std::string& name);

  void destroy(); // Must be called instead of the destructor

  std::vector<s4u::Disk*> get_disks() const;
  s4u::Disk* create_disk(const std::string& name, double read_bandwidth, double write_bandwidth);
  s4u::VirtualMachine* create_vm(const std::string& name, int core_amount, size_t ramsize = 1024);
  s4u::VirtualMachine* create_vm(const std::string& name, s4u::VirtualMachine* vm);
  void destroy_vm(const std::string& name);
  void add_disk(const s4u::Disk* disk);
  void remove_disk(const std::string& name);
  /** @brief Moves VM from this host to destination. Only sets the vm_ accordingly */
  void move_vm(VirtualMachineImpl* vm, HostImpl* destination);
  std::vector<s4u::VirtualMachine*> get_vms() const;
  VirtualMachineImpl* get_vm_by_name_or_null(const std::string& name) const;

  virtual const s4u::Host* get_iface() const { return &piface_; }
  virtual s4u::Host* get_iface() { return &piface_; }

  /** Retrieves the name of that host as a C++ string */
  std::string const& get_name() const { return name_; }
  /** Retrieves the name of that host as a C string */
  const char* get_cname() const { return name_.c_str(); }

  routing::NetZoneImpl* get_englobing_zone() const { return englobing_zone_; }
  /** @brief Set the NetZone in which this Host is included */
  HostImpl* set_englobing_zone(routing::NetZoneImpl* netzone_p);

  void turn_on() const;
  void turn_off(const actor::ActorImpl* issuer);
  std::vector<s4u::ActorPtr> get_all_actors();
  size_t get_actor_count() const;
  void add_actor(actor::ActorImpl* actor) { actor_list_.push_back(*actor); }
  void remove_actor(actor::ActorImpl* actor) { xbt::intrusive_erase(actor_list_, *actor); }
  void add_actor_at_boot(actor::ProcessArg* arg) { actors_at_boot_.emplace_back(arg); }

  virtual void seal();

  template <class F> void foreach_actor(F function)
  {
    for (auto& actor : actor_list_)
      function(actor);
  }
};
} // namespace simgrid::kernel::resource

#endif /* HOST_INTERFACE_HPP */
