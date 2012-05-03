/* Functions related to the java task instances.                            */

/* Copyright (c) 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "jmsg.h"
#include "jmsg_task.h"
#include "jxbt_utilities.h"

#include <msg/msg.h>
XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(jmsg);

void jtask_bind(jobject jtask, m_task_t task, JNIEnv * env)
{
  jfieldID id = jxbt_get_sfield(env, "org/simgrid/msg/Task", "bind", "J");

  if (!id)
    return;

  (*env)->SetLongField(env, jtask, id, (jlong) (long) (task));
}

m_task_t jtask_to_native_task(jobject jtask, JNIEnv * env)
{
  jfieldID id = jxbt_get_sfield(env, "org/simgrid/msg/Task", "bind", "J");

  if (!id)
    return NULL;

  return (m_task_t) (long) (*env)->GetLongField(env, jtask, id);
}

jboolean jtask_is_valid(jobject jtask, JNIEnv * env)
{
  jfieldID id = jxbt_get_sfield(env, "org/simgrid/msg/Task", "bind", "J");

  if (!id)
    return JNI_FALSE;

  return (*env)->GetLongField(env, jtask, id) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_Task_irecvBind(JNIEnv * env, jclass cls, jobject jcomm, jstring jmailbox) {
	msg_comm_t comm;
	const char *mailbox;
	//pointer to store the task object pointer.
	m_task_t *task = xbt_new(m_task_t,1);
	*task = NULL;
	/* There should be a cache here */
	jfieldID id = jxbt_get_sfield(env, "org/simgrid/msg/Comm", "bind", "J");
	jfieldID id_task = jxbt_get_sfield(env, "org/simgrid/msg/Comm", "bindTask", "J");

	if (!id || !id_task)
		return;


	mailbox = (*env)->GetStringUTFChars(env, jmailbox, 0);

	comm = MSG_task_irecv(task,mailbox);

	(*env)->SetLongField(env, jcomm, id, (jlong) (long)(comm));
	(*env)->SetLongField(env, jcomm, id_task, (jlong) (long)(task));

	(*env)->ReleaseStringUTFChars(env, jmailbox, mailbox);
}
