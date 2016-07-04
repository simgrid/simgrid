/* Copyright (c) 2006-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_ACTOR_HPP
#define SIMGRID_S4U_ACTOR_HPP

#include <atomic>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/intrusive_ptr.hpp>

#include <xbt/base.h>
#include <xbt/functional.hpp>

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
 * A resource will not get used if you don't schedule activities on them. This is the code of Actors that create and schedule these activities.
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
 * %As in the <a href="http://en.cppreference.com/w/cpp/thread">C++11
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

/** @brief Simulation Agent (see \ref s4u_actor)*/
XBT_PUBLIC_CLASS Actor {
  friend Mailbox;
  friend simgrid::simix::Process;
  smx_process_t pimpl_ = nullptr;

  /** Wrap a (possibly non-copyable) single-use task into a `std::function` */
  template<class F, class... Args>
  static std::function<void()> wrap_task(F f, Args... args)
  {
    typedef decltype(f(std::move(args)...)) R;
    auto task = std::make_shared<simgrid::xbt::Task<R()>>(
      simgrid::xbt::makeTask(std::move(f), std::move(args)...));
    return [=] {
      (*task)();
    };
  }

  Actor(smx_process_t pimpl) : pimpl_(pimpl) {}

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
  using Ptr = boost::intrusive_ptr<Actor>;

  // ***** Actor creation *****

  /** Create an actor using a function
   *
   *  If the actor is restarted, the actor has a fresh copy of the function.
   */
  static Ptr createActor(const char* name, s4u::Host *host, double killTime, std::function<void()> code);

  static Ptr createActor(const char* name, s4u::Host *host, std::function<void()> code)
  {
    return createActor(name, host, -1.0, std::move(code));
  }

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
  static Ptr createActor(const char* name, s4u::Host *host, F code, Args... args)
  {
    return createActor(name, host, wrap_task(std::move(code), std::move(args)...));
  }

  // Create actor from function name:

  static Ptr createActor(const char* name, s4u::Host *host, double killTime,
    const char* function, std::vector<std::string> args);

  static Ptr createActor(const char* name, s4u::Host *host, const char* function,
      std::vector<std::string> args)
  {
    return createActor(name, host, -1.0, function, std::move(args));
  }

  // ***** Methods *****

  /** Retrieves the actor that have the given PID (or NULL if not existing) */
  //static Actor *byPid(int pid); not implemented

  /** Retrieves the name of that actor */
  const char* getName();
  /** Retrieves the host on which that actor is running */
  s4u::Host *getHost();
  /** Retrieves the PID of that actor */
  int getPid();

  /** If set to true, the actor will automatically restart when its host reboots */
  void setAutoRestart(bool autorestart);
  /** Sets the time at which that actor should be killed */
  void setKillTime(double time);
  /** Retrieves the time at which that actor will be killed (or -1 if not set) */
  double getKillTime();

  /** Ask the actor to die.
   *
   * It will only notice your request when doing a simcall next time (a communication or similar).
   * SimGrid sometimes have issues when you kill actors that are currently communicating and such. We are working on it to fix the issues.
   */
  void kill();

  static void kill(int pid);
  static Ptr forPid(int pid);
  
  /**
   * Wait for the actor to finish.
   */ 
  void join();
  
  // Static methods on all actors:

  /** Ask kindly to all actors to die. Only the issuer will survive. */
  static void killAll();
  
  smx_process_t getInferior();
};

using ActorPtr = Actor::Ptr;

/** @ingroup s4u_api
 *  @brief Static methods working on the current actor (see @ref s4u::Actor) */
namespace this_actor {

  /** Block the actor sleeping for that amount of seconds (may throws hostFailure) */
  XBT_PUBLIC(void) sleep(double duration);

  /** Block the actor, computing the given amount of flops */
  XBT_PUBLIC(e_smx_state_t) execute(double flop);

  /** Block the actor until it gets a message from the given mailbox.
   *
   * See \ref Comm for the full communication API (including non blocking communications).
   */
  XBT_PUBLIC(void*) recv(Mailbox &chan);

  /** Block the actor until it delivers a message of the given simulated size to the given mailbox
   *
   * See \ref Comm for the full communication API (including non blocking communications).
  */
  XBT_PUBLIC(void) send(Mailbox &chan, void*payload, size_t simulatedSize);
  
  /**
   * Return the PID of the current actor.
   */
  XBT_PUBLIC(int) getPid();

};

/** @} */

}} // namespace simgrid::s4u


#endif /* SIMGRID_S4U_ACTOR_HPP */
