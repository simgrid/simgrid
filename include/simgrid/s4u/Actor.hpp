/* Copyright (c) 2006-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_ACTOR_HPP
#define SIMGRID_S4U_ACTOR_HPP

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/intrusive_ptr.hpp>

#include <xbt/Extendable.hpp>
#include <xbt/base.h>
#include <xbt/functional.hpp>
#include <xbt/string.hpp>

#include <simgrid/chrono.hpp>
#include <simgrid/simix.h>
#include <simgrid/s4u/forward.hpp>

namespace simgrid {
namespace s4u {

/** @ingroup s4u_api
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
 * #include "s4u/actor.hpp"
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
 * #include "s4u/actor.hpp"
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
 *   <!-- Start a process called 'master' on the host called 'Tremblay' -->
 *   <process host="Tremblay" function="master">
 *      <!-- Here come the parameter that you want to feed to this instance of master -->
 *      <argument value="20"/>        <!-- argv[1] -->
 *      <argument value="50000000"/>  <!-- argv[2] -->
 *      <argument value="1000000"/>   <!-- argv[3] -->
 *      <argument value="5"/>         <!-- argv[4] -->
 *   </process>
 *
 *   <!-- Start a process called 'worker' on the host called 'Jupiter' -->
 *   <process host="Jupiter" function="worker"/> <!-- Don't provide any parameter ->>
 *
 * </platform>
 * @endcode
 *
 *  @{
 */

/** @brief Simulation Agent */
XBT_PUBLIC_CLASS Actor : public simgrid::xbt::Extendable<Actor>
{
  friend Mailbox;
  friend simgrid::simix::ActorImpl;
  friend simgrid::kernel::activity::MailboxImpl;
  simix::ActorImpl* pimpl_ = nullptr;

  /** Wrap a (possibly non-copyable) single-use task into a `std::function` */
  template<class F, class... Args>
  static std::function<void()> wrap_task(F f, Args... args)
  {
    typedef decltype(f(std::move(args)...)) R;
    auto task = std::make_shared<simgrid::xbt::Task<R()>>(
      simgrid::xbt::makeTask(std::move(f), std::move(args)...));
    return [task] { (*task)(); };
  }

  explicit Actor(smx_actor_t pimpl) : pimpl_(pimpl) {}

public:

  // ***** No copy *****
  Actor(Actor const&) = delete;
  Actor& operator=(Actor const&) = delete;

  // ***** Reference count (delegated to pimpl_) *****
  friend void intrusive_ptr_add_ref(Actor* actor)
  {
    xbt_assert(actor != nullptr);
    SIMIX_process_ref(actor->pimpl_);
  }
  friend void intrusive_ptr_release(Actor* actor)
  {
    xbt_assert(actor != nullptr);
    SIMIX_process_unref(actor->pimpl_);
  }

  // ***** Actor creation *****
  /** Retrieve a reference to myself */
  static ActorPtr self();

  /** Create an actor using a function
   *
   *  If the actor is restarted, the actor has a fresh copy of the function.
   */
  static ActorPtr createActor(const char* name, s4u::Host* host, std::function<void()> code);

  /** Create an actor using code
   *
   *  Using this constructor, move-only type can be used. The consequence is
   *  that we cannot copy the value and restart the process in its initial
   *  state. In order to use auto-restart, an explicit `function` must be passed
   *  instead.
   */
  template<class F, class... Args,
    // This constructor is enabled only if the call code(args...) is valid:
    typename = typename std::result_of<F(Args...)>::type
    >
  static ActorPtr createActor(const char* name, s4u::Host *host, F code, Args... args)
  {
    return createActor(name, host, wrap_task(std::move(code), std::move(args)...));
  }

  // Create actor from function name:

  static ActorPtr createActor(const char* name, s4u::Host* host, const char* function, std::vector<std::string> args);

  // ***** Methods *****

  /** Retrieves the name of that actor as a C string */
  const char* cname();
  /** Retrieves the name of that actor as a C++ string */
  simgrid::xbt::string name();
  /** Retrieves the host on which that actor is running */
  s4u::Host* host();
  /** Retrieves the PID of that actor
   *
   * actor_id_t is an alias for unsigned long */
  aid_t pid();
  /** Retrieves the PPID of that actor
   *
   * actor_id_t is an alias for unsigned long */
  aid_t ppid();

  /** Suspend an actor by suspending the task on which it was waiting for the completion. */
  void suspend();

  /** Resume a suspended process by resuming the task on which it was waiting for the completion. */
  void resume();

  /** Returns true if the process is suspended. */
  int isSuspended();

  /** If set to true, the actor will automatically restart when its host reboots */
  void setAutoRestart(bool autorestart);

  /** Add a function to the list of "on_exit" functions for the current actor. The on_exit functions are the functions
   * executed when your actor is killed. You should use them to free the data used by your process.
   */
  void onExit(int_f_pvoid_pvoid_t fun, void* data);

  /** Sets the time at which that actor should be killed */
  void setKillTime(double time);
  /** Retrieves the time at which that actor will be killed (or -1 if not set) */
  double killTime();

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

  static void kill(aid_t pid);

  /** Retrieves the actor that have the given PID (or nullptr if not existing) */
  static ActorPtr byPid(aid_t pid);

  /** @brief Wait for the actor to finish.
   *
   * This blocks the calling actor until the actor on which we call join() is terminated
   */
  void join();
  
  // Static methods on all actors:

  /** Ask kindly to all actors to die. Only the issuer will survive. */
  static void killAll();
  static void killAll(int resetPid);

  /** Returns the internal implementation of this actor */
  simix::ActorImpl* getImpl();

  /** Retrieve the property value (or nullptr if not set) */
  const char* property(const char* key);
  void setProperty(const char* key, const char* value);
};

