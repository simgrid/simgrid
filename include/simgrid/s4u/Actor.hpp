/* Copyright (c) 2006-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_ACTOR_HPP
#define SIMGRID_S4U_ACTOR_HPP

#include <functional>
#include <map> // deprecated wrappers
#include <simgrid/chrono.hpp>
#include <unordered_map>
#include <xbt/Extendable.hpp>
#include <xbt/functional.hpp>
#include <xbt/signal.hpp>
#include <xbt/string.hpp>

namespace simgrid {
namespace s4u {

/**
 *
 * An actor is an independent stream of execution in your distributed application.
 *
 * You can think of an actor as a process in your distributed application, or as a thread in a multithreaded program.
 * This is the only component in SimGrid that actually does something on its own, executing its own code.
 * A resource will not get used if you don't schedule activities on them. This is the code of Actors that create and
 * schedule these activities.
 *
 * An actor is located on a (simulated) host, but it can interact
 * with the whole simulated platform.
 *
 * The s4u::Actor API is strongly inspired from the C++11 threads.
 * The <a href="http://en.cppreference.com/w/cpp/thread">documentation
 * of this standard</a> may help to understand the philosophy of the S4U
 * Actors.
 *
 * @section s4u_actor_def Defining the skeleton of an Actor
 *
 * As in the <a href="http://en.cppreference.com/w/cpp/thread">C++11
 * standard</a>, you can declare the code of your actor either as a
 * pure function or as an object. It is very simple with functions:
 *
 * @code{.cpp}
 * #include <simgrid/s4u/actor.hpp>
 *
 * // Declare the code of your worker
 * void worker() {
 *   printf("Hello s4u");
 *   simgrid::s4u::this_actor::execute(5*1024*1024); // Get the worker executing a task of 5 MFlops
 * };
 *
 * // From your main or from another actor, create your actor on the host Jupiter
 * // The following line actually creates a new actor, even if there is no "new".
 * Actor("Alice", simgrid::s4u::Host::by_name("Jupiter"), worker);
 * @endcode
 *
 * But some people prefer to encapsulate their actors in classes and
 * objects to save the actor state in a cleanly dedicated location.
 * The syntax is slightly more complicated, but not much.
 *
 * @code{.cpp}
 * #include <simgrid/s4u/actor.hpp>
 *
 * // Declare the class representing your actors
 * class Worker {
 * public:
 *   void operator()() { // Two pairs of () because this defines the method called ()
 *     printf("Hello s4u");
 *     simgrid::s4u::this_actor::execute(5*1024*1024); // Get the worker executing a task of 5 MFlops
 *   }
 * };
 *
 * // From your main or from another actor, create your actor. Note the () after Worker
 * Actor("Bob", simgrid::s4u::Host::by_name("Jupiter"), Worker());
 * @endcode
 *
 * @section s4u_actor_flesh Fleshing your actor
 *
 * The body of your actor can use the functions of the
 * simgrid::s4u::this_actor namespace to interact with the world.
 * This namespace contains the methods to start new activities
 * (executions, communications, etc), and to get informations about
 * the currently running thread (its location, etc).
 *
 * Please refer to the @link simgrid::s4u::this_actor full API @endlink.
 *
 *
 * @section s4u_actor_deploy Using a deployment file
 *
 * @warning This is currently not working with S4U. Sorry about that.
 *
 * The best practice is to use an external deployment file as
 * follows, because it makes it easier to test your application in
 * differing settings. Load this file with
 * s4u::Engine::loadDeployment() before the simulation starts.
 * Refer to the @ref deployment section for more information.
 *
 * @code{.xml}
 * <?xml version='1.0'?>
 * <!DOCTYPE platform SYSTEM "http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd">
 * <platform version="4">
 *
 *   <!-- Start an actor called 'master' on the host called 'Tremblay' -->
 *   <actor host="Tremblay" function="master">
 *      <!-- Here come the parameter that you want to feed to this instance of master -->
 *      <argument value="20"/>        <!-- argv[1] -->
 *      <argument value="50000000"/>  <!-- argv[2] -->
 *      <argument value="1000000"/>   <!-- argv[3] -->
 *      <argument value="5"/>         <!-- argv[4] -->
 *   </actor>
 *
 *   <!-- Start an actor called 'worker' on the host called 'Jupiter' -->
 *   <actor host="Jupiter" function="worker"/> <!-- Don't provide any parameter ->>
 *
 * </platform>
 * @endcode
 *
 *  @{
 */

