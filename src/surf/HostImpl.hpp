/* Copyright (c) 2004-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_HOST_INTERFACE_HPP_
#define SURF_HOST_INTERFACE_HPP_

#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/resource/DiskImpl.hpp"
#include "src/surf/PropertyHolder.hpp"
#include "src/surf/StorageImpl.hpp"
#include "src/surf/cpu_interface.hpp"
#include "src/surf/network_interface.hpp"

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
                                                     double rate);
};

/************
 * Resource *
 ************/
/** @ingroup SURF_host_interface
 * @brief SURF Host interface class
 * @details An host represents a machine with a aggregation of a Cpu, a RoutingEdge and a Storage
 */
class XBT_PRIVATE HostImpl : public simgrid::surf::PropertyHolder {
  std::vector<kernel::actor::ProcessArg*> actors_at_boot_;

public:
  friend simgrid::vm::VirtualMachineImpl;
  explicit HostImpl(s4u::Host* host);
  virtual ~HostImpl();

  std::vector<s4u::Disk*> get_disks();
  void add_disk(s4u::Disk* disk);
  void remove_disk(const std::string& disk_name);

  /** @brief Get the vector of storages (by names) attached to the Host */
  virtual std::vector<const char*> get_attached_storages();

  std::map<std::string, kernel::resource::StorageImpl*> storage_;
  std::vector<kernel::resource::DiskImpl*> disks_;

  s4u::Host* piface_ = nullptr;

  void turn_on();
  void turn_off();
  std::vector<s4u::ActorPtr> get_all_actors();
  size_t get_actor_count();
  void add_actor(kernel::actor::ActorImpl* actor) { actor_list_.push_back(*actor); }
  void remove_actor(kernel::actor::ActorImpl* actor) { xbt::intrusive_erase(actor_list_, *actor); }
  void add_actor_at_boot(kernel::actor::ProcessArg* arg) { actors_at_boot_.emplace_back(arg); }

  typedef boost::intrusive::list<
      kernel::actor::ActorImpl,
      boost::intrusive::member_hook<kernel::actor::ActorImpl, boost::intrusive::list_member_hook<>,
                                    &kernel::actor::ActorImpl::host_actor_list_hook>>
      ActorList;

  // FIXME: make these private
  ActorList actor_list_;
};
}
}

XBT_PUBLIC_DATA simgrid::surf::HostModel* surf_host_model;

#endif /* SURF_Host_INTERFACE_HPP_ */
