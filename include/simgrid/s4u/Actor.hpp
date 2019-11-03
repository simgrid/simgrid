/* Copyright (c) 2006-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_ACTOR_HPP
#define SIMGRID_S4U_ACTOR_HPP

#include <simgrid/forward.h>

#include <simgrid/chrono.hpp>
#include <xbt/Extendable.hpp>
#include <xbt/functional.hpp>
#include <xbt/signal.hpp>
#include <xbt/string.hpp>

#include <functional>
#include <map> // deprecated wrappers
#include <unordered_map>

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
 * <!DOCTYPE platform SYSTEM "https://simgrid.org/simgrid.dtd">
 * <platform version="4.1">
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
class XBT_PUBLIC Actor : public xbt::Extendable<Actor> {
#ifndef DOXYGEN
  friend Exec;
  friend Mailbox;
  friend kernel::actor::ActorImpl;
  friend kernel::activity::MailboxImpl;

  kernel::actor::ActorImpl* const pimpl_;
#endif

  explicit Actor(smx_actor_t pimpl) : pimpl_(pimpl) {}

public:
#ifndef DOXYGEN
  // ***** No copy *****
  Actor(Actor const&) = delete;
  Actor& operator=(Actor const&) = delete;

  // ***** Reference count *****
  friend XBT_PUBLIC void intrusive_ptr_add_ref(Actor * actor);
  friend XBT_PUBLIC void intrusive_ptr_release(Actor * actor);
#endif
  /** Retrive the amount of references on that object. Useful to debug the automatic refcounting */
  int get_refcount();

  // ***** Actor creation *****
  /** Retrieve a reference to myself */
  static Actor* self();

  /** Fired when a new actor has been created **/
  static xbt::signal<void(Actor&)> on_creation;
  /** Signal to others that an actor has been suspended**/
  static xbt::signal<void(Actor const&)> on_suspend;
  /** Signal to others that an actor has been resumed **/
  static xbt::signal<void(Actor const&)> on_resume;
  /** Signal to others that an actor is sleeping **/
  static xbt::signal<void(Actor const&)> on_sleep;
  /** Signal to others that an actor wakes up for a sleep **/
  static xbt::signal<void(Actor const&)> on_wake_up;
  /** Signal to others that an actor is going to migrated to another host**/
  static xbt::signal<void(Actor const&)> on_migration_start;
  /** Signal to others that an actor is has been migrated to another host **/
  static xbt::signal<void(Actor const&)> on_migration_end;
  /** Signal indicating that an actor terminated its code.
   *  The actor may continue to exist if it is still referenced in the simulation, but it's not active anymore.
   *  If you want to free extra data when the actor's destructor is called, use Actor::on_destruction.
   *  If you want to register to the termination of a given actor, use this_actor::on_exit() instead.*/
  static xbt::signal<void(Actor const&)> on_termination;
  /** Signal indicating that an actor is about to disappear (its destructor was called).
   *  This signal is fired for any destructed actor, which is mostly useful when designing plugins and extensions.
   *  If you want to react to the end of the actor's code, use Actor::on_termination instead.
   *  If you want to register to the termination of a given actor, use this_actor::on_exit() instead.*/
  static xbt::signal<void(Actor const&)> on_destruction;

  /** Create an actor from a std::function<void()>
   *
   *  If the actor is restarted, the actor has a fresh copy of the function.
   */
  static ActorPtr create(const std::string& name, s4u::Host* host, const std::function<void()>& code);
  static ActorPtr init(const std::string& name, s4u::Host* host);
  ActorPtr start(const std::function<void()>& code);

  /** Create an actor from a std::function
   *
   *  If the actor is restarted, the actor has a fresh copy of the function.
   */
  template <class F> static ActorPtr create(const std::string& name, s4u::Host* host, F code)
  {
    return create(name, host, std::function<void()>(std::move(code)));
  }

  /** Create an actor using a callable thing and its arguments.
   *
   * Note that the arguments will be copied, so move-only parameters are forbidden */
  template <class F, class... Args,
            // This constructor is enabled only if the call code(args...) is valid:
            typename = typename std::result_of<F(Args...)>::type>
  static ActorPtr create(const std::string& name, s4u::Host* host, F code, Args... args)
  {
    return create(name, host, std::bind(std::move(code), std::move(args)...));
  }

  // Create actor from function name:
  static ActorPtr create(const std::string& name, s4u::Host* host, const std::string& function,
                         std::vector<std::string> args);

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
  Host* get_host() const;
  /** Retrieves the actor ID of that actor */
  aid_t get_pid() const;
  /** Retrieves the actor ID of that actor's creator */
  aid_t get_ppid() const;

  /** Suspend an actor, that is blocked until resume()ed by another actor */
  void suspend();

  /** Resume an actor that was previously suspend()ed */
  void resume();

  /** Returns true if the actor is suspended. */
  bool is_suspended();

  /** If set to true, the actor will automatically restart when its host reboots */
  void set_auto_restart(bool autorestart);

  /** Add a function to the list of "on_exit" functions for the current actor. The on_exit functions are the functions
   * executed when your actor is killed. You should use them to free the data used by your actor.
   *
   * Please note that functions registered in this signal cannot do any simcall themselves. It means that they cannot
   * send or receive messages, acquire or release mutexes, nor even modify a host property or something. Not only are
   * blocking functions forbidden in this setting, but also modifications to the global state.
   *
   * The parameter of on_exit's callbacks denotes whether or not the actor's execution failed.
   * It will be set to true if the actor was killed or failed because of an exception,
   * while it will remain to false if the actor terminated gracefully.
   */
  void on_exit(const std::function<void(bool /*failed*/)>& fun) const;

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

  /** Retrieves the actor that have the given PID (or nullptr if not existing) */
  static ActorPtr by_pid(aid_t pid);

  /** Wait for the actor to finish.
   *
   * Blocks the calling actor until the joined actor is terminated. If actor alice executes bob.join(), then alice is
   * blocked until bob terminates.
   */
  void join();

  /** Wait for the actor to finish, or for the timeout to elapse.
   *
   * Blocks the calling actor until the joined actor is terminated. If actor alice executes bob.join(), then alice is
   * blocked until bob terminates.
   */
  void join(double timeout);
  Actor* restart();

  /** Kill all actors (but the issuer). Being killed is not something that actors can delay or avoid. */
  static void kill_all();

  /** Returns the internal implementation of this actor */
  kernel::actor::ActorImpl* get_impl() const { return pimpl_; }

  /** Retrieve the property value (or nullptr if not set) */
  const std::unordered_map<std::string, std::string>*
  get_properties() const; // FIXME: do not export the map, but only the keys or something
  const char* get_property(const std::string& key) const;
  void set_property(const std::string& key, const std::string& value);
};

