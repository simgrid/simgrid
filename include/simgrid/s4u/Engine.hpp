/* Copyright (c) 2006-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_ENGINE_HPP
#define SIMGRID_S4U_ENGINE_HPP

#include <xbt/base.h>

#include <simgrid/forward.h>

#include <simgrid/kernel/resource/Model.hpp>
#include <simgrid/s4u/NetZone.hpp>

#include <set>
#include <string>
#include <utility>
#include <vector>

namespace simgrid::s4u {
/** @brief Simulation engine
 *
 * This is a singleton containing all the main functions of the simulation.
 */
class XBT_PUBLIC Engine {
#ifndef DOXYGEN
  friend simgrid::kernel::EngineImpl;
#endif

public:
  /** Constructor, taking only the name of your main function */
  explicit Engine(std::string name);
  /** Constructor, taking the command line parameters of your main function */
  explicit Engine(int* argc, char** argv);

#ifndef DOXYGEN
  /* Currently, only one instance is allowed to exist. This is why you can't copy or move it */
  Engine(const Engine&) = delete;
  Engine(Engine&&)      = delete;
  ~Engine();
#endif

  /** Run the simulation until its end */
  void run() const;

  /** Run the simulation until the specified date */
  void run_until(double max_date) const;

  /** @brief Retrieve the simulation time (in seconds) */
  static double get_clock();
  static void papi_start();
  static void papi_stop();
  static int papi_get_num_counters();
  static std::vector<long long> get_papi_counters();
  /** @brief Retrieve the engine singleton */
  static s4u::Engine* get_instance();
  static s4u::Engine* get_instance(int* argc, char** argv);
  static bool has_instance() { return instance_ != nullptr && not shutdown_ongoing_; }
  const std::vector<std::string>& get_cmdline() const;
  const char* get_context_factory_name() const;

  /**
   * Creates a new platform, including hosts, links, and the routing table.
   *
   * @beginrst
   * See also: :ref:`platform`.
   * @endrst
   */
  void load_platform(const std::string& platf) const;
  /**
   * @brief Seals the platform, finishing the creation of its resources.
   *
   * This method is optional. The seal() is done automatically when you call Engine::run.
   */
  void seal_platform() const;
  /** @brief Get a debug output of the platform.
   *
   * It looks like a XML platform file, but it may be very different from the input platform file: All netzones are
   * flatified into a unique zone. This representation is mostly useful to debug your platform configuration and ensure
   * that your assumptions over your configuration hold. This enables you to verify the exact list of links traversed
   * between any two hosts, and the characteristics of every host and link. But you should not use the resulting file as
   * an input platform file: it is very verbose, and thus much less efficient (in parsing time and runtime performance)
   * than a regular platform file with the sufficient amount of intermediary netzones. Even if you use one zone only,
   * specialized zones (such as clusters) are more efficient than the one with fully explicit routing used here.
   */
  std::string flatify_platform() const;

  /** @verbatim embed:rst:inline Bind an actor name that could be found in :ref:`pf_tag_actor` tag to a function taking classical argc/argv parameters. See the :ref:`example <s4u_ex_actors_create>`. @endverbatim */
  void register_function(const std::string& name, const std::function<void(int, char**)>& code);
  /** @verbatim embed:rst:inline Bind an actor name that could be found in :ref:`pf_tag_actor` tag to a function taking a vector of strings as a parameter. See the :ref:`example <s4u_ex_actors_create>`. @endverbatim */
  void register_function(const std::string& name, const std::function<void(std::vector<std::string>)>& code);
  void register_function(const std::string& name, const kernel::actor::ActorCodeFactory& factory);

  /** @verbatim embed:rst:inline Provide a default function to be used when the name used in a :ref:`pf_tag_actor` tag was not binded with ``register_function`` nor ``register_actor``. @endverbatim */
  void register_default(const std::function<void(int, char**)>& code);
  void register_default(const kernel::actor::ActorCodeFactory& factory);

  /** @verbatim embed:rst:inline Bind an actor name that could be found in :ref:`pf_tag_actor` tag to a class name passed as a template parameter. See the :ref:`example <s4u_ex_actors_create>`. @endverbatim */
  template <class F> void register_actor(const std::string& name)
  {
    kernel::actor::ActorCodeFactory code_factory = [](std::vector<std::string> args_factory) {
      return kernel::actor::ActorCode([args = std::move(args_factory)]() mutable {
        F code(std::move(args));
        code();
      });
    };
    register_function(name, code_factory);
  }
  /** @verbatim embed:rst:inline Bind an actor name that could be found in :ref:`pf_tag_actor` tag to a function name passed as a parameter. See the :ref:`example <s4u_ex_actors_create>`. @endverbatim */
  template <class F> void register_actor(const std::string& name, F code)
  {
    kernel::actor::ActorCodeFactory code_factory = [code](std::vector<std::string> args_factory) {
      return kernel::actor::ActorCode([code, args = std::move(args_factory)]() mutable { code(std::move(args)); });
    };
    register_function(name, code_factory);
  }

