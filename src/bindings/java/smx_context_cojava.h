/* Copyright (c) 2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_CONTEXT_COJAVA_H
#define _XBT_CONTEXT_COJAVA_H

#include <xbt/misc.h>
#include <simgrid/simix.h>
#include <xbt/xbt_os_thread.h>

#include "jmsg.h"
#include "jmsg_process.h"

SG_BEGIN_DECL()

typedef struct s_smx_ctx_cojava {
  s_smx_ctx_base_t super;       /* Fields of super implementation */
  jobject jprocess;             /* the java process instance binded with the msg process structure */
  JNIEnv *jenv;                 /* jni interface pointer associated to this thread */
  jobject jcoroutine;						/* java coroutine object */
  int bound:1;
} s_smx_ctx_cojava_t, *smx_ctx_cojava_t;

void SIMIX_ctx_cojava_factory_init(smx_context_factory_t *factory);
void smx_ctx_cojava_stop(smx_context_t context);
smx_context_t smx_ctx_cojava_self(void);
SG_END_DECL()

#endif                          /* !_XBT_CONTEXT_JAVA_H */
