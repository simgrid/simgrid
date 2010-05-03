/* Functions related to the java task instances.                            */

/* Copyright (c) 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "jmsg.h"
#include "jmsg_task.h"
#include "jxbt_utilities.h"

void jtask_bind(jobject jtask, m_task_t task, JNIEnv * env)
{
  jfieldID id = jxbt_get_sfield(env, "simgrid/msg/Task", "bind", "J");

  if (!id)
    return;

  (*env)->SetLongField(env, jtask, id, (jlong) (long) (task));
}

m_task_t jtask_to_native_task(jobject jtask, JNIEnv * env)
{
  jfieldID id = jxbt_get_sfield(env, "simgrid/msg/Task", "bind", "J");

  if (!id)
    return NULL;

  return (m_task_t) (long) (*env)->GetLongField(env, jtask, id);
}

jboolean jtask_is_valid(jobject jtask, JNIEnv * env)
{
  jfieldID id = jxbt_get_sfield(env, "simgrid/msg/Task", "bind", "J");

  if (!id)
    return JNI_FALSE;

  return (*env)->GetLongField(env, jtask, id) ? JNI_TRUE : JNI_FALSE;
}