/** @ingroup s4u_api
 *  @brief Static methods working on the current actor (see @ref s4u::Actor) */
namespace this_actor {

  /** Block the actor sleeping for that amount of seconds (may throws hostFailure) */
  XBT_PUBLIC(void) sleep_for(double duration);
  XBT_PUBLIC(void) sleep_until(double timeout);

  template<class Rep, class Period>
  inline void sleep_for(std::chrono::duration<Rep, Period> duration)
  {
    auto seconds = std::chrono::duration_cast<SimulationClockDuration>(duration);
    this_actor::sleep_for(seconds.count());
  }
  template<class Duration>
  inline void sleep_until(const SimulationTimePoint<Duration>& timeout_time)
  {
    auto timeout_native = std::chrono::time_point_cast<SimulationClockDuration>(timeout_time);
    this_actor::sleep_until(timeout_native.time_since_epoch().count());
  }

  XBT_ATTRIB_DEPRECATED("Use sleep_for()")
  inline void sleep(double duration)
  {
    return sleep_for(duration);
  }

  /** Block the actor, computing the given amount of flops */
  XBT_PUBLIC(e_smx_state_t) execute(double flop);

  /** Block the actor until it gets a message from the given mailbox.
   *
   * See \ref Comm for the full communication API (including non blocking communications).
   */
  XBT_PUBLIC(void*) recv(MailboxPtr chan);
  XBT_PUBLIC(Comm&) irecv(MailboxPtr chan, void** data);

  /** Block the actor until it delivers a message of the given simulated size to the given mailbox
   *
   * See \ref Comm for the full communication API (including non blocking communications).
  */
  XBT_PUBLIC(void) send(MailboxPtr chan, void* payload, double simulatedSize);
  XBT_PUBLIC(void) send(MailboxPtr chan, void* payload, double simulatedSize, double timeout);

  XBT_PUBLIC(Comm&) isend(MailboxPtr chan, void* payload, double simulatedSize);

  /** @brief Returns the actor ID of the current actor (same as pid). */
  XBT_PUBLIC(aid_t) pid();

  /** @brief Returns the ancestor's actor ID of the current actor (same as ppid). */
  XBT_PUBLIC(aid_t) ppid();

  /** @brief Returns the name of the current actor. */
  XBT_PUBLIC(std::string) name();

  /** @brief Returns the name of the host on which the process is running. */
  XBT_PUBLIC(Host*) host();

  /** @brief Suspend the actor. */
  XBT_PUBLIC(void) suspend();

  /** @brief Resume the actor. */
  XBT_PUBLIC(void) resume();

  XBT_PUBLIC(int) isSuspended();

  /** @brief kill the actor. */
  XBT_PUBLIC(void) kill();

  /** @brief Add a function to the list of "on_exit" functions. */
  XBT_PUBLIC(void) onExit(int_f_pvoid_pvoid_t fun, void* data);

  /** @brief Migrate the actor to a new host. */
  XBT_PUBLIC(void) migrate(Host* new_host);
};

/** @} */

}} // namespace simgrid::s4u


#endif /* SIMGRID_S4U_ACTOR_HPP */
