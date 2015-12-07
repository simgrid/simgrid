/* Copyright (c) 2009-2010, 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_JAVA_JAVA_CONTEXT_HPP
#define SIMGRID_JAVA_JAVA_CONTEXT_HPP

#include <jni.h>

#include <xbt/misc.h>
#include <simgrid/simix.h>
#include <xbt/xbt_os_thread.h>

#include "src/simix/smx_private.hpp"

#include "jmsg.h"
#include "jmsg_process.h"

namespace simgrid {
namespace java {

class JavaContext;
class JavacontextFactory;

class JavaContext : public simgrid::simix::Context {
public:
  // The java process instance bound with the msg process structure:
  jobject jprocess;
  // JNI interface pointer associated to this thread:
  JNIEnv *jenv;
  xbt_os_thread_t thread;
  // Sempahore used to schedule/yield the process:
  xbt_os_sem_t begin;
  // Semaphore used to schedule/unschedule the process:
  xbt_os_sem_t end;
public:
  friend class JavaContextFactory;
  JavaContext(xbt_main_func_t code,
          int argc, char **argv,
          void_pfn_smxprocess_t cleanup_func,
          smx_process_t process);
  ~JavaContext() override;
  void stop() override;
  void suspend() override;
  void resume();
private:
  static void* wrapper(void *data);
};

class JavaContextFactory : public simgrid::simix::ContextFactory {
public:
  JavaContextFactory();
  ~JavaContextFactory() override;
  JavaContext* self() override;
  JavaContext* create_context(
    xbt_main_func_t, int, char **, void_pfn_smxprocess_t,
    smx_process_t process
    ) override;
  void run_all() override;
};

XBT_PRIVATE simgrid::simix::ContextFactory* java_factory();

}
}

#endif                          /* !_XBT_CONTEXT_JAVA_H */
