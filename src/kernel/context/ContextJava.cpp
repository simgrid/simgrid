/* Context switching within the JVM.                                        */

/* Copyright (c) 2009-2024. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/context/ContextJava.hpp"
#include "simgrid/Exception.hpp"
#include "simgrid/s4u/Actor.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "xbt/backtrace.hpp"
#include "xbt/log.h"

#include <functional>
#include <utility>

extern JavaVM* simgrid_cached_jvm;

XBT_LOG_NEW_DEFAULT_CATEGORY(java, "SimGrid for Java(TM)");

namespace simgrid::kernel::context {

JavaContextFactory::~JavaContextFactory()
{
  simgrid_cached_jvm->DetachCurrentThread(); // Maestro needs to be detached
}

Context* JavaContextFactory::create_context(std::function<void()>&& code, actor::ActorImpl* actor)
{
  return this->new_context<JavaContext>(std::move(code), actor);
}

void JavaContextFactory::run_all(std::vector<actor::ActorImpl*> const& actors)
{
  SerialThreadContext::run_all(actors);
}

JavaContext::JavaContext(std::function<void()>&& code, actor::ActorImpl* actor)
    : SerialThreadContext(std::move(code), actor, false /* not maestro */)
{
  /* ThreadContext already does all we need */
}

void JavaContext::initialized()
{
  Context::set_current(this); // We need to attach it also for maestro, in contrary to our ancestor

  // Attach the thread to the JVM
  JNIEnv* env;
  xbt_assert(simgrid_cached_jvm->AttachCurrentThread((void**)&env, NULL) == JNI_OK,
             "The thread could not be attached to the JVM");
  XBT_DEBUG("Attach thread %p (%s)", this, get_actor()->get_cname()); // Maestro does not take this execution path
  this->jenv_ = env;
}

void JavaContext::stop()
{
  XBT_DEBUG("Stopping %s", get_actor()->get_cname());
  this->get_actor()->cleanup_from_self();
 // sthread_disable();

  /* Unregister the thread from the JVM */
  jint error = simgrid_cached_jvm->DetachCurrentThread();
  if (error != JNI_OK) {
    /* This is probably a Java thread, ie an actor not created from the XML (and thus from the C++),
     * but from Java with something like new Process().start().
     *
     * We should not even try to detach such threads. Instead, we throw a Java exception that will raise up
     * until run_jprocess(), IIUC.
     */
    // Cache the exception class
    static jclass klass = 0;
    if (klass == 0) {
      klass = jenv_->FindClass("org/simgrid/s4u/ForcefulKillException");
      xbt_assert(klass, "Class not found");
    }

    jenv_->ThrowNew(klass, "Actor killed");
  }

  simgrid::ForcefulKillException::do_throw(); // clean RAII variables with the dedicated exception
}

} // namespace simgrid::kernel::context
