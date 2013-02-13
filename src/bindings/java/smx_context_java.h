/* Copyright (c) 2009, 2010, 2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_CONTEXT_JAVA_H
#define _XBT_CONTEXT_JAVA_H

#include <xbt/misc.h>
#include <simgrid/simix.h>
#include <xbt/xbt_os_thread.h>

#include "jmsg.h"
#include "jmsg_process.h"

SG_BEGIN_DECL()

typedef struct s_smx_ctx_java {
  s_smx_ctx_base_t super;       /* Fields of super implementation */
  jobject jprocess;             /* the java process instance bound with the msg process structure */
  JNIEnv *jenv;                 /* jni interface pointer associated to this thread */
  xbt_os_thread_t thread;
  xbt_os_sem_t begin;           /* this semaphore is used to schedule/yield the process  */
  xbt_os_sem_t end;             /* this semaphore is used to schedule/unschedule the process   */
} s_smx_ctx_java_t, *smx_ctx_java_t;

void SIMIX_ctx_java_factory_init(smx_context_factory_t *factory);
void smx_ctx_java_stop(smx_context_t context);
smx_context_t smx_ctx_java_self(void);
SG_END_DECL()

#endif                          /* !_XBT_CONTEXT_JAVA_H */
