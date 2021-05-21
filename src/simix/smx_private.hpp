/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_PRIVATE_HPP
#define SIMIX_PRIVATE_HPP

#include "simgrid/s4u/Actor.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/context/Context.hpp"

#include <boost/intrusive/list.hpp>
#include <mutex>
#include <unordered_map>
#include <vector>

/********************************** Simix Global ******************************/

namespace simgrid {
namespace simix {

class Global {
public:
  /**
   * Garbage collection
   *
   * Should be called some time to time to free the memory allocated for actors that have finished (or killed).
   */
  void empty_trash();
  void display_all_actor_status() const;

  kernel::context::ContextFactory* context_factory = nullptr;
  std::map<aid_t, kernel::actor::ActorImpl*> process_list;
  boost::intrusive::list<kernel::actor::ActorImpl,
                         boost::intrusive::member_hook<kernel::actor::ActorImpl, boost::intrusive::list_member_hook<>,
                                                       &kernel::actor::ActorImpl::smx_destroy_list_hook>>
      actors_to_destroy;
#if SIMGRID_HAVE_MC
  /* MCer cannot read members process_list and actors_to_destroy above in the remote process, so we copy the info it
   * needs in a dynar.
   * FIXME: This is supposed to be a temporary hack.
   * A better solution would be to change the split between MCer and MCed, where the responsibility
   *   to compute the list of the enabled transitions goes to the MCed.
   * That way, the MCer would not need to have the list of actors on its side.
   * These info could be published by the MCed to the MCer in a way inspired of vd.so
   */
  xbt_dynar_t actors_vector      = xbt_dynar_new(sizeof(kernel::actor::ActorImpl*), nullptr);
  xbt_dynar_t dead_actors_vector = xbt_dynar_new(sizeof(kernel::actor::ActorImpl*), nullptr);
#endif
  kernel::actor::ActorImpl* maestro_ = nullptr;

  std::mutex mutex;
};
}
}

XBT_PUBLIC_DATA std::unique_ptr<simgrid::simix::Global> simix_global;

XBT_PUBLIC void SIMIX_clean();

#endif
