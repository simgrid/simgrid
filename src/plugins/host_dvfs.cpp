/* Copyright (c) 2010-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/dvfs.h"
#include "simgrid/plugins/load.h"
#include "simgrid/s4u/Engine.hpp"
#include "src/internal_config.h" // HAVE_SMPI
#include "src/kernel/activity/ExecImpl.hpp"
#include "src/plugins/vm/VirtualMachineImpl.hpp"
#if HAVE_SMPI
#include "src/smpi/plugins/ampi/ampi.hpp"
#endif
#include <xbt/config.hpp>

#include <boost/algorithm/string.hpp>
#if HAVE_SMPI
#include "src/smpi/include/smpi_request.hpp"
#endif

SIMGRID_REGISTER_PLUGIN(host_dvfs, "Dvfs support", &sg_host_dvfs_plugin_init)

static simgrid::config::Flag<double> cfg_sampling_rate("plugin/dvfs/sampling-rate", {"plugin/dvfs/sampling_rate"},
    "How often should the dvfs plugin check whether the frequency needs to be changed?", 0.1,
    [](double val){if (val != 0.1) sg_host_dvfs_plugin_init();});

static simgrid::config::Flag<std::string> cfg_governor("plugin/dvfs/governor",
                                                       "Which Governor should be used that adapts the CPU frequency?",
                                                       "performance",

                                                       std::map<std::string, std::string>({
#if HAVE_SMPI
                                                         {"adagio", "TODO: Doc"},
#endif
                                                             {"conservative", "TODO: Doc"}, {"ondemand", "TODO: Doc"},
                                                             {"performance", "TODO: Doc"}, {"powersave", "TODO: Doc"},
                                                       }),

                                                       [](const std::string& val) {
                                                         if (val != "performance")
                                                           sg_host_dvfs_plugin_init();
                                                       });

static simgrid::config::Flag<int>
    cfg_min_pstate("plugin/dvfs/min-pstate", {"plugin/dvfs/min_pstate"},
                   "Which pstate is the minimum (and hence fastest) pstate for this governor?", 0);

static const int max_pstate_not_limited = -1;
static simgrid::config::Flag<int>
    cfg_max_pstate("plugin/dvfs/max-pstate", {"plugin/dvfs/max_pstate"},
                   "Which pstate is the maximum (and hence slowest) pstate for this governor?", max_pstate_not_limited);

/** @addtogroup SURF_plugin_load

  This plugin makes it very simple for users to obtain the current load for each host.

*/

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_plugin_dvfs, surf, "Logging specific to the SURF HostDvfs plugin");

namespace simgrid {
namespace plugin {

namespace dvfs {

/**
 *  Add this to your host tag:
 *    - \<prop id="plugin/dvfs/governor" value="performance" /\>
 *
 *  Valid values as of now are: performance, powersave, ondemand, conservative
 *  It doesn't matter if you use uppercase or lowercase.
 *
 *  For the sampling rate, use this:
 *
 *    - \<prop id="plugin/dvfs/sampling-rate" value="2" /\>
 *
 *  This will run the update() method of the specified governor every 2 seconds
 *  on that host.
 *
 *  These properties can also be used within the \<config\> tag to configure
 *  these values globally. Using them within the \<host\> will overwrite this
 *  global configuration
 */
class Governor {
  simgrid::s4u::Host* const host_;
  double sampling_rate_;
  int min_pstate; //< Never use a pstate less than this one
  int max_pstate; //< Never use a pstate larger than this one

public:
  explicit Governor(simgrid::s4u::Host* ptr)
      : host_(ptr)
      , min_pstate(cfg_min_pstate)
      , max_pstate(cfg_max_pstate == max_pstate_not_limited ? host_->get_pstate_count() - 1 : cfg_max_pstate)
  {
    init();
  }
  virtual ~Governor() = default;
  virtual std::string get_name() const = 0;
  simgrid::s4u::Host* get_host() const { return host_; }
  int get_min_pstate() const { return min_pstate; }
  int get_max_pstate() const { return max_pstate; }

  void init()
  {
    const char* local_sampling_rate_config = host_->get_property(cfg_sampling_rate.get_name());
    if (local_sampling_rate_config != nullptr) {
      sampling_rate_ = std::stod(local_sampling_rate_config);
    } else {
      sampling_rate_ = cfg_sampling_rate;
    }
    const char* local_min_pstate_config = host_->get_property(cfg_min_pstate.get_name());
    if (local_min_pstate_config != nullptr) {
      min_pstate = std::stoi(local_min_pstate_config);
    }

    const char* local_max_pstate_config = host_->get_property(cfg_max_pstate.get_name());
    if (local_max_pstate_config != nullptr) {
      max_pstate = std::stod(local_max_pstate_config);
    }
    xbt_assert(max_pstate <= host_->get_pstate_count() - 1, "Value for max_pstate too large!");
    xbt_assert(min_pstate <= max_pstate, "min_pstate is larger than max_pstate!");
    xbt_assert(0 <= min_pstate, "min_pstate is negative!");
  }

