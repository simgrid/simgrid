/* Copyright (c) 2006-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_ACTOR_HPP
#define SIMGRID_S4U_ACTOR_HPP

#include <simgrid/forward.h>

#include <simgrid/chrono.hpp>
#include <xbt/Extendable.hpp>
#include <xbt/signal.hpp>
#include <xbt/string.hpp>

#include <functional>
#include <unordered_map>

namespace simgrid {

extern template class XBT_PUBLIC xbt::Extendable<s4u::Actor>;

namespace s4u {

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
 * @example examples/cpp/exec-ptask/s4u-exec-ptask.cpp
 */

/** Block the current actor until the built parallel execution terminates
 *
 * @beginrst
 * .. _API_s4u_parallel_execute:
 *
 * **Example of use:** `examples/cpp/exec-ptask/s4u-exec-ptask.cpp
 * <https://framagit.org/simgrid/simgrid/tree/master/examples/cpp/exec-ptask/s4u-exec-ptask.cpp>`_
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
 * - from host0: [ to host0:  0 bytes; to host1: 1 byte ]
 *
 * - from host1: [ to host0: 2 bytes; to host1: 3 bytes ]
 *
 * Or, in other words:
 *
 * - From host0 to host0: 0 bytes are exchanged
 *
 * - From host0 to host1: 1 byte is exchanged
 *
 * - From host1 to host0: 2 bytes are exchanged
 *
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
 * @endrst
 */
/** Block the current actor until the built parallel execution completes */
XBT_PUBLIC void parallel_execute(const std::vector<s4u::Host*>& hosts, const std::vector<double>& flops_amounts,
                                 const std::vector<double>& bytes_amounts);

/** Block the current actor until the built multi-thread execution completes */
XBT_PUBLIC void thread_execute(s4u::Host* host, double flop_amounts, int thread_count);

/** Initialize a sequential execution that must then be started manually */
XBT_PUBLIC ExecPtr exec_init(double flops_amounts);
/** Initialize a parallel execution that must then be started manually */
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

/** @brief kill the current actor. */
XBT_ATTRIB_NORETURN XBT_PUBLIC void exit();

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
 * It will be set to true if the actor was killed or failed because of an exception or if the simulation deadlocked,
 * while it will remain to false if the actor terminated gracefully.
 */

XBT_PUBLIC void on_exit(const std::function<void(bool)>& fun);

/** @brief Migrate the current actor to a new host. */
XBT_PUBLIC void set_host(Host* new_host);
} // namespace this_actor

/** An actor is an independent stream of execution in your distributed application.
 *
 * @beginrst
 * It is located on a (simulated) :cpp:class:`host <simgrid::s4u::Host>`, but can interact
 * with the whole simulated platform.
 *
 * You can think of an actor as a process in your distributed application, or as a thread in a multithreaded program.
 * This is the only component in SimGrid that actually does something on its own, executing its own code.
 * A resource will not get used if you don't schedule activities on them. This is the code of Actors that create and
 * schedule these activities. **Please refer to the** :ref:`examples <s4u_ex_actors>` **for more information.**
 *
 * This API is strongly inspired from the C++11 threads.
 * The `documentation of this standard <http://en.cppreference.com/w/cpp/thread>`_
 * may help to understand the philosophy of the SimGrid actors.
 *
 * @endrst
 */
class XBT_PUBLIC Actor : public xbt::Extendable<Actor> {
#ifndef DOXYGEN
  friend Exec;
  friend Mailbox;
  friend kernel::actor::ActorImpl;
  friend kernel::activity::MailboxImpl;
  friend XBT_PUBLIC void this_actor::sleep_for(double);
  friend XBT_PUBLIC void this_actor::suspend();

  kernel::actor::ActorImpl* const pimpl_;
#endif

  explicit Actor(kernel::actor::ActorImpl* pimpl) : pimpl_(pimpl) {}

public:
#ifndef DOXYGEN
  // ***** No copy *****
  Actor(Actor const&) = delete;
  Actor& operator=(Actor const&) = delete;

  // ***** Reference count *****
  friend XBT_PUBLIC void intrusive_ptr_add_ref(const Actor* actor);
  friend XBT_PUBLIC void intrusive_ptr_release(const Actor* actor);
#endif
  /** Retrieve the amount of references on that object. Useful to debug the automatic refcounting */
  int get_refcount() const;

