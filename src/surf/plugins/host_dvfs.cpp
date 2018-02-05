/* Copyright (c) 2010, 2012-2018. The SimGrid Team. All rights reserved.    */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/dvfs.h"
#include "simgrid/plugins/load.h"
#include "simgrid/simix.hpp"
#include "src/plugins/vm/VirtualMachineImpl.hpp"
#include "src/surf/cpu_interface.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <simgrid/msg.h>
#include <simgrid/s4u/Engine.hpp>
#include <string>
#include <utility>
#include <vector>
#include <xbt/config.hpp>

/** @addtogroup SURF_plugin_load

  This plugin makes it very simple for users to obtain the current load for each host.

*/

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_plugin_dvfs, surf, "Logging specific to the SURF HostDvfs plugin");

namespace simgrid {
namespace plugin {

namespace dvfs {
class Governor {

protected:
  simgrid::s4u::Host* host;

public:
  double sampling_rate;

  explicit Governor(simgrid::s4u::Host* ptr) : host(ptr) { init(); }

  void init()
  {
    const char* local_sampling_rate_config = host->getProperty("plugin/dvfs/sampling_rate");
    double global_sampling_rate_config     = xbt_cfg_get_double("plugin/dvfs/sampling_rate");
    if (local_sampling_rate_config != nullptr) {
      sampling_rate = std::stod(local_sampling_rate_config);
    } else {
      sampling_rate = global_sampling_rate_config;
    }
  }

  virtual void update() {}
  double samplingRate() { return sampling_rate; }
};

class Performance : public Governor {
public:
  explicit Performance(simgrid::s4u::Host* ptr) : Governor(ptr) {}

  void update() { host->setPstate(0); }
};

class Powersave : public Governor {
public:
  explicit Powersave(simgrid::s4u::Host* ptr) : Governor(ptr) {}

  void update() { host->setPstate(host->getPstatesCount() - 1); }
};

class OnDemand : public Governor {
  double freq_up_threshold = 0.95;

public:
  explicit OnDemand(simgrid::s4u::Host* ptr) : Governor(ptr) {}

  void update()
  {
    double load = sg_host_get_current_load(host);

    if (load > freq_up_threshold) {
      host->setPstate(0); /* Run at max. performance! */
      XBT_INFO("Changed to pstate %f", 0.0);
    } else {
      /* The actual implementation uses a formula here: (See Kernel file cpufreq_ondemand.c:158)
       *
       *    freq_next = min_f + load * (max_f - min_f) / 100;
       *
       * So they assume that frequency increases by 100 MHz. We will just use
       * lowest_pstate - load*pstatesCount();
       */
      int max_pstate = host->getPstatesCount() - 1;

      host->setPstate(max_pstate - load * max_pstate);
      XBT_INFO("Changed to pstate %f -- check: %i", max_pstate - load * max_pstate, host->getPstate());
    }
  }
};

class Conservative : public Governor {
  double freq_up_threshold   = .8;
  double freq_down_threshold = .2;

public:
  explicit Conservative(simgrid::s4u::Host* ptr) : Governor(ptr) {}

  void update()
  {
    double load = sg_host_get_current_load(host);
    int pstate  = host->getPstate();

    if (load > freq_up_threshold) {
      if (pstate != 0) {
        host->setPstate(pstate - 1);
        XBT_INFO("Increasing performance to pstate %d", pstate - 1);
      }
      else {
        XBT_DEBUG("Cannot speed up even more, already in slowest pstate %d", pstate);
      }
    }

    if (load < freq_down_threshold) {
      int max_pstate = host->getPstatesCount() - 1;
      if (pstate != max_pstate) { // Are we in the slowest pstate already?
        host->setPstate(pstate + 1);
        XBT_INFO("Slowing down to pstate %d", pstate + 1);
      }
      else {
        XBT_DEBUG("Cannot slow down even more, already in slowest pstate %d", pstate);
      }
    }
  }
};
}

class HostDvfs {
public:
  static simgrid::xbt::Extension<simgrid::s4u::Host, HostDvfs> EXTENSION_ID;

  explicit HostDvfs(simgrid::s4u::Host*);
  ~HostDvfs();
};

simgrid::xbt::Extension<simgrid::s4u::Host, HostDvfs> HostDvfs::EXTENSION_ID;

HostDvfs::HostDvfs(simgrid::s4u::Host* ptr) {}

HostDvfs::~HostDvfs() = default;
}
}

using simgrid::plugin::HostDvfs;

/* **************************** events  callback *************************** */
static void on_host_added(simgrid::s4u::Host& host)
{
  if (dynamic_cast<simgrid::s4u::VirtualMachine*>(&host)) // Ignore virtual machines
    return;

  std::string name              = std::string("dvfs-daemon-") + host.getCname();
  simgrid::s4u::ActorPtr daemon = simgrid::s4u::Actor::createActor(name.c_str(), &host, []() {
    /**
     * This lambda function is the function the actor (daemon) will execute
     * all the time - in the case of the dvfs plugin, this controls when to
     * lower/raise the frequency.
     */
    simgrid::s4u::ActorPtr daemonProc = simgrid::s4u::Actor::self();

    XBT_DEBUG("DVFS process on %s is a daemon: %d", daemonProc->getHost()->getName().c_str(), daemonProc->isDaemon());

    std::string dvfs_governor;
    const char* host_conf = daemonProc->getHost()->getProperty("plugin/dvfs/governor");
    if (host_conf != nullptr) {
      dvfs_governor = std::string(daemonProc->getHost()->getProperty("plugin/dvfs/governor"));
      boost::algorithm::to_lower(dvfs_governor);
    } else {
      dvfs_governor = xbt_cfg_get_string("plugin/dvfs/governor");
      boost::algorithm::to_lower(dvfs_governor);
    }

    simgrid::plugin::dvfs::Governor governor(daemonProc->getHost());
    if (dvfs_governor == "conservative") {
      governor = simgrid::plugin::dvfs::Conservative(daemonProc->getHost());
    }

    while (1) {
      // Sleep *before* updating; important for startup (i.e., t = 0).
      // In the beginning, we want to go with the pstates specified in the platform file
      // (so we sleep first)
      simgrid::s4u::this_actor::sleep_for(governor.samplingRate());
      governor.update();
      XBT_INFO("Governor just updated!");
    }

    XBT_WARN("I should have never reached this point: daemons should be killed when all regular processes are done");
    return 0;
  });

  // This call must be placed in this function. Otherweise, the daemonize() call comes too late and
  // SMPI will take this process as an MPI process!
  daemon->daemonize();
}

/* **************************** Public interface *************************** */
extern "C" {

/** \ingroup SURF_plugin_load
 * \brief Initializes the HostDvfs plugin
 * \details The HostDvfs plugin provides an API to get the current load of each host.
 */
void sg_host_dvfs_plugin_init()
{
  if (HostDvfs::EXTENSION_ID.valid())
    return;

  HostDvfs::EXTENSION_ID = simgrid::s4u::Host::extension_create<HostDvfs>();

  sg_host_load_plugin_init();

  simgrid::s4u::Host::onCreation.connect(&on_host_added);
  xbt_cfg_register_double("plugin/dvfs/sampling_rate", 0.1, nullptr,
                          "How often should the dvfs plugin check whether the frequency needs to be changed?");
  xbt_cfg_register_string("plugin/dvfs/governor", "performance", nullptr,
                          "Which Governor should be used that adapts the CPU frequency?");
}
}