/** @brief Simulation Agent */
class XBT_PUBLIC Actor : public simgrid::xbt::Extendable<Actor> {
  friend simgrid::s4u::Exec;
  friend simgrid::s4u::Mailbox;
  friend simgrid::kernel::actor::ActorImpl;
  friend simgrid::kernel::activity::MailboxImpl;

  kernel::actor::ActorImpl* pimpl_ = nullptr;

  explicit Actor(smx_actor_t pimpl) : pimpl_(pimpl) {}

public:

  // ***** No copy *****
  Actor(Actor const&) = delete;
  Actor& operator=(Actor const&) = delete;

  // ***** Reference count *****
  friend XBT_PUBLIC void intrusive_ptr_add_ref(Actor * actor);
  friend XBT_PUBLIC void intrusive_ptr_release(Actor * actor);

  // ***** Actor creation *****
  /** Retrieve a reference to myself */
  static ActorPtr self();

  /** Signal to others that a new actor has been created **/
  static simgrid::xbt::signal<void(simgrid::s4u::ActorPtr)> on_creation;
  /** Signal to others that an actor has been suspended**/
  static simgrid::xbt::signal<void(simgrid::s4u::ActorPtr)> on_suspend;
  /** Signal to others that an actor has been resumed **/
  static simgrid::xbt::signal<void(simgrid::s4u::ActorPtr)> on_resume;
  /** Signal to others that an actor is sleeping **/
  static simgrid::xbt::signal<void(simgrid::s4u::ActorPtr)> on_sleep;
  /** Signal to others that an actor wakes up for a sleep **/
  static simgrid::xbt::signal<void(simgrid::s4u::ActorPtr)> on_wake_up;
  /** Signal to others that an actor is going to migrated to another host**/
  static simgrid::xbt::signal<void(simgrid::s4u::ActorPtr)> on_migration_start;
  /** Signal to others that an actor is has been migrated to another host **/
  static simgrid::xbt::signal<void(simgrid::s4u::ActorPtr)> on_migration_end;
  /** Signal indicating that an actor is about to disappear.
   *  This signal is fired for any dying actor, which is mostly useful when
   *  designing plugins and extensions. If you want to register to the
   *  termination of a given actor, use this_actor::on_exit() instead.*/
  static simgrid::xbt::signal<void(simgrid::s4u::ActorPtr)> on_destruction;

  /** Create an actor from a std::function<void()>
   *
   *  If the actor is restarted, the actor has a fresh copy of the function.
   */
  static ActorPtr create(std::string name, s4u::Host* host, std::function<void()> code);

  /** Create an actor from a std::function
   *
   *  If the actor is restarted, the actor has a fresh copy of the function.
   */
  template <class F> static ActorPtr create(std::string name, s4u::Host* host, F code)
  {
    return create(name, host, std::function<void()>(std::move(code)));
  }

  /** Create an actor using a callable thing and its arguments.
   *
   * Note that the arguments will be copied, so move-only parameters are forbidden */
  template <class F, class... Args,
            // This constructor is enabled only if the call code(args...) is valid:
            typename = typename std::result_of<F(Args...)>::type>
  static ActorPtr create(std::string name, s4u::Host* host, F code, Args... args)
  {
    return create(name, host, std::bind(std::move(code), std::move(args)...));
  }

  // Create actor from function name:
  static ActorPtr create(std::string name, s4u::Host* host, std::string function, std::vector<std::string> args);

  // ***** Methods *****
  /** This actor will be automatically terminated when the last non-daemon actor finishes **/
  void daemonize();

