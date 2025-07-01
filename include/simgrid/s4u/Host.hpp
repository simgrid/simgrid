/* Copyright (c) 2006-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_HOST_HPP
#define SIMGRID_S4U_HOST_HPP

#include <simgrid/forward.h>
#include <simgrid/kernel/resource/Action.hpp>
#include <xbt/Extendable.hpp>
#include <xbt/signal.hpp>

#include <map>
#include <unordered_map>

namespace simgrid {

extern template class XBT_PUBLIC xbt::Extendable<s4u::Host>;

namespace s4u {
/** @ingroup s4u_api
 *
 * @tableofcontents
 *
 * Some physical resource with computing and networking capabilities on which Actors execute.
 *
 * @beginrst
 * All hosts are automatically created during the call of the method
 * :cpp:func:`simgrid::s4u::Engine::load_platform()`.
 * You cannot create a host yourself.
 *
 * You can retrieve a particular host using :cpp:func:`simgrid::s4u::Host::by_name()`
 * and actors can retrieve the host on which they run using :cpp:func:`simgrid::s4u::Host::current()` or
 * :cpp:func:`simgrid::s4u::this_actor::get_host()`
 * @endrst
 */
class XBT_PUBLIC Host : public xbt::Extendable<Host> {

#ifndef DOXYGEN
  friend kernel::resource::VMModel;            // Use the pimpl_cpu to compute the VM sharing
  friend kernel::resource::VirtualMachineImpl; // creates the the pimpl_cpu
  friend kernel::routing::NetZoneImpl;
  friend kernel::resource::HostImpl; // call destructor from private implementation
  friend kernel::resource::CpuAction; // signal exec_state_changed

  // The private implementation, that never changes
  kernel::resource::HostImpl* const pimpl_;

  kernel::resource::CpuImpl* pimpl_cpu_      = nullptr;
  kernel::routing::NetPoint* pimpl_netpoint_ = nullptr;

  static xbt::signal<void(Host&)> on_creation;
  static xbt::signal<void(Host const&)> on_destruction;
  xbt::signal<void(Host const&)> on_this_destruction;
  static xbt::signal<void(kernel::resource::CpuAction&, kernel::resource::Action::State)> on_exec_state_change;

public:
  explicit Host(kernel::resource::HostImpl* pimpl) : pimpl_(pimpl) {}

protected:
  virtual ~Host(); // Call destroy() instead of manually deleting it.
  Host* set_netpoint(kernel::routing::NetPoint* netpoint);

public:
  static xbt::signal<void(Host const&)> on_speed_change;
  xbt::signal<void(Host const&)> on_this_speed_change;
  static xbt::signal<void(Host const&)> on_onoff;
  xbt::signal<void(Host const&)> on_this_onoff;
#endif

  /** \static Add a callback fired on each newly created host */
  static void on_creation_cb(const std::function<void(Host&)>& cb) { on_creation.connect(cb); }
  /** \static Add a callback fired when any machine is turned on or off (called AFTER the change) */
  static void on_onoff_cb(const std::function<void(Host const&)>& cb) { on_onoff.connect(cb); }
  /** Add a callback fired when this specific machine is turned on or off (called AFTER the change) */
  void on_this_onoff_cb(const std::function<void(Host const&)>& cb)
  {
    on_this_onoff.connect(cb);
  }
  /** \static Add a callback fired when the speed of any machine is changed (called AFTER the change)
   * (either because of a pstate switch or because of an external load event coming from the profile) */
  static void on_speed_change_cb(const std::function<void(Host const&)>& cb) { on_speed_change.connect(cb); }
  /** Add a callback fired when the speed of this specific machine is changed (called AFTER the change)
   * (either because of a pstate switch or because of an external load event coming from the profile) */
  void on_this_speed_change_cb(const std::function<void(Host const&)>& cb)
  {
    on_this_speed_change.connect(cb);
  }
  /** \static Add a callback fired just before destructing any host */
  static void on_destruction_cb(const std::function<void(Host const&)>& cb) { on_destruction.connect(cb); }
  /** Add a callback fired just before destructing this specific host */
  void on_this_destruction_cb(const std::function<void(Host const&)>& cb)
  {
    on_this_destruction.connect(cb);
  }
  /** \static Add a callback fired when the state of any exec activity changes */
  static void on_exec_state_change_cb(
      const std::function<void(kernel::resource::CpuAction&, kernel::resource::Action::State previous)>& cb)
  {
    on_exec_state_change.connect(cb);
  }

  virtual void destroy();
#ifndef DOXYGEN
  // No copy/move
  Host(Host const&) = delete;
  Host& operator=(Host const&) = delete;
#endif

  /** \static Retrieve a host from its name, or return nullptr */
  static Host* by_name_or_null(const std::string& name);
  /** \static Retrieve a host from its name, or die */
  static Host* by_name(const std::string& name);
  /** \static Retrieves the host on which the running actor is located */
  static Host* current();

