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

JNIEXPORT jobject JNICALL
Java_org_simgrid_msg_Task_irecv(JNIEnv * env, jclass cls, jstring jmailbox) {
	msg_comm_t comm;
	const char *mailbox;
	jclass comm_class;
	jmethodID cid;
	jfieldID id;
	jfieldID id_task;
	jfieldID id_receiving;
	//pointer to store the task object pointer.
	m_task_t *task = xbt_new(m_task_t,1);
	*task = NULL;
	/* There should be a cache here */
	comm_class = (*env)->FindClass(env, "org/simgrid/msg/Comm");
	cid = (*env)->GetMethodID(env, comm_class, "<init>", "()V");
	id = jxbt_get_sfield(env, "org/simgrid/msg/Comm", "bind", "J");
	id_task = jxbt_get_sfield(env, "org/simgrid/msg/Comm", "bindTask", "J");
	id_receiving = jxbt_get_sfield(env, "org/simgrid/msg/Comm", "receiving", "Z");


	if (!id || !id_task || !comm_class || !cid || !id_receiving) {
		jxbt_throw_native(env,bprintf("fieldID or methodID or class not found."));
		return NULL;
	}

	jobject jcomm = (*env)->NewObject(env, comm_class, cid);
	if (!jcomm) {
		jxbt_throw_native(env,bprintf("Can't create a Comm object."));
		return NULL;
	}

	mailbox = (*env)->GetStringUTFChars(env, jmailbox, 0);

	comm = MSG_task_irecv(task,mailbox);

	(*env)->SetLongField(env, jcomm, id, (jlong) (long)(comm));
	(*env)->SetLongField(env, jcomm, id_task, (jlong) (long)(task));
	(*env)->SetBooleanField(env, jcomm, id_receiving, JNI_TRUE);

	(*env)->ReleaseStringUTFChars(env, jmailbox, mailbox);

	return jcomm;
}

JNIEXPORT jobject JNICALL
Java_org_simgrid_msg_Task_isend(JNIEnv *env, jobject jtask, jstring jmailbox) {
	jclass comm_class;
	jmethodID cid;
	jfieldID id_bind;
	jfieldID id_bind_task;
	jfieldID id_receiving;
	jobject jcomm;
	const char *mailbox;
	m_task_t task;
	msg_comm_t comm;

	comm_class = (*env)->FindClass(env, "org/simgrid/msg/Comm");
	cid = (*env)->GetMethodID(env, comm_class, "<init>", "()V");
	id_bind = jxbt_get_sfield(env, "org/simgrid/msg/Comm", "bind", "J");
	id_bind_task = jxbt_get_sfield(env, "org/simgrid/msg/Comm", "bindTask", "J");
	id_receiving = jxbt_get_sfield(env, "org/simgrid/msg/Comm", "receiving", "Z");

	if (!comm_class || !cid || !id_bind || !id_bind_task || !id_receiving) return NULL;

	jcomm = (*env)->NewObject(env, comm_class, cid);
	mailbox = (*env)->GetStringUTFChars(env, jmailbox, 0);

	task = jtask_to_native_task(jtask, env);

	if (!task) {
    (*env)->ReleaseStringUTFChars(env, jmailbox, mailbox);
    (*env)->DeleteLocalRef(env, jcomm);
    jxbt_throw_notbound(env, "task", jtask);
		return NULL;
	}
  MSG_task_set_data(task, (void *) (*env)->NewGlobalRef(env, jtask));
	comm = MSG_task_isend(task,mailbox);

	(*env)->SetLongField(env, jcomm, id_bind, (jlong) (long)(comm));
	(*env)->SetLongField(env, jcomm, id_bind_task, (jlong) (long)(NULL));
	(*env)->SetBooleanField(env, jcomm, id_receiving, JNI_FALSE);

	(*env)->ReleaseStringUTFChars(env, jmailbox, mailbox);

	return jcomm;
}