  /** Returns whether or not this actor has been daemonized or not **/
  bool is_daemon() const;

  /** Retrieves the name of that actor as a C++ string */
  const simgrid::xbt::string& get_name() const;
  /** Retrieves the name of that actor as a C string */
  const char* get_cname() const;
  /** Retrieves the host on which that actor is running */
  s4u::Host* get_host();
  /** Retrieves the PID of that actor
   *
   * aid_t is an alias for long */
  aid_t get_pid() const;
  /** Retrieves the PPID of that actor
   *
   * aid_t is an alias for long */
  aid_t get_ppid() const;

  /** Suspend an actor by suspending the task on which it was waiting for the completion. */
  void suspend();

  /** Resume a suspended actor by resuming the task on which it was waiting for the completion. */
  void resume();

  void yield();

  /** Returns true if the actor is suspended. */
  bool is_suspended();

  /** If set to true, the actor will automatically restart when its host reboots */
  void set_auto_restart(bool autorestart);

  /** Add a function to the list of "on_exit" functions for the current actor. The on_exit functions are the functions
   * executed when your actor is killed. You should use them to free the data used by your actor.
   */
  void on_exit(std::function<void(int, void*)> fun, void* data);

  /** Sets the time at which that actor should be killed */
  void set_kill_time(double time);
  /** Retrieves the time at which that actor will be killed (or -1 if not set) */
  double get_kill_time();

  /** @brief Moves the actor to another host
   *
   * If the actor is currently blocked on an execution activity, the activity is also
   * migrated to the new host. If it's blocked on another kind of activity, an error is
   * raised as the mandated code is not written yet. Please report that bug if you need it.
   *
   * Asynchronous activities started by the actor are not migrated automatically, so you have
   * to take care of this yourself (only you knows which ones should be migrated).
   */
  void migrate(Host * new_host);

  /** Ask the actor to die.
   *
   * Any blocking activity will be canceled, and it will be rescheduled to free its memory.
   * Being killed is not something that actors can defer or avoid.
   *
   * SimGrid still have sometimes issues when you kill actors that are currently communicating and such.
   * Still. Please report any bug that you may encounter with a minimal working example.
   */
  void kill();

  /** Kill an actor from its ID */
  static void kill(aid_t pid);

  /** Retrieves the actor that have the given PID (or nullptr if not existing) */
  static ActorPtr by_pid(aid_t pid);

  /** Wait for the actor to finish.
   *
   * This blocks the calling actor until the actor on which we call join() is terminated
   */
  void join();
  void join(double timeout);
  Actor* restart();

  /** Ask kindly to all actors to die. Only the issuer will survive. */
  static void kill_all();

  /** Returns the internal implementation of this actor */
  kernel::actor::ActorImpl* get_impl();