  /** Retrieves the name of that host as a C++ string */
  std::string const& get_name() const;
  /** Retrieves the name of that host as a C string */
  const char* get_cname() const;

  Host* set_cpu(kernel::resource::CpuImpl* cpu);
  kernel::resource::CpuImpl* get_cpu() const { return pimpl_cpu_; }
  kernel::routing::NetPoint* get_netpoint() const { return pimpl_netpoint_; }

  /** Configure the factor callback to the CPU associated to this host
   *
   * This callback takes the execution size in flops and returns the multiply factor
   */
  Host* set_cpu_factor_cb(const std::function<double(Host&, double)>& cb);

  size_t get_actor_count() const;
  std::vector<ActorPtr> get_all_actors() const;

  /** Turns that host on if it was previously off
   *
   * This call does nothing if the host is already on. If it was off, all actors which were marked 'autorestart' on that
   * host will be restarted automatically (note that this may differ from the actors that were initially running on the
   * host).
   *
   * All other Host's properties are left unchanged; in particular, the pstate is left unchanged and not reset to its
   * initial value.
   */
  void turn_on();
  /** Turns that host off. All actors are forcefully stopped. */
  void turn_off();
  /** Returns if that host is currently up and running */
  bool is_on() const;

  const char* get_property(const std::string& key) const;
  Host* set_property(const std::string& key, const std::string& value);
  const std::unordered_map<std::string, std::string>* get_properties() const;
  Host* set_properties(const std::unordered_map<std::string, std::string>& properties);

  Host* set_state_profile(kernel::profile::Profile* p);
  Host* set_speed_profile(kernel::profile::Profile* p);

  /**
   * @brief Set the max amount of executions that can take place on this host at the same time
   *
   * Use -1 to set no limit.
   *
   * @param limit  Number of concurrent executions
   */
  Host* set_concurrency_limit(int limit);
  int get_concurrency_limit() const;

  /** \static @brief Convert the CPU's speed from string to double */
  static std::vector<double> convert_pstate_speed_vector(const std::vector<std::string>& speed_per_state);
  /**
   * @brief Set the CPU's speed
   *
   * @param speed_per_state list of powers for this processor (default power is at index 0)
   */
  Host* set_pstate_speed(const std::vector<double>& speed_per_state);
  /**
   * @brief Set the CPU's speed (string version)
   *
   * @throw std::invalid_argument if speed format is incorrect.
   */
  Host* set_pstate_speed(const std::vector<std::string>& speed_per_state);

  /** @brief Get the peak computing speed in flops/s at the current pstate, NOT taking the external load into account.
   *
   *  The amount of flops per second available for computing depends on several things:
   *    - The current pstate determines the maximal peak computing speed (use
   *      @verbatim embed:rst:inline :cpp:func:`get_pstate_speed() <simgrid::s4u::Host::get_pstate_speed>` @endverbatim
   *      to retrieve the computing speed you would get at another pstate)
   *    - If you declared an external load (with
   *      @verbatim embed:rst:inline :cpp:func:`set_speed_profile() <simgrid::s4u::Host::set_speed_profile>` @endverbatim ),
   *      you must multiply the result of
   *      @verbatim embed:rst:inline :cpp:func:`get_speed() <simgrid::s4u::Host::get_speed>` @endverbatim by
   *      @verbatim embed:rst:inline :cpp:func:`get_available_speed() <simgrid::s4u::Host::get_available_speed>` @endverbatim
   *      to retrieve what a new computation would get.
   *
   *  The remaining speed is then shared between the executions located on this host.
   *  You can retrieve the amount of tasks currently running on this host with
   *  @verbatim embed:rst:inline :cpp:func:`get_load() <simgrid::s4u::Host::get_load>` @endverbatim .
   *
   *  The host may have multiple cores, and your executions may be able to use more than a single core.
   *
   *  Finally, executions of priority 2 get twice the amount of flops than executions of priority 1.
   */
  double get_speed() const;
  /** @brief Get the available speed ratio, between 0 and 1.
   *
   * This accounts for external load (see
   * @verbatim embed:rst:inline :cpp:func:`set_speed_profile() <simgrid::s4u::Host::set_speed_profile>` @endverbatim ).
   */
  double get_available_speed() const;

  /** Returns the number of core of the processor. */
  int get_core_count() const;
  Host* set_core_count(int core_count);

  enum class SharingPolicy { NONLINEAR = 1, LINEAR = 0 };
  /**
   * @brief Describes how the CPU is shared between concurrent tasks
   *
   * Note that the NONLINEAR callback is in the critical path of the solver, so it should be fast.
   *
   * @param policy Sharing policy
   * @param cb Callback for NONLINEAR policies
   */
  Host* set_sharing_policy(SharingPolicy policy, const s4u::NonLinearResourceCb& cb = {});
  SharingPolicy get_sharing_policy() const;

