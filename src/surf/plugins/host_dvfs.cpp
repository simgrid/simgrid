/* Copyright (c) 2010-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/dvfs.h"
#include "simgrid/plugins/load.h"
#include "src/plugins/vm/VirtualMachineImpl.hpp"
#include <xbt/config.hpp>

#include <boost/algorithm/string.hpp>

/** @addtogroup SURF_plugin_load

  This plugin makes it very simple for users to obtain the current load for each host.

*/

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_plugin_dvfs, surf, "Logging specific to the SURF HostDvfs plugin");

static const char* property_sampling_rate = "plugin/dvfs/sampling_rate";
static const char* property_governor      = "plugin/dvfs/governor";

namespace simgrid {
namespace plugin {

namespace dvfs {
class Governor {

private:
  simgrid::s4u::Host* const host_;

protected:
  simgrid::s4u::Host* get_host() const { return host_; }

public:
  double sampling_rate;

  explicit Governor(simgrid::s4u::Host* ptr) : host_(ptr) { init(); }
  virtual ~Governor() = default;

  void init()
  {
    const char* local_sampling_rate_config = host_->getProperty(property_sampling_rate);
    double global_sampling_rate_config     = simgrid::config::get_config<double>(property_sampling_rate);
    if (local_sampling_rate_config != nullptr) {
      sampling_rate = std::stod(local_sampling_rate_config);
    } else {
      sampling_rate = global_sampling_rate_config;
    }
  }

  virtual void update()         = 0;
  virtual std::string getName() = 0;
  double samplingRate() { return sampling_rate; }
};

/**
 * The linux kernel doc describes this governor as follows:
 * https://www.kernel.org/doc/Documentation/cpu-freq/governors.txt
 *
 * > The CPUfreq governor "performance" sets the CPU statically to the
 * > highest frequency within the borders of scaling_min_freq and
 * > scaling_max_freq.
 *
 * We do not support scaling_min_freq/scaling_max_freq -- we just pick the lowest frequency.
 */
class Performance : public Governor {
public:
  explicit Performance(simgrid::s4u::Host* ptr) : Governor(ptr) {}

  void update() override { get_host()->setPstate(0); }
  std::string getName() override { return "Performance"; }
};

/**
 * The linux kernel doc describes this governor as follows:
 * https://www.kernel.org/doc/Documentation/cpu-freq/governors.txt
 *
 * > The CPUfreq governor "powersave" sets the CPU statically to the
 * > lowest frequency within the borders of scaling_min_freq and
 * > scaling_max_freq.
 *
 * We do not support scaling_min_freq/scaling_max_freq -- we just pick the lowest frequency.
 */
class Powersave : public Governor {
public:
  explicit Powersave(simgrid::s4u::Host* ptr) : Governor(ptr) {}

  void update() override { get_host()->setPstate(get_host()->getPstatesCount() - 1); }
  std::string getName() override { return "Powersave"; }
};

/**
 * The linux kernel doc describes this governor as follows:
 * https://www.kernel.org/doc/Documentation/cpu-freq/governors.txt
 *
 * > The CPUfreq governor "ondemand" sets the CPU frequency depending on the
 * > current system load. [...] when triggered, cpufreq checks
 * > the CPU-usage statistics over the last period and the governor sets the
 * > CPU accordingly.
 */
class OnDemand : public Governor {
  /**
   * See https://elixir.bootlin.com/linux/v4.15.4/source/drivers/cpufreq/cpufreq_ondemand.c
   * DEF_FREQUENCY_UP_THRESHOLD and od_update()
   */
  double freq_up_threshold = 0.80;

public:
  explicit OnDemand(simgrid::s4u::Host* ptr) : Governor(ptr) {}

  std::string getName() override { return "OnDemand"; }
  void update() override
  {
    double load = get_host()->getCoreCount() * sg_host_get_avg_load(get_host());
    sg_host_load_reset(get_host()); // Only consider the period between two calls to this method!

    if (load > freq_up_threshold) {
      get_host()->setPstate(0); /* Run at max. performance! */
      XBT_INFO("Load: %f > threshold: %f --> changed to pstate %i", load, freq_up_threshold, 0);
    } else {
      /* The actual implementation uses a formula here: (See Kernel file cpufreq_ondemand.c:158)
       *
       *    freq_next = min_f + load * (max_f - min_f) / 100
       *
       * So they assume that frequency increases by 100 MHz. We will just use
       * lowest_pstate - load*pstatesCount()
       */
      int max_pstate = get_host()->getPstatesCount() - 1;
      // Load is now < freq_up_threshold; exclude pstate 0 (the fastest)
      // because pstate 0 can only be selected if load > freq_up_threshold
      int new_pstate = max_pstate - load * (max_pstate + 1);
      get_host()->setPstate(new_pstate);

      XBT_DEBUG("Load: %f < threshold: %f --> changed to pstate %i", load, freq_up_threshold, new_pstate);
    }
  }
};

/**
 * This is the conservative governor, which is very similar to the
 * OnDemand governor. The Linux Kernel Documentation describes it
 * very well, see https://www.kernel.org/doc/Documentation/cpu-freq/governors.txt:
 *
 * > The CPUfreq governor "conservative", much like the "ondemand"
 * > governor, sets the CPU frequency depending on the current usage.  It
 * > differs in behaviour in that it gracefully increases and decreases the
 * > CPU speed rather than jumping to max speed the moment there is any load
 * > on the CPU. This behaviour is more suitable in a battery powered
 * > environment.
 */
class Conservative : public Governor {
  double freq_up_threshold   = .8;
  double freq_down_threshold = .2;

public:
  explicit Conservative(simgrid::s4u::Host* ptr) : Governor(ptr) {}