  // ***** Actor creation *****
  /** Retrieve a reference to myself */
  static Actor* self();

private:
  static xbt::signal<void(Actor&)> on_creation;
  static xbt::signal<void(Actor const&)> on_suspend;
  static xbt::signal<void(Actor const&)> on_resume;
  static xbt::signal<void(Actor const&)> on_sleep;
  static xbt::signal<void(Actor const&)> on_wake_up;
  static xbt::signal<void(const Actor&, const Host& previous_location)> on_host_change;
  static xbt::signal<void(Actor const&)> on_termination;
  static xbt::signal<void(Actor const&)> on_destruction;

public:
  /** Add a callback fired when a new actor has been created **/
  static void on_creation_cb(const std::function<void(Actor&)>& cb) { on_creation.connect(cb); }
  /** Add a callback fired when an actor has been suspended**/
  static void on_suspend_cb(const std::function<void(Actor const&)>& cb) { on_suspend.connect(cb); }
  /** Add a callback fired when an actor has been resumed **/
  static void on_resume_cb(const std::function<void(Actor const&)>& cb) { on_resume.connect(cb); }
  /** Add a callback fired when an actor starts sleeping **/
  static void on_sleep_cb(const std::function<void(Actor const&)>& cb) { on_sleep.connect(cb); }
  /** Add a callback fired when an actor wakes up from a sleep **/
  static void on_wake_up_cb(const std::function<void(Actor const&)>& cb) { on_wake_up.connect(cb); }
  /** Add a callback fired when an actor is has been migrated to another host **/
  static void on_host_change_cb(const std::function<void(const Actor&, const Host& previous_location)>& cb)
  {
    on_host_change.connect(cb);
  }

  /** Add a callback fired when an actor terminates its code.
   *  @beginrst
   *  The actor may continue to exist if it is still referenced in the simulation, but it's not active anymore.
   *  If you want to free extra data when the actor's destructor is called, use :cpp:func:`Actor::on_destruction_cb`.
   *  If you want to register to the termination of a given actor, use :cpp:func:`this_actor::on_exit()` instead.
   *  @endrst
   */
  static void on_termination_cb(const std::function<void(Actor const&)>& cb) { on_termination.connect(cb); }
  /** Add a callback fired when an actor is about to disappear (its destructor was called).
   *  This signal is fired for any destructed actor, which is mostly useful when designing plugins and extensions.
   *  If you want to react to the end of the actor's code, use Actor::on_termination instead.
   *  If you want to register to the termination of a given actor, use this_actor::on_exit() instead.*/
  static void on_destruction_cb(const std::function<void(Actor const&)>& cb) { on_destruction.connect(cb); }

  /** Create an actor from a @c std::function<void()>.
   *  If the actor is restarted, it gets a fresh copy of the function. 
   *  @verbatim embed:rst:inline See the :ref:`example <s4u_ex_actors_create>`. @endverbatim */
  static ActorPtr create(const std::string& name, s4u::Host* host, const std::function<void()>& code);
  /** Create an actor, but don't start it yet.
   *
   * This is useful to set some properties or extension before actually starting it */
  static ActorPtr init(const std::string& name, s4u::Host* host);
  ActorPtr set_stacksize(unsigned stacksize);
  /** Start a previously initialized actor */
  ActorPtr start(const std::function<void()>& code);

  template <class F> ActorPtr start(F code) { return start(std::function<void()>(std::move(code))); }

  template <class F, class... Args,
  // This constructor is enabled only if the call code(args...) is valid:
#ifndef DOXYGEN /* breathe seem to choke on function signatures in template parameter, see breathe#611 */
            typename = typename std::result_of_t<F(Args...)>
#endif
            >
  ActorPtr start(F code, Args... args)
  {
    return start(std::bind(std::move(code), std::move(args)...));
  }

  ActorPtr start(const std::function<void()>& code, std::vector<std::string> args);

  /** Create an actor from a callable thing. 
   *  @verbatim embed:rst:inline See the :ref:`example <s4u_ex_actors_create>`. @endverbatim */
  template <class F> static ActorPtr create(const std::string& name, s4u::Host* host, F code)
  {
    return create(name, host, std::function<void()>(std::move(code)));
  }

