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

static jmethodID jtask_field_Comm_constructor;

static jfieldID jtask_field_Task_bind;
static jfieldID jtask_field_Comm_bind;
static jfieldID jtask_field_Comm_taskBind;
static jfieldID jtask_field_Comm_receiving;

void jtask_bind(jobject jtask, m_task_t task, JNIEnv * env)
{
  (*env)->SetLongField(env, jtask, jtask_field_Task_bind, (jlong) (long) (task));
}

m_task_t jtask_to_native_task(jobject jtask, JNIEnv * env)
{
  return (m_task_t) (long) (*env)->GetLongField(env, jtask, jtask_field_Task_bind);
}

jboolean jtask_is_valid(jobject jtask, JNIEnv * env)
{
  return (*env)->GetLongField(env, jtask, jtask_field_Task_bind) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_Task_nativeInit(JNIEnv *env, jclass cls) {
	jclass jtask_class_Comm = (*env)->FindClass(env, "org/simgrid/msg/Comm");

	jtask_field_Comm_constructor = (*env)->GetMethodID(env, jtask_class_Comm, "<init>", "()V");
	//FIXME: Don't use jxbt_get_sfield directly.
	jtask_field_Task_bind = jxbt_get_sfield(env, "org/simgrid/msg/Task", "bind", "J");
	jtask_field_Comm_bind = jxbt_get_sfield(env, "org/simgrid/msg/Comm", "bind", "J");
	jtask_field_Comm_taskBind = jxbt_get_sfield(env, "org/simgrid/msg/Comm", "taskBind", "J");
	jtask_field_Comm_receiving = jxbt_get_sfield(env, "org/simgrid/msg/Comm", "receiving", "Z");
}

JNIEXPORT jobject JNICALL
Java_org_simgrid_msg_Task_irecv(JNIEnv * env, jclass cls, jstring jmailbox) {
	msg_comm_t comm;
	const char *mailbox;
	jclass comm_class;
	//pointer to store the task object pointer.
	m_task_t *task = xbt_new(m_task_t,1);
	*task = NULL;
	/* There should be a cache here */
	comm_class = (*env)->FindClass(env, "org/simgrid/msg/Comm");

	if (!comm_class) {
		jxbt_throw_native(env,bprintf("fieldID or methodID or class not found."));
		return NULL;
	}

	jobject jcomm = (*env)->NewObject(env, comm_class, jtask_field_Comm_constructor);
	if (!jcomm) {
		jxbt_throw_native(env,bprintf("Can't create a Comm object."));
		return NULL;
	}

	mailbox = (*env)->GetStringUTFChars(env, jmailbox, 0);

	comm = MSG_task_irecv(task,mailbox);

	(*env)->SetLongField(env, jcomm, jtask_field_Comm_bind, (jlong) (long)(comm));
	(*env)->SetLongField(env, jcomm, jtask_field_Comm_taskBind, (jlong) (long)(task));
	(*env)->SetBooleanField(env, jcomm, jtask_field_Comm_receiving, JNI_TRUE);

	(*env)->ReleaseStringUTFChars(env, jmailbox, mailbox);

	return jcomm;
}

JNIEXPORT jobject JNICALL
Java_org_simgrid_msg_Task_isend(JNIEnv *env, jobject jtask, jstring jmailbox) {
	jclass comm_class;

	const char *mailbox;

	m_task_t task;

	jobject jcomm;
	msg_comm_t comm;

	comm_class = (*env)->FindClass(env, "org/simgrid/msg/Comm");

	if (!comm_class) return NULL;

	jcomm = (*env)->NewObject(env, comm_class, jtask_field_Comm_constructor);
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

	(*env)->SetLongField(env, jcomm, jtask_field_Comm_bind, (jlong) (long)(comm));
	(*env)->SetLongField(env, jcomm, jtask_field_Comm_taskBind, (jlong) (long)(NULL));
	(*env)->SetBooleanField(env, jcomm, jtask_field_Comm_receiving, JNI_FALSE);

	(*env)->ReleaseStringUTFChars(env, jmailbox, mailbox);

	return jcomm;
}
