/* context_java - implementation of context switching for java threads */

/* Copyright (c) 2009-2010, 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <functional>
#include <utility>

#include <xbt/function_types.h>
#include <simgrid/simix.h>
#include <xbt/ex.h>
#include "JavaContext.hpp"
#include "jxbt_utilities.h"
#include "xbt/dynar.h"
#include "../../simix/smx_private.h"
extern JavaVM *__java_vm;

XBT_LOG_NEW_DEFAULT_CATEGORY(jmsg, "MSG for Java(TM)");

namespace simgrid {
namespace java {

simgrid::simix::ContextFactory* java_factory()
{
  XBT_INFO("Using regular java threads.");
  return new JavaContextFactory();
}

JavaContextFactory::JavaContextFactory()
  : ContextFactory("JavaContextFactory")
{
}

JavaContextFactory::~JavaContextFactory()
{
}

JavaContext* JavaContextFactory::self()
{
  return (JavaContext*) xbt_os_thread_get_extra_data();
}

JavaContext* JavaContextFactory::create_context(
  std::function<void()> code,
  void_pfn_smxprocess_t cleanup, smx_process_t process)
{
  return this->new_context<JavaContext>(std::move(code), cleanup, process);
}

void JavaContextFactory::run_all()
{
  xbt_dynar_t processes = SIMIX_process_get_runnable();
  smx_process_t process;
  unsigned int cursor;
  xbt_dynar_foreach(processes, cursor, process) {
    static_cast<JavaContext*>(SIMIX_process_get_context(process))->resume();
  }
}

JavaContext::JavaContext(std::function<void()> code,
        void_pfn_smxprocess_t cleanup_func,
        smx_process_t process)
  : Context(std::move(code), cleanup_func, process)
{
  static int thread_amount=0;
  thread_amount++;

  /* If the user provided a function for the process then use it otherwise is the context for maestro */
  if (has_code()) {
    this->jprocess = nullptr;
    this->begin = xbt_os_sem_init(0);
    this->end = xbt_os_sem_init(0);

    TRY {
       this->thread = xbt_os_thread_create(
         nullptr, JavaContext::wrapper, this, nullptr);
    }
    CATCH_ANONYMOUS {
      RETHROWF("Failed to create context #%d. You may want to switch to Java coroutines to increase your limits (error: %s)."
               "See the Install section of simgrid-java documentation (in doc/install.html) for more on coroutines.",
               thread_amount);
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
  JavaContext* context = (JavaContext*)data;
  xbt_os_thread_set_extra_data(context);
  //Attach the thread to the JVM

  JNIEnv *env;
  XBT_ATTRIB_UNUSED jint error =
    __java_vm->AttachCurrentThread((void **) &env, NULL);
  xbt_assert((error == JNI_OK), "The thread could not be attached to the JVM");
  context->jenv = get_current_thread_env();
  //Wait for the first scheduling round to happen.
  xbt_os_sem_acquire(context->begin);
  //Create the "Process" object if needed.
  (*context)();
  context->stop();
  return nullptr;
}

void JavaContext::stop()
{
  /* I am the current process and I am dying */
  if (this->iwannadie) {
    this->iwannadie = 0;
    JNIEnv *env = get_current_thread_env();
    XBT_DEBUG("Gonna launch Killed Error");
    // TODO Adrien, if the process has not been created at the java layer, why should we raise the exception/error at the java level (this happens
    // for instance during the migration process that creates at the C level two processes: one on the SRC node and one on the DST node, if the DST process is killed.
    // it is not required to raise an exception at the JAVA level, the low level should be able to manage such an issue correctly but this is not the case right now unfortunately ...
    // TODO it will be nice to have the name of the process to help the end-user to know which Process has been killed
   // jxbt_throw_by_name(env, "org/simgrid/msg/ProcessKilledError", bprintf("Process %s killed :) (file smx_context_java.c)", MSG_process_get_name( (msg_process_t)context) ));
    jxbt_throw_by_name(env, "org/simgrid/msg/ProcessKilledError",
      bprintf("Process %s killed :) (file JavaContext.cpp)",
      simcall_process_get_name((smx_process_t) SIMIX_context_get_process(this))) );
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
    xbt_os_thread_exit(NULL);
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

}
}