  /** Retrieve the property value (or nullptr if not set) */
  std::unordered_map<std::string, std::string>*
  get_properties(); // FIXME: do not export the map, but only the keys or something
  const char* get_property(std::string key);
  void set_property(std::string key, std::string value);

#ifndef DOXYGEN
  /** @deprecated See Actor::create() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Actor::create()") static ActorPtr createActor(
      const char* name, s4u::Host* host, std::function<void()> code)
  {
    return create(name, host, code);
  }
  /** @deprecated See Actor::create() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Actor::create()") static ActorPtr createActor(
      const char* name, s4u::Host* host, std::function<void(std::vector<std::string>*)> code,
      std::vector<std::string>* args)
  {
    return create(name, host, code, args);
  }
  /** @deprecated See Actor::create() */
  template <class F, class... Args, typename = typename std::result_of<F(Args...)>::type>
  XBT_ATTRIB_DEPRECATED_v323("Please use Actor::create()") static ActorPtr createActor(
      const char* name, s4u::Host* host, F code, Args... args)
  {
    return create(name, host, code, std::move(args)...);
  }
  /** @deprecated See Actor::create() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Actor::create()") static ActorPtr createActor(
      const char* name, s4u::Host* host, const char* function, std::vector<std::string> args)
  {
    return create(name, host, function, args);
  }
  /** @deprecated See Actor::is_daemon() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Actor::is_daemon()") bool isDaemon() const;
  /** @deprecated See Actor::get_name() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Actor::get_name()") const simgrid::xbt::string& getName() const
  {
    return get_name();
  }
  /** @deprecated See Actor::get_cname() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Actor::get_cname()") const char* getCname() const { return get_cname(); }
  /** @deprecated See Actor::get_host() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Actor::get_host()") Host* getHost() { return get_host(); }
  /** @deprecated See Actor::get_pid() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Actor::get_pid()") aid_t getPid() { return get_pid(); }
  /** @deprecated See Actor::get_ppid() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Actor::get_ppid()") aid_t getPpid() { return get_ppid(); }
  /** @deprecated See Actor::is_suspended() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Actor::is_suspended()") int isSuspended() { return is_suspended(); }
  /** @deprecated See Actor::set_auto_restart() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Actor::set_auto_restart()") void setAutoRestart(bool a)
  {
    set_auto_restart(a);
  }
  /** @deprecated Please use a std::function<void(int, void*)> for first parameter */
  XBT_ATTRIB_DEPRECATED_v323("Please use a std::function<void(int, void*)> for first parameter.") void on_exit(
      int_f_pvoid_pvoid_t fun, void* data);
  /** @deprecated See Actor::on_exit() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Actor::on_exit()") void onExit(int_f_pvoid_pvoid_t fun, void* data)
  {
    on_exit([fun](int a, void* b) { fun((void*)(intptr_t)a, b); }, data);
  }
  /** @deprecated See Actor::set_kill_time() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Actor::set_kill_time()") void setKillTime(double time) { set_kill_time(time); }
  /** @deprecated See Actor::get_kill_time() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Actor::get_kill_time()") double getKillTime() { return get_kill_time(); }
  /** @deprecated See Actor::by_pid() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Actor::by_pid()") static ActorPtr byPid(aid_t pid) { return by_pid(pid); }
  /** @deprecated See Actor::kill_all() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Actor::kill_all()") static void killAll() { kill_all(); }
  /** @deprecated See Actor::kill_all() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Actor::kill_all() with no parameter") static void killAll(
      int XBT_ATTRIB_UNUSED resetPid)
  {
    kill_all();
  }
  /** @deprecated See Actor::get_impl() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Actor::get_impl()") kernel::actor::ActorImpl* getImpl() { return get_impl(); }
  /** @deprecated See Actor::get_property() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Actor::get_property()") const char* getProperty(const char* key)
  {
    return get_property(key);
  }
  /** @deprecated See Actor::get_properties() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Actor::get_properties()") std::map<std::string, std::string>* getProperties()
  {
    std::map<std::string, std::string>* res             = new std::map<std::string, std::string>();
    std::unordered_map<std::string, std::string>* props = get_properties();
    for (auto const& kv : *props)
      res->insert(kv);
    return res;
  }
  /** @deprecated See Actor::get_properties() */
  XBT_ATTRIB_DEPRECATED_v323("Please use Actor::get_properties()") void setProperty(const char* key, const char* value)
  {
    set_property(key, value);
  }
#endif
};

/** @ingroup s4u_api
 *  @brief Static methods working on the current actor (see @ref s4u::Actor) */
