/*
 * $Id$
 *
 * Copyright 2006,2007 Arnaud Legrand, Martin Quinson, Malek Cherier         
 * All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. 
 */

#ifndef XBT_JCONTEXT_H
#define XBT_JCONTEXT_H


#include "xbt/sysdep.h"
#include "xbt/swag.h"
#include "xbt/dynar.h" /* void_f_pvoid_t */
#include "portable.h"  /* loads context system definitions */
#include "xbt/context.h"
#include "xbt/ex.h"
#include "xbt/xbt_os_thread.h"
#include <jni.h>

SG_BEGIN_DECL()

/* pointers to schedule and unschedule functions */
typedef void (*pfn_schedule_t)(xbt_context_t);
typedef pfn_schedule_t pfn_unschedule_t;

/*
 * This function is called by the simulator to terminate a java process instance.
 *
 * @param context	The context of the java process to terminate.
 * @param value		not used.
 */
void __context_exit(xbt_context_t context ,int value);

/*
 * This function is called by the java process to indicate it's done
 * 
 * @param context	The context of the process concerning by the ending.
 * @parma value		not used.
 * @param env		The jni interface pointer of the process.
 */
void jcontext_exit(xbt_context_t context ,int value,JNIEnv* env);

SG_END_DECL()

#endif	/* !_XBT_CONTEXT_PRIVATE_H */