  virtual void update()         = 0;
  double get_sampling_rate() const { return sampling_rate_; }
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
  std::string get_name() const override { return "Performance"; }

  void update() override { get_host()->set_pstate(get_min_pstate()); }
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
  std::string get_name() const override { return "Powersave"; }

  void update() override { get_host()->set_pstate(get_max_pstate()); }
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
  double freq_up_threshold_ = 0.80;

public:
  explicit OnDemand(simgrid::s4u::Host* ptr) : Governor(ptr) {}
  std::string get_name() const override { return "OnDemand"; }

  void update() override
  {
    double load = get_host()->get_core_count() * sg_host_get_avg_load(get_host());
    sg_host_load_reset(get_host()); // Only consider the period between two calls to this method!

    if (load > freq_up_threshold_) {
      get_host()->set_pstate(get_min_pstate()); /* Run at max. performance! */
      XBT_INFO("Load: %f > threshold: %f --> changed to pstate %i", load, freq_up_threshold_, get_min_pstate());
    } else {
      /* The actual implementation uses a formula here: (See Kernel file cpufreq_ondemand.c:158)
       *
       *    freq_next = min_f + load * (max_f - min_f) / 100
       *
       * So they assume that frequency increases by 100 MHz. We will just use
       * lowest_pstate - load*pstatesCount()
       */
      // Load is now < freq_up_threshold; exclude pstate 0 (the fastest)
      // because pstate 0 can only be selected if load > freq_up_threshold_
      int new_pstate = get_max_pstate() - load * (get_max_pstate() + 1);
      if (new_pstate < get_min_pstate())
        new_pstate = get_min_pstate();
      get_host()->set_pstate(new_pstate);

      XBT_DEBUG("Load: %f < threshold: %f --> changed to pstate %i", load, freq_up_threshold_, new_pstate);
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
 * > differs in behavior in that it gracefully increases and decreases the
 * > CPU speed rather than jumping to max speed the moment there is any load
 * > on the CPU. This behavior is more suitable in a battery powered
 * > environment.
 */
class Conservative : public Governor {
  double freq_up_threshold_   = .8;
  double freq_down_threshold_ = .2;

public:
  explicit Conservative(simgrid::s4u::Host* ptr) : Governor(ptr) {}
  std::string get_name() const override { return "Conservative"; }

  void update() override
  {
    double load = get_host()->get_core_count() * sg_host_get_avg_load(get_host());
    int pstate  = get_host()->get_pstate();
    sg_host_load_reset(get_host()); // Only consider the period between two calls to this method!

    if (load > freq_up_threshold_) {
      if (pstate != get_min_pstate()) {
        get_host()->set_pstate(pstate - 1);
        XBT_INFO("Load: %f > threshold: %f -> increasing performance to pstate %d", load, freq_up_threshold_,
                 pstate - 1);
      } else {
        XBT_DEBUG("Load: %f > threshold: %f -> but cannot speed up even more, already in highest pstate %d", load,
                  freq_up_threshold_, pstate);
      }
    } else if (load < freq_down_threshold_) {
      if (pstate != get_max_pstate()) { // Are we in the slowest pstate already?
        get_host()->set_pstate(pstate + 1);
        XBT_INFO("Load: %f < threshold: %f -> slowing down to pstate %d", load, freq_down_threshold_, pstate + 1);
      } else {
        XBT_DEBUG("Load: %f < threshold: %f -> cannot slow down even more, already in slowest pstate %d", load,
                  freq_down_threshold_, pstate);
      }
    }
  }
};

#if HAVE_SMPI
class Adagio : public Governor {
private:
  int best_pstate     = 0;
  double start_time   = 0;
  double comp_counter = 0;
  double comp_timer   = 0;

  std::vector<std::vector<double>> rates; // Each host + all frequencies of that host

  unsigned int task_id   = 0;
  bool iteration_running = false; /*< Are we currently between iteration_in and iteration_out calls? */

public:
  explicit Adagio(simgrid::s4u::Host* ptr)
      : Governor(ptr), rates(100, std::vector<double>(ptr->get_pstate_count(), 0.0))
  {
    simgrid::smpi::plugin::ampi::on_iteration_in.connect([this](simgrid::s4u::Actor const& actor) {
      // Every instance of this class subscribes to this event, so one per host
      // This means that for any actor, all 'hosts' are normally notified of these
      // changes, even those who don't currently run the actor 'proc_id'.
      // -> Let's check if this signal call is for us!
      if (get_host() == actor.get_host()) {
        iteration_running = true;
      }
    });
    simgrid::smpi::plugin::ampi::on_iteration_out.connect([this](simgrid::s4u::Actor const& actor) {
      if (get_host() == actor.get_host()) {
        iteration_running = false;
        task_id           = 0;
      }
    });
    simgrid::s4u::Exec::on_start.connect([this](simgrid::s4u::Actor const&, simgrid::s4u::Exec const& activity) {
      if (activity.get_host() == get_host())
        pre_task();
    });
    simgrid::s4u::Exec::on_completion.connect([this](simgrid::s4u::Actor const&, simgrid::s4u::Exec const& activity) {
      // For more than one host (not yet supported), we can access the host via
      // simcalls_.front()->issuer->iface()->get_host()
      if (activity.get_host() == get_host() && iteration_running) {
        comp_timer += activity.get_finish_time() - activity.get_start_time();
      }
    });
    // FIXME I think that this fires at the same time for all hosts, so when the src sends something,
    // the dst will be notified even though it didn't even arrive at the recv yet
    simgrid::s4u::Link::on_communicate.connect(
        [this](kernel::resource::NetworkAction const&, s4u::Host* src, s4u::Host* dst) {
          if ((get_host() == src || get_host() == dst) && iteration_running) {
            post_task();
          }
        });
  }

  std::string get_name() const override { return "Adagio"; }

  void pre_task()
  {
    sg_host_load_reset(get_host());
    comp_counter = sg_host_get_computed_flops(get_host()); // Should be 0 because of the reset
    comp_timer   = 0;
    start_time   = simgrid::s4u::Engine::get_clock();
    if (rates.size() <= task_id)
      rates.resize(task_id + 5, std::vector<double>(get_host()->get_pstate_count(), 0.0));
    if (rates[task_id][best_pstate] == 0)
      best_pstate = 0;
    get_host()->set_pstate(best_pstate); // Load our schedule
    XBT_DEBUG("Set pstate to %i", best_pstate);
  }

  void post_task()
  {
    double computed_flops = sg_host_get_computed_flops(get_host()) - comp_counter;
    double target_time    = (simgrid::s4u::Engine::get_clock() - start_time);
    target_time           = target_time * 99.0 / 100.0; // FIXME We account for t_copy arbitrarily with 1%
                                                        // -- this needs to be fixed

    bool is_initialized         = rates[task_id][best_pstate] != 0;
    rates[task_id][best_pstate] = computed_flops / comp_timer;
    if (not is_initialized) {
      for (int i = 1; i < get_host()->get_pstate_count(); i++) {
        rates[task_id][i] = rates[task_id][0] * (get_host()->get_pstate_speed(i) / get_host()->get_speed());
      }
    }

    for (int pstate = get_host()->get_pstate_count() - 1; pstate >= 0; pstate--) {
      if (computed_flops / rates[task_id][pstate] <= target_time) {
        // We just found the pstate we want to use!
        best_pstate = pstate;
        break;
      }
    }
    task_id++;
  }

  void update() override {}
};
#endif
} // namespace dvfs
} // namespace plugin
} // namespace simgrid

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
    const char* host_conf = daemon_proc->get_host()->get_property("plugin/dvfs/governor");
    if (host_conf != nullptr) {
      dvfs_governor = std::string(host_conf);
      boost::algorithm::to_lower(dvfs_governor);
    } else {
      dvfs_governor = cfg_governor;
      boost::algorithm::to_lower(dvfs_governor);
    }

