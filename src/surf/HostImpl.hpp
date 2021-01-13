/* Copyright (c) 2004-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_HOST_INTERFACE_HPP
#define SURF_HOST_INTERFACE_HPP

#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/resource/DiskImpl.hpp"
#include "src/surf/StorageImpl.hpp"
#include "src/surf/cpu_interface.hpp"
#include "src/surf/network_interface.hpp"
#include <xbt/PropertyHolder.hpp>

#include <vector>

namespace simgrid {
namespace surf {

/*********
 * Model *
 *********/

/** @ingroup SURF_host_interface
 * @brief SURF Host model interface class
 * @details A model is an object which handle the interactions between its Resources and its Actions
 */
class XBT_PRIVATE HostModel : public kernel::resource::Model {
public:
  HostModel() : Model(Model::UpdateAlgo::FULL) {}

  virtual kernel::resource::Action* execute_parallel(const std::vector<s4u::Host*>& host_list,
                                                     const double* flops_amount, const double* bytes_amount,
                                                     double rate) = 0;
};

/************
 * Resource *
 ************/
/** @ingroup SURF_host_interface
 * @brief SURF Host interface class
 * @details A host represents a machine with an aggregation of a Cpu, a RoutingEdge and a Storage
 */
class XBT_PRIVATE HostImpl : public xbt::PropertyHolder {
  std::vector<kernel::actor::ProcessArg*> actors_at_boot_;
  s4u::Host* piface_ = nullptr; // we must have a pointer there because the VM wants to change the piface in its ctor
  std::map<std::string, kernel::resource::StorageImpl*, std::less<>> storage_;
  std::vector<kernel::resource::DiskImpl*> disks_;

public:
  friend simgrid::vm::VirtualMachineImpl;
  explicit HostImpl(s4u::Host* host);
  virtual ~HostImpl();

  std::vector<s4u::Disk*> get_disks() const;
  void set_disks(const std::vector<kernel::resource::DiskImpl*>& disks, s4u::Host* host);
  void add_disk(const s4u::Disk* disk);
  void remove_disk(const std::string& disk_name);

  /** @brief Get the vector of storages (by names) attached to the Host */
  virtual std::vector<const char*> get_attached_storages();
  std::unordered_map<std::string, s4u::Storage*>* get_mounted_storages();
  void set_storages(const std::map<std::string, kernel::resource::StorageImpl*, std::less<>>& storages)
  {
    storage_ = storages;
  }

  s4u::Host* get_iface() const { return piface_; }

  void turn_on() const;
  void turn_off(const kernel::actor::ActorImpl* issuer);
  std::vector<s4u::ActorPtr> get_all_actors();
  size_t get_actor_count() const;
  void add_actor(kernel::actor::ActorImpl* actor) { actor_list_.push_back(*actor); }
  void remove_actor(kernel::actor::ActorImpl* actor) { xbt::intrusive_erase(actor_list_, *actor); }
  void add_actor_at_boot(kernel::actor::ProcessArg* arg) { actors_at_boot_.emplace_back(arg); }

  using ActorList = boost::intrusive::list<
      kernel::actor::ActorImpl,
      boost::intrusive::member_hook<kernel::actor::ActorImpl, boost::intrusive::list_member_hook<>,
                                    &kernel::actor::ActorImpl::host_actor_list_hook>>;

  // FIXME: make these private
  ActorList actor_list_;
};
}
}

XBT_PUBLIC_DATA simgrid::surf::HostModel* surf_host_model;

#endif /* SURF_HOST_INTERFACE_HPP */