  /** Create an actor using a callable thing and its arguments.
   *
   * Note that the arguments will be copied, so move-only parameters are forbidden. 
   * @verbatim embed:rst:inline See the :ref:`example <s4u_ex_actors_create>`. @endverbatim */

  template <class F, class... Args,
            // This constructor is enabled only if the call code(args...) is valid:
#ifndef DOXYGEN /* breathe seem to choke on function signatures in template parameter, see breathe#611 */
            typename = typename std::result_of_t<F(Args...)>
#endif
            >
  static ActorPtr create(const std::string& name, s4u::Host* host, F code, Args... args)
  {
    return create(name, host, std::bind(std::move(code), std::move(args)...));
  }

  /** Create actor from function name and a vector of strings as arguments. 
   *  @verbatim embed:rst:inline See the :ref:`example <s4u_ex_actors_create>`. @endverbatim */
  static ActorPtr create(const std::string& name, s4u::Host* host, const std::string& function,
                         std::vector<std::string> args);

  // ***** Methods *****
  /** This actor will be automatically terminated when the last non-daemon actor finishes.
   *
   * Daemons are killed as soon as the last regular actor disappears. If another regular actor
   * gets restarted later on by a timer or when its host reboots, the daemons do not get restarted.
   **/
  Actor* daemonize();

  /** Returns whether or not this actor has been daemonized or not **/
  bool is_daemon() const;
  static bool is_maestro();

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

  /** Suspend an actor, that is blocked until resumed by another actor. */
  void suspend();

  /** Resume an actor that was previously suspended */
  void resume();

  /** Returns true if the actor is suspended. */
  bool is_suspended() const;

  /** If set to true, the actor will automatically restart when its host reboots.
   *
   * Some elements of the actor are remembered over reboots: name, host, properties, the on_exit functions, whether it
   * is daemonized and whether it should automatically restart when its host reboots. Note that the state after reboot
   * is the one when set_auto_restart() is called.
   *
   * If you daemonize your actor after marking it auto_restart, then the new actor after rebooot will not be a daemon.
   *
   * The on_exit functions are the one defined when the actor dies, not the ones given when it was marked auto_restart
   * (sorry for the inconsistency -- speak to us if it's too hard to bear).
   */
  Actor* set_auto_restart(bool autorestart = true);
  /** Returns the number of reboots that this actor did. Before the first reboot, this function returns 0. */
  int get_restart_count() const;

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
  double get_kill_time() const;

  /** @brief Moves the actor to another host
   *
   * If the actor is currently blocked on an execution activity, the activity is also
   * migrated to the new host. If it's blocked on another kind of activity, an error is
   * raised as the mandated code is not written yet. Please report that bug if you need it.
   *
   * Asynchronous activities started by the actor are not migrated automatically, so you have
   * to take care of this yourself (only you knows which ones should be migrated).
   */
  void set_host(Host* new_host);

  /** Ask the actor to die.
   *
   * Any blocking activity will be canceled, and it will be rescheduled to free its memory.
   * Being killed is not something that actors can defer or avoid.
   */
  void kill();

  /** Retrieves the actor that have the given PID (or nullptr if not existing) */
  static ActorPtr by_pid(aid_t pid);

  /** Wait for the actor to finish.
   *
   * Blocks the calling actor until the joined actor is terminated. If actor alice executes bob.join(), then alice is
   * blocked until bob terminates.
   */
  void join() const;

  /** Wait for the actor to finish, or for the timeout to elapse.
   *
   * Blocks the calling actor until the joined actor is terminated. If actor alice executes bob.join(), then alice is
   * blocked until bob terminates.
   */
  void join(double timeout) const;
  /** Kill that actor and restart it from start. */
  Actor* restart();

  /** Kill all actors (but the issuer). Being killed is not something that actors can delay or avoid. */
  static void kill_all();

  /** Returns the internal implementation of this actor */
  kernel::actor::ActorImpl* get_impl() const { return pimpl_; }

  /** Retrieve the list of properties for that actor */
  const std::unordered_map<std::string, std::string>*
  get_properties() const; // FIXME: do not export the map, but only the keys or something

  /** Retrieve the property value (or nullptr if not set) */
  const char* get_property(const std::string& key) const;

  /** Set a property (old values will be overwritten) */
  void set_property(const std::string& key, const std::string& value);
};

}} // namespace simgrid::s4u


#endif /* SIMGRID_S4U_ACTOR_HPP */