    auto governor = [&dvfs_governor, &daemon_proc]() {
      if (dvfs_governor == "conservative") {
        return std::unique_ptr<simgrid::plugin::dvfs::Governor>(
            new simgrid::plugin::dvfs::Conservative(daemon_proc->get_host()));
      } else if (dvfs_governor == "ondemand") {
        return std::unique_ptr<simgrid::plugin::dvfs::Governor>(
            new simgrid::plugin::dvfs::OnDemand(daemon_proc->get_host()));
      }
#if HAVE_SMPI
      else if (dvfs_governor == "adagio") {
        return std::unique_ptr<simgrid::plugin::dvfs::Governor>(
            new simgrid::plugin::dvfs::Adagio(daemon_proc->get_host()));
      }
#endif
      else if (dvfs_governor == "performance") {
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
      simgrid::s4u::this_actor::sleep_for(governor->get_sampling_rate());
      governor->update();
      XBT_DEBUG("Governor (%s) just updated!", governor->get_name().c_str());
    }

    XBT_WARN("I should have never reached this point: daemons should be killed when all regular processes are done");
    return 0;
  });

  // This call must be placed in this function. Otherwise, the daemonize() call comes too late and
  // SMPI will take this process as an MPI process!
  daemon->daemonize();
}

/* **************************** Public interface *************************** */

/**
 * @brief Initializes the HostDvfs plugin
 * @details The HostDvfs plugin provides an API to get the current load of each host.
 */
void sg_host_dvfs_plugin_init()
{
  static bool inited = false;
  if (inited)
    return;
  inited = true;

  sg_host_load_plugin_init();

  simgrid::s4u::Host::on_creation.connect(&on_host_added);
}