  /** If non-null, the provided set will be filled with all activities that fail to start because of a veto */
  void track_vetoed_activities(std::set<Activity*>* vetoed_activities) const;

  /** @verbatim embed:rst:inline Load a deployment file. See:ref:`deploy` and the :ref:`example <s4u_ex_actors_create>`. @endverbatim */
  void load_deployment(const std::string& deploy) const;

protected:
#ifndef DOXYGEN
  friend Host;
  friend Link;
  friend Disk;
  friend kernel::routing::NetPoint;
  friend kernel::routing::NetZoneImpl;
  friend kernel::resource::HostImpl;
  friend kernel::resource::StandardLinkImpl;
  void netpoint_register(simgrid::kernel::routing::NetPoint* card);
  void netpoint_unregister(simgrid::kernel::routing::NetPoint* card);
  XBT_ATTRIB_DEPRECATED_v403("The root netzone was already created for you. Please use one of the "
                             "Netzone::add_netzone_*() method instead") void set_netzone_root(const NetZone* netzone);
#endif /*DOXYGEN*/

public:
  /** Returns the amount of hosts existing in the platform. */
  size_t get_host_count() const;
  /** Returns a vector of all hosts found in the platform.
   *
   * The order is generally different from the creation/declaration order in the XML platform because we use a hash
   * table internally.
   */
  std::vector<s4u::Host*> get_all_hosts() const;
  std::vector<s4u::Host*> get_filtered_hosts(const std::function<bool(s4u::Host*)>& filter) const;
  std::vector<s4u::Host*> get_hosts_from_MPI_hostfile(const std::string& hostfile) const;
  s4u::Host* host_by_name(const std::string& name) const;
  s4u::Host* host_by_name_or_null(const std::string& name) const;

  size_t get_link_count() const;
  std::vector<s4u::Link*> get_all_links() const;
  std::vector<s4u::Link*> get_filtered_links(const std::function<bool(s4u::Link*)>& filter) const;
  s4u::Link* link_by_name(const std::string& name) const;
  /**
   * @brief Find a split-duplex link from its name.
   * @throw std::invalid_argument if the searched link does not exist.
   */
  SplitDuplexLink* split_duplex_link_by_name(const std::string& name) const;
  s4u::Link* link_by_name_or_null(const std::string& name) const;

  s4u::Mailbox* mailbox_by_name_or_create(const std::string& name) const;
  s4u::MessageQueue* message_queue_by_name_or_create(const std::string& name) const;

  size_t get_actor_count() const;
  aid_t get_actor_max_pid() const;
  std::vector<ActorPtr> get_all_actors() const;
  /* Display the status of all actors on the standard error stream */
  void display_all_actors_status() const;
  std::vector<ActorPtr> get_filtered_actors(const std::function<bool(ActorPtr)>& filter) const;

  std::vector<kernel::routing::NetPoint*> get_all_netpoints() const;
  kernel::routing::NetPoint* netpoint_by_name_or_null(const std::string& name) const;
  /**
   * @brief Get netpoint by its name
   *
   * @param name Netpoint name
   * @throw std::invalid_argument if netpoint doesn't exist
   */
  kernel::routing::NetPoint* netpoint_by_name(const std::string& name) const;

  s4u::NetZone* get_netzone_root() const;
  std::vector<s4u::NetZone*> get_all_netzones() const;

  s4u::NetZone* netzone_by_name_or_null(const std::string& name) const;

  /** @brief Retrieves all netzones of the type indicated by the template argument */
  template <class T> std::vector<T*> get_filtered_netzones() const
  {
    static_assert(std::is_base_of_v<kernel::routing::NetZoneImpl, T>,
                  "Filtering netzones is only possible for subclasses of kernel::routing::NetZoneImpl");
    std::vector<T*> res;
    get_filtered_netzones_recursive(get_netzone_root(), &res);
    return res;
  }

  /**
   * @brief Add a model to engine list
   *
   * @param model        Pointer to model
   * @param dependencies List of dependencies for this model (optional)
   */
  void add_model(std::shared_ptr<simgrid::kernel::resource::Model> model,
                 const std::vector<kernel::resource::Model*>& dependencies = {});

  /** Get list of all models managed by this engine */
  const std::vector<simgrid::kernel::resource::Model*>& get_all_models() const;

  /** Create an actor from a @c std::function<void()>.
   *  If the actor is restarted, it gets a fresh copy of the function.
   *  @verbatim embed:rst:inline See the :ref:`example <s4u_ex_actors_create>`. @endverbatim */
  XBT_ATTRIB_DEPRECATED_v403("Please use Host::add_actor.")
  static ActorPtr add_actor(const std::string& name, s4u::Host* host, const std::function<void()>& code);

