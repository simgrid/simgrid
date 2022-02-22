/* Copyright (c) 2006-2022. The SimGrid Team. All rights reserved.          */

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

namespace simgrid {
namespace s4u {
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
  /* Currently, only one instance is allowed to exist. This is why you can't copy or move it */
#ifndef DOXYGEN
  Engine(const Engine&) = delete;
  Engine(Engine&&)      = delete;
  ~Engine();
#endif

  /** Finalize the default engine and all its dependencies */
  XBT_ATTRIB_DEPRECATED_v335("Users are not supposed to shutdown the Engine") void shutdown();

  /** Run the simulation until its end */
  void run() const;

  /** Run the simulation until the specified date */
  void run_until(double max_date) const;

  /** @brief Retrieve the simulation time (in seconds) */
  static double get_clock();
  /** @brief Retrieve the engine singleton */
  static s4u::Engine* get_instance();
  static s4u::Engine* get_instance(int* argc, char** argv);
  static bool has_instance() { return instance_ != nullptr; }

  void load_platform(const std::string& platf) const;
  void seal_platform() const;

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
    kernel::actor::ActorCodeFactory code_factory = [](std::vector<std::string> args) {
      return kernel::actor::ActorCode([args = std::move(args)]() mutable {
        F code(std::move(args));
        code();
      });
    };
    register_function(name, code_factory);
  }
  /** @verbatim embed:rst:inline Bind an actor name that could be found in :ref:`pf_tag_actor` tag to a function name passed as a parameter. See the :ref:`example <s4u_ex_actors_create>`. @endverbatim */
  template <class F> void register_actor(const std::string& name, F code)
  {
    kernel::actor::ActorCodeFactory code_factory = [code](std::vector<std::string> args) {
      return kernel::actor::ActorCode([code, args = std::move(args)]() mutable { code(std::move(args)); });
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
  void host_register(const std::string& name, Host* host);
  void host_unregister(const std::string& name);
  void link_register(const std::string& name, const Link* link);
  void link_unregister(const std::string& name);
  void netpoint_register(simgrid::kernel::routing::NetPoint* card);
  void netpoint_unregister(simgrid::kernel::routing::NetPoint* card);
  void set_netzone_root(const NetZone* netzone);
#endif /*DOXYGEN*/

public:
  /** Returns the amount of hosts existing in the platform. */
  size_t get_host_count() const;
  /** Returns a vector of all hosts found in the platform.
   *
   * The order is generally different from the creation/declaration order in the XML platform because we use a hash
   * table internally.
   */
  std::vector<Host*> get_all_hosts() const;
  std::vector<Host*> get_filtered_hosts(const std::function<bool(Host*)>& filter) const;
  Host* host_by_name(const std::string& name) const;
  Host* host_by_name_or_null(const std::string& name) const;

  size_t get_link_count() const;
  std::vector<Link*> get_all_links() const;
  std::vector<Link*> get_filtered_links(const std::function<bool(Link*)>& filter) const;
  Link* link_by_name(const std::string& name) const;
  /**
   * @brief Find a split-duplex link from its name.
   * @throw std::invalid_argument if the searched link does not exist.
   */
  SplitDuplexLink* split_duplex_link_by_name(const std::string& name) const;
  Link* link_by_name_or_null(const std::string& name) const;

  Mailbox* mailbox_by_name_or_create(const std::string& name) const;

  size_t get_actor_count() const;
  std::vector<ActorPtr> get_all_actors() const;
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

  NetZone* get_netzone_root() const;

  NetZone* netzone_by_name_or_null(const std::string& name) const;

  /**
   * @brief Add a model to engine list
   *
   * @param model        Pointer to model
   * @param dependencies List of dependencies for this model (optional)
   */
  void add_model(std::shared_ptr<simgrid::kernel::resource::Model> model,
                 const std::vector<kernel::resource::Model*>& dependencies = {});

  /** @brief Get list of all models managed by this engine */
  const std::vector<simgrid::kernel::resource::Model*>& get_all_models() const;

  /** @brief Retrieves all netzones of the type indicated by the template argument */
  template <class T> std::vector<T*> get_filtered_netzones() const
  {
    static_assert(std::is_base_of<kernel::routing::NetZoneImpl, T>::value,
                  "Filtering netzones is only possible for subclasses of kernel::routing::NetZoneImpl");
    std::vector<T*> res;
    get_filtered_netzones_recursive(get_netzone_root(), &res);
    return res;
  }

  kernel::EngineImpl* get_impl() const { return pimpl; }

  /** Returns whether SimGrid was initialized yet -- mostly for internal use */
  static bool is_initialized();
  /** @brief set a configuration variable
   *
   * @beginrst
   * Do --help on any simgrid binary to see the list of currently existing configuration variables
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

  Engine* set_default_comm_data_copy_callback(void (*callback)(kernel::activity::CommImpl*, void*, size_t));

  /** Add a callback fired when the platform is created (ie, the xml file parsed),
   * right before the actual simulation starts. */
  static void on_platform_created_cb(const std::function<void()>& cb) { on_platform_created.connect(cb); }
  /** Add a callback fired when the platform is about to be created
   * (ie, after any configuration change and just before the resource creation) */
  static void on_platform_creation_cb(const std::function<void()>& cb) { on_platform_creation.connect(cb); }
  /** Add a callback fired when the main simulation loop ends, just before the end of Engine::run() */
  static void on_simulation_end_cb(const std::function<void()>& cb) { on_simulation_end.connect(cb); }

  /** Add a callback fired when the time jumps into the future */
  static void on_time_advance_cb(const std::function<void(double)>& cb) { on_time_advance.connect(cb); }

  /** Add a callback fired when the time cannot advance because of inter-actors deadlock. Note that the on_exit of each
   * actor is also executed on deadlock. */
  static void on_deadlock_cb(const std::function<void(void)>& cb) { on_deadlock.connect(cb); }

#ifndef DOXYGEN
  /* FIXME signals should be private */
  static xbt::signal<void()> on_platform_created;
  static xbt::signal<void()> on_platform_creation;
#endif

private:
  static xbt::signal<void()> on_simulation_end;
  static xbt::signal<void(double)> on_time_advance;
  static xbt::signal<void(void)> on_deadlock;
  kernel::EngineImpl* const pimpl;
  static Engine* instance_;
  void initialize(int* argc, char** argv);
};

std::vector<ActivityPtr> create_DAG_from_dot(const std::string& filename);
std::vector<ActivityPtr> create_DAG_from_DAX(const std::string& filename);

#ifndef DOXYGEN /* Internal use only, no need to expose it */
template <class T>
XBT_PRIVATE void get_filtered_netzones_recursive(const s4u::NetZone* current, std::vector<T*>* whereto)
{
  static_assert(std::is_base_of<kernel::routing::NetZoneImpl, T>::value,
                "Filtering netzones is only possible for subclasses of kernel::routing::NetZoneImpl");
  for (auto const& elem : current->get_children()) {
    get_filtered_netzones_recursive(elem, whereto);
    auto* elem_impl = dynamic_cast<T*>(elem->get_impl());
    if (elem_impl != nullptr)
      whereto->push_back(elem_impl);
  }
}
#endif
} // namespace s4u
} // namespace simgrid

#endif /* SIMGRID_S4U_ENGINE_HPP */