namespace this_actor {

XBT_PUBLIC bool is_maestro();

/** Block the actor sleeping for that amount of seconds (may throws hostFailure) */
XBT_PUBLIC void sleep_for(double duration);
XBT_PUBLIC void sleep_until(double timeout);

template <class Rep, class Period> inline void sleep_for(std::chrono::duration<Rep, Period> duration)
{
  auto seconds = std::chrono::duration_cast<SimulationClockDuration>(duration);
  this_actor::sleep_for(seconds.count());
}

template <class Duration> inline void sleep_until(const SimulationTimePoint<Duration>& timeout_time)
{
  auto timeout_native = std::chrono::time_point_cast<SimulationClockDuration>(timeout_time);
  this_actor::sleep_until(timeout_native.time_since_epoch().count());
}

/** Block the actor, computing the given amount of flops */
XBT_PUBLIC void execute(double flop);

/** Block the actor, computing the given amount of flops at the given priority.
 *  An execution of priority 2 computes twice as fast as an execution at priority 1. */
XBT_PUBLIC void execute(double flop, double priority);

XBT_PUBLIC void parallel_execute(int host_nb, sg_host_t* host_list, double* flops_amount, double* bytes_amount);
XBT_PUBLIC void parallel_execute(int host_nb, sg_host_t* host_list, double* flops_amount, double* bytes_amount,
                                 double timeout);

XBT_PUBLIC ExecPtr exec_init(double flops_amounts);
XBT_PUBLIC ExecPtr exec_async(double flops_amounts);

/** @brief Returns the actor ID of the current actor). */
XBT_PUBLIC aid_t get_pid();

/** @brief Returns the ancestor's actor ID of the current actor. */
XBT_PUBLIC aid_t get_ppid();

/** @brief Returns the name of the current actor. */
XBT_PUBLIC std::string get_name();
/** @brief Returns the name of the current actor as a C string. */
XBT_PUBLIC const char* get_cname();

/** @brief Returns the name of the host on which the actor is running. */
XBT_PUBLIC Host* get_host();

/** @brief Suspend the actor. */
XBT_PUBLIC void suspend();

/** @brief yield the actor. */
XBT_PUBLIC void yield();

/** @brief Resume the actor. */
XBT_PUBLIC void resume();

XBT_PUBLIC bool is_suspended();

/** @brief kill the actor. */
XBT_PUBLIC void exit();

/** @brief Add a function to the list of "on_exit" functions. */
XBT_PUBLIC void on_exit(std::function<void(int, void*)> fun, void* data);

/** @brief Migrate the actor to a new host. */
XBT_PUBLIC void migrate(Host* new_host);

/** @} */

#ifndef DOXYGEN
/** @deprecated Please use std::function<void(int, void*)> for first parameter */
XBT_ATTRIB_DEPRECATED_v323("Please use std::function<void(int, void*)> for first parameter.") XBT_PUBLIC
    void on_exit(int_f_pvoid_pvoid_t fun, void* data);
/** @deprecated See this_actor::get_name() */
XBT_ATTRIB_DEPRECATED_v323("Please use this_actor::get_name()") XBT_PUBLIC std::string getName();
/** @deprecated See this_actor::get_cname() */
XBT_ATTRIB_DEPRECATED_v323("Please use this_actor::get_cname()") XBT_PUBLIC const char* getCname();
/** @deprecated See this_actor::is_maestro() */
XBT_ATTRIB_DEPRECATED_v323("Please use this_actor::is_maestro()") XBT_PUBLIC bool isMaestro();
/** @deprecated See this_actor::get_pid() */
XBT_ATTRIB_DEPRECATED_v323("Please use this_actor::get_pid()") XBT_PUBLIC aid_t getPid();
/** @deprecated See this_actor::get_ppid() */
XBT_ATTRIB_DEPRECATED_v323("Please use this_actor::get_ppid()") XBT_PUBLIC aid_t getPpid();
/** @deprecated See this_actor::get_host() */
XBT_ATTRIB_DEPRECATED_v323("Please use this_actor::get_host()") XBT_PUBLIC Host* getHost();
/** @deprecated See this_actor::is_suspended() */
XBT_ATTRIB_DEPRECATED_v323("Please use this_actor::is_suspended()") XBT_PUBLIC bool isSuspended();
/** @deprecated See this_actor::on_exit() */
XBT_ATTRIB_DEPRECATED_v323("Please use this_actor::on_exit()") XBT_PUBLIC void onExit(int_f_pvoid_pvoid_t fun, void* data);
/** @deprecated See this_actor::exit() */
XBT_ATTRIB_DEPRECATED_v324("Please use this_actor::exit()") XBT_PUBLIC void kill();
#endif
}


}} // namespace simgrid::s4u


#endif /* SIMGRID_S4U_ACTOR_HPP */