  /** Add an actor taking a vector of strings as parameters.
   *  @verbatim embed:rst:inline See the :ref:`example <s4u_ex_actors_create>`. @endverbatim */
  XBT_ATTRIB_DEPRECATED_v403("Please use Host::add_actor.")
  ActorPtr add_actor(const std::string& name, s4u::Host* host, const std::string& function,
                     std::vector<std::string> args);
  /** Create an actor from a callable thing.
   *  @verbatim embed:rst:inline See the :ref:`example <s4u_ex_actors_create>`. @endverbatim */
  
  template <class F> 
  XBT_ATTRIB_DEPRECATED_v403("Please use Host::add_actor.")
  ActorPtr add_actor(const std::string& name, s4u::Host* host, F code)
  {
    return host->add_actor(name, std::function<void()>(std::move(code)));
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
  XBT_ATTRIB_DEPRECATED_v403("Please use Host::add_actor.")
  ActorPtr add_actor(const std::string& name, s4u::Host* host, F code, Args... args)
  {
    return host->add_actor(name, std::bind(std::move(code), std::move(args)...));
  }

  kernel::EngineImpl* get_impl() const { return pimpl_; }
  /** Returns whether SimGrid was initialized yet -- mostly for internal use */
  static bool is_initialized();
  /** @brief set a configuration variable
   *
   * @beginrst
   * Do --help on any SimGrid binary to see the list of currently existing configuration variables
   * (see also :ref:`options`).
   * @endrst
   *
   * Example:
   * simgrid::s4u::Engine::set_config("host/model:ptask_L07");
   */
  static void set_config(const std::string& str);
  static void set_config(const std::string& name, int value);
  static void set_config(const std::string& name, bool value);
  static void set_config(const std::string& name, double value);
  static void set_config(const std::string& name, const std::string& value);

  Engine*
  set_default_comm_data_copy_callback(const std::function<void(kernel::activity::CommImpl*, void*, size_t)>& callback);

  /** Add a callback fired when the platform is created (ie, the xml file parsed). */
  static void on_platform_created_cb(const std::function<void()>& cb) { on_platform_created.connect(cb); }
  /** Add a callback fired when the platform is about to be created
   * (ie, after any configuration change and just before the resource creation) */
  static void on_platform_creation_cb(const std::function<void()>& cb) { on_platform_creation.connect(cb); }
  /** Add a callback fired when the main simulation loop starts, at the beginning of the first call to Engine::run() */
  static void on_simulation_start_cb(const std::function<void()>& cb) { on_simulation_start.connect(cb); }
  /** Add a callback fired when the main simulation loop ends, just before the end of Engine::run() */
  static void on_simulation_end_cb(const std::function<void()>& cb) { on_simulation_end.connect(cb); }

  /** Add a callback fired when the time jumps into the future.
   *
   * It is fired right after the time change (use get_clock() to get the new timestamp).
   * The callback parameter is the time delta since previous timestamp. */
  static void on_time_advance_cb(const std::function<void(double)>& cb) { on_time_advance.connect(cb); }

  /** Add a callback fired when the time cannot advance because of inter-actors deadlock. Note that the on_exit of each
   * actor is also executed on deadlock. */
  static void on_deadlock_cb(const std::function<void(void)>& cb) { on_deadlock.connect(cb); }

#ifndef DOXYGEN
  /* FIXME signals should be private */
  static xbt::signal<void()> on_platform_creation;
  static xbt::signal<void()> on_platform_created;
#endif

private:
  static xbt::signal<void()> on_simulation_start;
  static xbt::signal<void(double)> on_time_advance;
  static xbt::signal<void(void)> on_deadlock;
  static xbt::signal<void()> on_simulation_end;

  kernel::EngineImpl* const pimpl_;
  static Engine* instance_;
  static bool shutdown_ongoing_; // set to true just before the final shutdown, to break dependency loops in that area

  void initialize(int* argc, char** argv);
};

std::vector<ActivityPtr> create_DAG_from_dot(const std::string& filename);
std::vector<ActivityPtr> create_DAG_from_DAX(const std::string& filename);
std::vector<ActivityPtr> create_DAG_from_json(const std::string& filename);

#ifndef DOXYGEN /* Internal use only, no need to expose it */
template <class T>
XBT_PRIVATE void get_filtered_netzones_recursive(const s4u::NetZone* current, std::vector<T*>* whereto)
{
  static_assert(std::is_base_of_v<kernel::routing::NetZoneImpl, T>,
                "Filtering netzones is only possible for subclasses of kernel::routing::NetZoneImpl");
  for (auto const& elem : current->get_children()) {
    get_filtered_netzones_recursive(elem, whereto);
    auto* elem_impl = dynamic_cast<T*>(elem->get_impl());
    if (elem_impl != nullptr)
      whereto->push_back(elem_impl);
  }
}
#endif
} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_ENGINE_HPP */
