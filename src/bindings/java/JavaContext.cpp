/* Context switching within the JVM.                                        */

/* Copyright (c) 2009-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "JavaContext.hpp"
#include "jxbt_utilities.hpp"
#include "simgrid/Exception.hpp"

#include <functional>
#include <utility>

extern JavaVM* __java_vm;

XBT_LOG_NEW_DEFAULT_CATEGORY(java, "MSG for Java(TM)");

namespace simgrid {
namespace kernel {
namespace context {

ContextFactory* java_factory()
{
  XBT_INFO("Using regular java threads.");
  return new JavaContextFactory();
}

JavaContextFactory::JavaContextFactory() : ContextFactory()
{
  xbt_assert(xbt::binary_name == "java");
}

JavaContextFactory::~JavaContextFactory()=default;

Context* JavaContextFactory::create_context(std::function<void()>&& code, actor::ActorImpl* actor)
{
  return this->new_context<JavaContext>(std::move(code), actor);
}

void JavaContextFactory::run_all()
{
  SerialThreadContext::run_all();
}

JavaContext::JavaContext(std::function<void()>&& code, smx_actor_t actor)
    : SerialThreadContext(std::move(code), actor, false /* not maestro */)
{
  /* ThreadContext already does all we need */
}

void JavaContext::start_hook()
{
  Context::set_current(this); // We need to attach it also for maestro, in contrary to our ancestor

  //Attach the thread to the JVM
  JNIEnv *env;
  xbt_assert(__java_vm->AttachCurrentThread((void**)&env, nullptr) == JNI_OK,
             "The thread could not be attached to the JVM");
  this->jenv_ = env;
}

void JavaContext::stop_hook()
{
    JNIEnv* env = this->jenv_;
    env->DeleteGlobalRef(this->jprocess_);
    jint error = __java_vm->DetachCurrentThread();
    if (error != JNI_OK) {
      /* This is probably a Java thread, ie an actor not created from the XML (and thus from the C++),
       * but from Java with something like new Process().start().
       *
       * We should not even try to detach such threads. Instead, we throw a Java exception that will raise up
       * until run_jprocess(), IIUC.
       */
      jxbt_throw_by_name(env, "org/simgrid/msg/ProcessKilledError", "Process killed");
      XBT_DEBUG("Cannot detach the current thread");
    }
}

}}} // namespace simgrid::kernel::context