  virtual std::string getName() override { return "Conservative"; }
  virtual void update() override
  {
    double load = get_host()->getCoreCount() * sg_host_get_avg_load(get_host());
    int pstate  = get_host()->getPstate();
    sg_host_load_reset(get_host()); // Only consider the period between two calls to this method!

    if (load > freq_up_threshold) {
      if (pstate != 0) {
        get_host()->setPstate(pstate - 1);
        XBT_INFO("Load: %f > threshold: %f -> increasing performance to pstate %d", load, freq_up_threshold, pstate - 1);
      }
      else {
        XBT_DEBUG("Load: %f > threshold: %f -> but cannot speed up even more, already in highest pstate %d", load, freq_up_threshold, pstate);
      }
    } else if (load < freq_down_threshold) {
      int max_pstate = get_host()->getPstatesCount() - 1;
      if (pstate != max_pstate) { // Are we in the slowest pstate already?
        get_host()->setPstate(pstate + 1);
        XBT_INFO("Load: %f < threshold: %f -> slowing down to pstate %d", load, freq_down_threshold, pstate + 1);
      }
      else {
        XBT_DEBUG("Load: %f < threshold: %f -> cannot slow down even more, already in slowest pstate %d", load, freq_down_threshold, pstate);
      }
    }
  }
};

/**
 *  Add this to your host tag:
 *    - <prop id="plugin/dvfs/governor" value="performance" />
 *
 *  Valid values as of now are: performance, powersave, ondemand, conservative
 *  It doesn't matter if you use uppercase or lowercase.
 *
 *  For the sampling rate, use this:
 *
 *    - <prop id="plugin/dvfs/sampling_rate" value="2" />
 *
 *  This will run the update() method of the specified governor every 2 seconds
 *  on that host.
 *
 *  These properties can also be used within the <config> tag to configure
 *  these values globally. Using them within the <host> will overwrite this
 *  global configuration
 */
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
}

using simgrid::plugin::dvfs::HostDvfs;

/* **************************** events  callback *************************** */
static void on_host_added(simgrid::s4u::Host& host)
{
  if (dynamic_cast<simgrid::s4u::VirtualMachine*>(&host)) // Ignore virtual machines
    return;

  std::string name              = std::string("dvfs-daemon-") + host.get_cname();
  simgrid::s4u::ActorPtr daemon = simgrid::s4u::Actor::create(name.c_str(), &host, []() {
    /**
     * This lambda function is the function the actor (daemon) will execute
     * all the time - in the case of the dvfs plugin, this controls when to
     * lower/raise the frequency.
     */
    simgrid::s4u::ActorPtr daemon_proc = simgrid::s4u::Actor::self();

    XBT_DEBUG("DVFS process on %s is a daemon: %d", daemon_proc->get_host()->get_cname(), daemon_proc->is_daemon());

    std::string dvfs_governor;
    const char* host_conf = daemon_proc->get_host()->getProperty(property_governor);
    if (host_conf != nullptr) {
      dvfs_governor = std::string(daemon_proc->get_host()->getProperty(property_governor));
      boost::algorithm::to_lower(dvfs_governor);
    } else {
      dvfs_governor = simgrid::config::get_config<std::string>(property_governor);
      boost::algorithm::to_lower(dvfs_governor);
    }

    auto governor = [&dvfs_governor, &daemon_proc]() {
      if (dvfs_governor == "conservative") {
        return std::unique_ptr<simgrid::plugin::dvfs::Governor>(
            new simgrid::plugin::dvfs::Conservative(daemon_proc->get_host()));
      } else if (dvfs_governor == "ondemand") {
        return std::unique_ptr<simgrid::plugin::dvfs::Governor>(
            new simgrid::plugin::dvfs::OnDemand(daemon_proc->get_host()));
      } else if (dvfs_governor == "performance") {
        return std::unique_ptr<simgrid::plugin::dvfs::Governor>(
            new simgrid::plugin::dvfs::Performance(daemon_proc->get_host()));
      } else if (dvfs_governor == "powersave") {
        return std::unique_ptr<simgrid::plugin::dvfs::Governor>(
            new simgrid::plugin::dvfs::Powersave(daemon_proc->get_host()));
      } else {
        XBT_CRITICAL("No governor specified for host %s, falling back to Performance",
                     daemon_proc->get_host()->get_cname());
        return std::unique_ptr<simgrid::plugin::dvfs::Governor>(
            new simgrid::plugin::dvfs::Performance(daemon_proc->get_host()));
      }
    }();

    while (1) {
      // Sleep *before* updating; important for startup (i.e., t = 0).
      // In the beginning, we want to go with the pstates specified in the platform file
      // (so we sleep first)
      simgrid::s4u::this_actor::sleep_for(governor->samplingRate());
      governor->update();
      XBT_DEBUG("Governor (%s) just updated!", governor->getName().c_str());
    }

    XBT_WARN("I should have never reached this point: daemons should be killed when all regular processes are done");
    return 0;
  });

  // This call must be placed in this function. Otherweise, the daemonize() call comes too late and
  // SMPI will take this process as an MPI process!
  daemon->daemonize();
}

/* **************************** Public interface *************************** */

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
  simgrid::config::declare_flag<double>(
      property_sampling_rate, "How often should the dvfs plugin check whether the frequency needs to be changed?", 0.1);
  simgrid::config::declare_flag<std::string>(
      property_governor, "Which Governor should be used that adapts the CPU frequency?", "performance");
}