  /** Returns the current computation load (in flops per second)
   *
   * The external load (coming from an availability trace) is not taken in account.
   * You may also be interested in the load plugin.
   */
  double get_load() const;

  unsigned long get_pstate_count() const;
  unsigned long get_pstate() const;
  double get_pstate_speed(unsigned long pstate_index) const;
  Host* set_pstate(unsigned long pstate_index);
  Host* set_coordinates(const std::string& coords);

  /** Create an actor from a @c std::function<void()>.
   *  If the actor is restarted, it gets a fresh copy of the function.
   *  @verbatim embed:rst:inline See the :ref:`example <s4u_ex_actors_create>`. @endverbatim */
  ActorPtr add_actor(const std::string& name, const std::function<void()>& code);

  /** Add an actor taking a vector of strings as parameters.
   *  @verbatim embed:rst:inline See the :ref:`example <s4u_ex_actors_create>`. @endverbatim */
  ActorPtr add_actor(const std::string& name, const std::string& function,
                     std::vector<std::string> args);
  /** Create an actor from a callable thing.
   *  @verbatim embed:rst:inline See the :ref:`example <s4u_ex_actors_create>`. @endverbatim */
  template <class F> ActorPtr add_actor(const std::string& name, F code)
  {
    return add_actor(name, std::function<void()>(std::move(code)));
  }

  /** Create an actor using a callable thing and its arguments.
   *
   * Note that the arguments will be copied, so move-only parameters are forbidden.
   * @verbatim embed:rst:inline See the :ref:`example <s4u_ex_actors_create>`. @endverbatim */
  template <class F, class... Args // This constructor is enabled only if calling code(args...) is valid
#ifndef DOXYGEN /* breathe seem to choke on function signatures in template parameter, see breathe#611 */
            ,
            typename = typename std::invoke_result_t<F, Args...>
#endif
            >
  ActorPtr add_actor(const std::string& name, F code, Args... args)
  {
    return add_actor(name, std::bind(std::move(code), std::move(args)...));
  }

  std::vector<Disk*> get_disks() const;
  Disk* get_disk_by_name(const std::string& name) const;

  #ifndef DOXYGEN
   XBT_ATTRIB_DEPRECATED_v403("Please use Host::add_disk")
   Disk* create_disk(const std::string& name, double read_bandwidth, double write_bandwidth)
   { return add_disk(name, read_bandwidth, write_bandwidth); }
   XBT_ATTRIB_DEPRECATED_v403("Please use Host::add_disk")
   Disk* create_disk(const std::string& name, const std::string& read_bandwidth, const std::string& write_bandwidth)
   { return add_disk(name, read_bandwidth, write_bandwidth); }
  #endif 

   /**
   * @brief Add a disk to a host
   *
   * @param name Disk name
   * @param read_bandwidth Reading speed of the disk
   * @param write_bandwidth Writing speed of the disk
   */
  Disk* add_disk(const std::string& name, double read_bandwidth, double write_bandwidth);
  /**
   * @brief Human-friendly version of add_disk function.
   *
   * @throw std::invalid_argument if read/write speeds are incorrect
   */
  Disk* add_disk(const std::string& name, const std::string& read_bandwidth, const std::string& write_bandwidth);
  void register_disk(const Disk* disk);
  void remove_disk(const std::string& disk_name);

  VirtualMachine* create_vm(const std::string& name, int core_amount);
  VirtualMachine* create_vm(const std::string& name, int core_amount, size_t ramsize);
  /** Retrieve a VM running on this host from its name, or return nullptr */
  VirtualMachine* vm_by_name_or_null(const std::string& name);

  void route_to(const Host* dest, std::vector<Link*>& links, double* latency) const;
  void route_to(const Host* dest, std::vector<kernel::resource::StandardLinkImpl*>& links, double* latency) const;
  std::pair<std::vector<Link*>, double> route_to(const Host* dest) const;

  /**
   * @brief Seal this host
   * No more configuration is allowed after the seal
   */
  Host* seal();

  NetZone* get_englobing_zone() const;
  /** Block the calling actor on an execution located on the called host
   *
   * It is not a problem if the actor is not located on the called host.
   * The actor will not be migrated in this case. Such remote execution are easy in simulation.
   */
  void execute(double flops) const;
  /** Start an asynchronous computation on that host (possibly remote) */
  ExecPtr exec_init(double flops_amounts) const;
  ExecPtr exec_async(double flops_amounts) const;

  /** Block the calling actor on an execution located on the called host (with explicit priority) */
  void execute(double flops, double priority) const;
  kernel::resource::HostImpl* get_impl() const { return pimpl_; }
};
} // namespace s4u
} // namespace simgrid

#endif /* SIMGRID_S4U_HOST_HPP */