/** @ingroup s4u_api
 *  @brief Static methods working on the current actor (see @ref s4u::Actor) */
namespace this_actor {

XBT_PUBLIC bool is_maestro();

/** Block the current actor sleeping for that amount of seconds */
XBT_PUBLIC void sleep_for(double duration);
/** Block the current actor sleeping until the specified timestamp */
XBT_PUBLIC void sleep_until(double wakeup_time);

template <class Rep, class Period> inline void sleep_for(std::chrono::duration<Rep, Period> duration)
{
  auto seconds = std::chrono::duration_cast<SimulationClockDuration>(duration);
  this_actor::sleep_for(seconds.count());
}

template <class Duration> inline void sleep_until(const SimulationTimePoint<Duration>& wakeup_time)
{
  auto timeout_native = std::chrono::time_point_cast<SimulationClockDuration>(wakeup_time);
  this_actor::sleep_until(timeout_native.time_since_epoch().count());
}

/** Block the current actor, computing the given amount of flops */
XBT_PUBLIC void execute(double flop);

/** Block the current actor, computing the given amount of flops at the given priority.
 *  An execution of priority 2 computes twice as fast as an execution at priority 1. */
XBT_PUBLIC void execute(double flop, double priority);

/**
 * @example examples/s4u/exec-ptask/s4u-exec-ptask.cpp
 */

/** Block the current actor until the built parallel execution terminates
 *
 * \rst
 * .. _API_s4u_parallel_execute:
 *
 * **Example of use:** `examples/s4u/exec-ptask/s4u-exec-ptask.cpp
 * <https://framagit.org/simgrid/simgrid/tree/master/examples/s4u/exec-ptask/s4u-exec-ptask.cpp>`_
 *
 * Parallel executions convenient abstractions of parallel computational kernels that span over several machines,
 * such as a PDGEM and the other ScaLAPACK routines. If you are interested in the effects of such parallel kernel
 * on the platform (e.g. to schedule them wisely), there is no need to model them in all details of their internal
 * execution and communications. It is much more convenient to model them as a single execution activity that spans
 * over several hosts. This is exactly what s4u's Parallel Executions are.
 *
 * To build such an object, you need to provide a list of hosts that are involved in the parallel kernel (the
 * actor's own host may or may not be in this list) and specify the amount of computations that should be done by
 * each host, using a vector of flops amount. Then, you should specify the amount of data exchanged between each
 * hosts during the parallel kernel. For that, a matrix of values is expected.
 *
 * It is OK to build a parallel execution without any computation and/or without any communication.
 * Just pass an empty vector to the corresponding parameter.
 *
 * For example, if your list of hosts is ``[host0, host1]``, passing a vector ``[1000, 2000]`` as a `flops_amount`
 * vector means that `host0` should compute 1000 flops while `host1` will compute 2000 flops. A matrix of
 * communications' sizes of ``[0, 1, 2, 3]`` specifies the following data exchanges:
 *
 *   +-----------+-------+------+
 *   |from \\ to | host0 | host1|
 *   +===========+=======+======+
 *   |host0      |   0   |  1   |
 *   +-----------+-------+------+
 *   |host1      |   2   |  3   |
 *   +-----------+-------+------+
 *
 * - From host0 to host0: 0 bytes are exchanged
 * - From host0 to host1: 1 byte is exchanged
 * - From host1 to host0: 2 bytes are exchanged
 * - From host1 to host1: 3 bytes are exchanged
 *
 * In a parallel execution, all parts (all executions on each hosts, all communications) progress exactly at the
 * same pace, so they all terminate at the exact same pace. If one part is slow because of a slow resource or
 * because of contention, this slows down the parallel execution as a whole.
 *
 * These objects are somewhat surprising from a modeling point of view. For example, the unit of their speed is
 * somewhere between flop/sec and byte/sec. Arbitrary parallel executions will simply not work with the usual platform
 * models, and you must :ref:`use the ptask_L07 host model <options_model_select>` for that. Note that you can mix
 * regular executions and communications with parallel executions, provided that the host model is ptask_L07.
 *
 * \endrst
 */
XBT_PUBLIC void parallel_execute(const std::vector<s4u::Host*>& hosts, const std::vector<double>& flops_amounts,
                                 const std::vector<double>& bytes_amounts);

/** \rst
 * Block the current actor until the built :ref:`parallel execution <API_s4u_parallel_execute>` completes, or until the
 * timeout. \endrst
 */
XBT_PUBLIC void parallel_execute(const std::vector<s4u::Host*>& hosts, const std::vector<double>& flops_amounts,
                                 const std::vector<double>& bytes_amounts, double timeout);

XBT_PUBLIC ExecPtr exec_init(double flops_amounts);
XBT_PUBLIC ExecPtr exec_init(const std::vector<s4u::Host*>& hosts, const std::vector<double>& flops_amounts,
                             const std::vector<double>& bytes_amounts);

XBT_PUBLIC ExecPtr exec_async(double flops_amounts);

/** @brief Returns the actor ID of the current actor. */
XBT_PUBLIC aid_t get_pid();

/** @brief Returns the ancestor's actor ID of the current actor. */
XBT_PUBLIC aid_t get_ppid();

/** @brief Returns the name of the current actor. */
XBT_PUBLIC std::string get_name();
/** @brief Returns the name of the current actor as a C string. */
XBT_PUBLIC const char* get_cname();

/** @brief Returns the name of the host on which the current actor is running. */
XBT_PUBLIC Host* get_host();

/** @brief Suspend the current actor, that is blocked until resume()ed by another actor. */
XBT_PUBLIC void suspend();

/** @brief Yield the current actor. */
XBT_PUBLIC void yield();

/** @brief Resume the current actor, that was suspend()ed previously. */
XBT_PUBLIC void resume();

/** @brief kill the current actor. */
XBT_PUBLIC void exit();

/** @brief Add a function to the list of "on_exit" functions of the current actor.
 *
 * The on_exit functions are the functions executed when your actor is killed. You should use them to free the data used
 * by your actor.
 *
 * Please note that functions registered in this signal cannot do any simcall themselves. It means that they cannot
 * send or receive messages, acquire or release mutexes, nor even modify a host property or something. Not only are
 * blocking functions forbidden in this setting, but also modifications to the global state.
 *
 * The parameter of on_exit's callbacks denotes whether or not the actor's execution failed.
 * It will be set to true if the actor was killed or failed because of an exception,
 * while it will remain to false if the actor terminated gracefully.
 */

XBT_PUBLIC void on_exit(const std::function<void(bool)>& fun);

/** @brief Migrate the current actor to a new host. */
XBT_PUBLIC void migrate(Host* new_host);

/** @} */
}


}} // namespace simgrid::s4u


#endif /* SIMGRID_S4U_ACTOR_HPP */
