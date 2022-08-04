/* Copyright (c) 2020-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "api.hpp"

#include "src/kernel/activity/MailboxImpl.hpp"
#include "src/kernel/activity/MutexImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/mc/api/RemoteApp.hpp"
#include "src/mc/explo/Exploration.hpp"
#include "src/mc/mc_base.hpp"
#include "src/mc/mc_exit.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/remote/RemoteProcess.hpp"
#include "src/surf/HostImpl.hpp"

#include <xbt/asserts.h>
#include <xbt/log.h>
#include "simgrid/s4u/Host.hpp"
#include "xbt/string.hpp"
#if HAVE_SMPI
#include "src/smpi/include/smpi_request.hpp"
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(Api, mc, "Logging specific to MC Facade APIs ");
XBT_LOG_EXTERNAL_CATEGORY(mc_global);

namespace simgrid::mc {

simgrid::mc::Exploration* Api::initialize(char** argv, const std::unordered_map<std::string, std::string>& env,
                                          simgrid::mc::ExplorationAlgorithm algo)
{
  remote_app_ = std::make_unique<simgrid::mc::RemoteApp>([argv, &env] {
    int i = 1;
    while (argv[i] != nullptr && argv[i][0] == '-')
      i++;

    for (auto const& [key, val] : env) {
      XBT_INFO("setenv '%s'='%s'", key.c_str(), val.c_str());
      setenv(key.c_str(), val.c_str(), 1);
    }
    xbt_assert(argv[i] != nullptr,
               "Unable to find a binary to exec on the command line. Did you only pass config flags?");
    execvp(argv[i], argv + i);
    xbt_die("The model-checked process failed to exec(%s): %s", argv[i], strerror(errno));
  });

  simgrid::mc::Exploration* explo;
  switch (algo) {
    case ExplorationAlgorithm::CommDeterminism:
      explo = simgrid::mc::create_communication_determinism_checker(*(remote_app_.get()));
      break;

    case ExplorationAlgorithm::UDPOR:
      explo = simgrid::mc::create_udpor_checker(*(remote_app_.get()));
      break;

    case ExplorationAlgorithm::Safety:
      explo = simgrid::mc::create_dfs_exploration(*(remote_app_.get()));
      break;

    case ExplorationAlgorithm::Liveness:
      explo = simgrid::mc::create_liveness_checker(*(remote_app_.get()));
      break;

    default:
      THROW_IMPOSSIBLE;
  }

  mc_model_checker->set_exploration(explo);
  return explo;
}


std::size_t Api::get_remote_heap_bytes() const
{
  RemoteProcess& process    = mc_model_checker->get_remote_process();
  auto heap_bytes_used      = mmalloc_get_bytes_used_remote(process.get_heap()->heaplimit, process.get_malloc_info());
  return heap_bytes_used;
}

bool Api::snapshot_equal(const Snapshot* s1, const Snapshot* s2) const
{
  return simgrid::mc::snapshot_equal(s1, s2);
}

simgrid::mc::Snapshot* Api::take_snapshot(long num_state) const
{
  auto snapshot = new simgrid::mc::Snapshot(num_state);
  return snapshot;
}

} // namespace simgrid::mc
