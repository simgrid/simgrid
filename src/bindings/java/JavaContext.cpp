/* Context switching within the JVM.                                        */

/* Copyright (c) 2009-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "JavaContext.hpp"
#include "jxbt_utilities.hpp"
#include "src/simix/smx_private.hpp"
#include "xbt/ex.hpp"

#include <functional>
#include <utility>

extern "C" JavaVM* __java_vm;

XBT_LOG_NEW_DEFAULT_CATEGORY(java, "MSG for Java(TM)");

namespace simgrid {
namespace kernel {
namespace context {

ContextFactory* java_factory()
{
  XBT_INFO("Using regular java threads.");
  return new JavaContextFactory();
}

JavaContextFactory::JavaContextFactory(): ContextFactory("JavaContextFactory")
{
}

JavaContextFactory::~JavaContextFactory()=default;

JavaContext* JavaContextFactory::self()
{
  return static_cast<JavaContext*>(xbt_os_thread_get_extra_data());
}

JavaContext* JavaContextFactory::create_context(
  std::function<void()> code,
  void_pfn_smxprocess_t cleanup, smx_actor_t process)
{
  return this->new_context<JavaContext>(std::move(code), cleanup, process);
}

void JavaContextFactory::run_all()
{
  for (smx_actor_t const& process : simgrid::simix::process_get_runnable()) {
    static_cast<JavaContext*>(process->context)->resume();
  }
}

JavaContext::JavaContext(std::function<void()> code,
        void_pfn_smxprocess_t cleanup_func,
        smx_actor_t process)
  : Context(std::move(code), cleanup_func, process)
{
  static int thread_amount=0;
  thread_amount++;

  /* If the user provided a function for the process then use it otherwise is the context for maestro */
  if (has_code()) {
    this->jprocess = nullptr;
    this->begin = xbt_os_sem_init(0);
    this->end = xbt_os_sem_init(0);

    try {
       this->thread = xbt_os_thread_create(
         nullptr, JavaContext::wrapper, this, nullptr);
    }
    catch (xbt_ex& ex) {
      char* str = bprintf(
        "Failed to create context #%d. You may want to switch to Java coroutines to increase your limits (error: %s)."
        "See the Install section of simgrid-java documentation (in doc/install.html) for more on coroutines.",
        thread_amount, ex.what());
      xbt_ex new_exception(XBT_THROW_POINT, str);
      new_exception.category = ex.category;
      new_exception.value = ex.value;
      std::throw_with_nested(std::move(new_exception));
    }
  } else {
    this->thread = nullptr;
    xbt_os_thread_set_extra_data(this);
  }
}

JavaContext::~JavaContext()
{
  if (this->thread) {
    // We are not in maestro context
    xbt_os_thread_join(this->thread, nullptr);
    xbt_os_sem_destroy(this->begin);
    xbt_os_sem_destroy(this->end);
  }
}

void* JavaContext::wrapper(void *data)
{
  JavaContext* context = static_cast<JavaContext*>(data);
  xbt_os_thread_set_extra_data(context);
  //Attach the thread to the JVM

  JNIEnv *env;
  XBT_ATTRIB_UNUSED jint error = __java_vm->AttachCurrentThread((void**)&env, nullptr);
  xbt_assert((error == JNI_OK), "The thread could not be attached to the JVM");
  context->jenv = env;
  //Wait for the first scheduling round to happen.
  xbt_os_sem_acquire(context->begin);
  //Create the "Process" object if needed.
  (*context)();
  context->stop();
  return nullptr;
}

void JavaContext::stop()
{
  /* I was asked to die (either with kill() or because of a failed element) */
  if (this->iwannadie) {
    this->iwannadie = 0;
    JNIEnv *env = get_current_thread_env();
    XBT_DEBUG("Gonna launch Killed Error");
    // When the process wants to stop before its regular end, we should cut its call stack quickly.
    // The easiest way to do so is to raise an exception that will be catched in its top calling level.
    //
    // For that, we raise a ProcessKilledError that is catched in Process::run() (in msg/Process.java)
    //
    // Throwing a Java exception to stop the actor may be an issue for pure C actors
    // (as the ones created for the VM migration). The Java exception will not be catched anywhere.
    // Bad things happen currently if these actors get killed, unfortunately.
    jxbt_throw_by_name(env, "org/simgrid/msg/ProcessKilledError",
                       std::string("Process ") + this->process()->getCname() + " killed from file JavaContext.cpp");

    // (remember that throwing a java exception from C does not break the C execution path.
    //  Instead, it marks the exception to be raised when returning to the Java world and
    //  continues to execute the C function until it ends or returns).

    // Once the Java stack is marked to be unrolled, a C cancel_error is raised to kill the simcall
    //  on which the killed actor is blocked (if any).
    // Not doing so would prevent the actor to notice that it's dead, leading to segfaults when it wakes up.
    // This is dangerous: if the killed actor is not actually blocked, the cancel_error will not get catched.
    // But it should be OK in most cases:
    //  - If I kill myself, I must do so with Process.kill().
    //    The binding of this function in jmsg_process.cpp adds a try/catch around the MSG_process_kill() leading to us
    //  - If I kill someone else that is blocked, the cancel_error will unblock it.
    //
    // A problem remains probably if I kill a process that is ready_to_run in the same scheduling round.
    // I guess that this will kill the whole simulation because the victim does not catch the exception.
    // The only solution I see to that problem would be to completely rewrite the process killing sequence
    // (also in C) so that it's based on regular C++ exceptions that would be catched anyway.
    // In other words, we need to do in C++ what we do in Java for sake of uniformity.
    //
    // Plus, C++ RAII would work in that case, too.

    XBT_DEBUG("Trigger a cancel error at the C level");
    THROWF(cancel_error, 0, "process cancelled");
  } else {
    Context::stop();
    /* detach the thread and kills it */
    JNIEnv *env = this->jenv;
    env->DeleteGlobalRef(this->jprocess);
    XBT_ATTRIB_UNUSED jint error = __java_vm->DetachCurrentThread();
    xbt_assert((error == JNI_OK), "The thread couldn't be detached.");
    xbt_os_sem_release(this->end);
    xbt_os_thread_exit(nullptr);
  }
}

void JavaContext::suspend()
{
  xbt_os_sem_release(this->end);
  xbt_os_sem_acquire(this->begin);
}

// FIXME: inline those functions
void JavaContext::resume()
{
  xbt_os_sem_release(this->begin);
  xbt_os_sem_acquire(this->end);
}

}}} // namespace simgrid::kernel::context
