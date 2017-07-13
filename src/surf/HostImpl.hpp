/* Copyright (c) 2004-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_interface.hpp"
#include "cpu_interface.hpp"
#include "network_interface.hpp"
#include "src/surf/PropertyHolder.hpp"

#include "StorageImpl.hpp"
#include <xbt/base.h>

#ifndef SURF_HOST_INTERFACE_HPP_
#define SURF_HOST_INTERFACE_HPP_

/***********
 * Classes *
 ***********/

namespace simgrid {
namespace surf {
class XBT_PRIVATE HostAction;
}
}

/*********
 * Tools *
 *********/

XBT_PUBLIC_DATA(simgrid::surf::HostModel*) surf_host_model;

/*********
 * Model *
 *********/

namespace simgrid {
namespace surf {

/** @ingroup SURF_host_interface
 * @brief SURF Host model interface class
 * @details A model is an object which handle the interactions between its Resources and its Actions
 */
class XBT_PRIVATE HostModel : public Model {
public:
  HostModel() : Model() {}

  virtual void ignoreEmptyVmInPmLMM();
  virtual Action* executeParallelTask(int host_nb, sg_host_t* host_list, double* flops_amount, double* bytes_amount,
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

public:
  explicit HostImpl(s4u::Host* host);
  virtual ~HostImpl() = default;

  /** @brief Get the vector of storages (by names) attached to the Host */
  virtual void getAttachedStorageList(std::vector<const char*>* storages);

  std::map<std::string, simgrid::surf::StorageImpl*> storage_;
  simgrid::s4u::Host* piface_ = nullptr;

  simgrid::s4u::Host* getHost() { return piface_; }
};
}
}

#endif /* SURF_Host_INTERFACE_HPP_ */
