/* Functions related to the java comm instances 																*/

/* Copyright (c) 2012. The SimGrid Team. All rights reserved.                   */
#include "jmsg_comm.h"
#include "jxbt_utilities.h"
#include "jmsg.h"

#include <msg/msg.h>
XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(jmsg);

static jfieldID jcomm_field_Comm_bind;
static jfieldID jcomm_field_Comm_finished;
static jfieldID jcomm_field_Comm_receiving;
static jfieldID jtask_field_Comm_task;
static jfieldID jcomm_field_Comm_taskBind;

void jcomm_bind_task(JNIEnv *env, jobject jcomm) {
	msg_comm_t comm = (msg_comm_t) (long) (*env)->GetLongField(env, jcomm, jcomm_field_Comm_bind);
	//test if we are receiving or sending a task.
	jboolean jreceiving = (*env)->GetBooleanField(env, jcomm, jcomm_field_Comm_receiving);
	if (jreceiving == JNI_TRUE) {
		//bind the task object.
		m_task_t task = MSG_comm_get_task(comm);
		xbt_assert(task != NULL, "Task is NULL");
		jobject jtask_global = MSG_task_get_data(task);
		//case where the data has already been retrieved
		if (jtask_global == NULL) {
			return;
		}

		//Make sure the data will be correctly gc.
		jobject jtask_local = (*env)->NewLocalRef(env, jtask_global);
		(*env)->DeleteGlobalRef(env, jtask_global);

		(*env)->SetObjectField(env, jcomm, jtask_field_Comm_task, jtask_local);

		MSG_task_set_data(task, NULL);
	}

}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_Comm_nativeInit(JNIEnv *env, jclass cls) {
	jclass jfield_class_Comm = (*env)->FindClass(env, "org/simgrid/msg/Comm");
	if (!jfield_class_Comm) {
		jxbt_throw_native(env,bprintf("Can't find the org/simgrid/msg/Comm class."));
		return;
	}
	jcomm_field_Comm_bind = jxbt_get_jfield(env, jfield_class_Comm, "bind", "J");
	jcomm_field_Comm_taskBind  = jxbt_get_jfield(env, jfield_class_Comm, "taskBind", "J");
	jcomm_field_Comm_receiving = jxbt_get_jfield(env, jfield_class_Comm, "receiving", "Z");
	jtask_field_Comm_task = jxbt_get_jfield(env, jfield_class_Comm, "task", "Lorg/simgrid/msg/Task;");
	jcomm_field_Comm_finished = jxbt_get_jfield(env, jfield_class_Comm, "finished", "Z");
	if (!jcomm_field_Comm_bind || !jcomm_field_Comm_taskBind || !jcomm_field_Comm_receiving || !jtask_field_Comm_task || !jcomm_field_Comm_finished) {
  	jxbt_throw_native(env,bprintf("Can't find some fields in Java class."));
	}
}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_Comm_destroy(JNIEnv *env, jobject jcomm) {
	msg_comm_t comm;
	m_task_t *task_received;

	task_received = (m_task_t*)  (long) (*env)->GetLongField(env, jcomm, jcomm_field_Comm_taskBind);
	xbt_free(task_received);

	comm = (msg_comm_t) (long) (*env)->GetLongField(env, jcomm, jcomm_field_Comm_bind);
	MSG_comm_destroy(comm);
}

JNIEXPORT jboolean JNICALL
Java_org_simgrid_msg_Comm_test(JNIEnv *env, jobject jcomm) {
	msg_comm_t comm;
	comm = (msg_comm_t) (long) (*env)->GetLongField(env, jcomm, jcomm_field_Comm_bind);

	jboolean finished = (*env)->GetBooleanField(env, jcomm, jcomm_field_Comm_finished);
	if (finished == JNI_TRUE) {
		return JNI_TRUE;
	}

	if (!comm) {
		jxbt_throw_native(env,bprintf("comm is null"));
		return JNI_FALSE;
	}
	TRY {
		if (MSG_comm_test(comm)) {
			MSG_error_t status = MSG_comm_get_status(comm);
			if (status == MSG_OK) {
				jcomm_bind_task(env,jcomm);
				return JNI_TRUE;
			}
			else {
				//send the correct exception
				jmsg_throw_status(env,status);
				return JNI_FALSE;
			}
		}
		else {
			return JNI_FALSE;
		}
	}
	CATCH_ANONYMOUS {

	}
	return JNI_FALSE;
}
JNIEXPORT void JNICALL
Java_org_simgrid_msg_Comm_waitCompletion(JNIEnv *env, jobject jcomm, jdouble timeout) {
	msg_comm_t comm = (msg_comm_t) (long) (*env)->GetLongField(env, jcomm, jcomm_field_Comm_bind);
	if (!comm) {
		jxbt_throw_native(env,bprintf("comm is null"));
		return;
	}

	jboolean finished = (*env)->GetBooleanField(env, jcomm, jcomm_field_Comm_finished);
	if (finished == JNI_TRUE) {
		return;
	}

	MSG_error_t status;
	TRY {
		status = MSG_comm_wait(comm,(double)timeout);
	}
	CATCH_ANONYMOUS {
		return;
	}
	(*env)->SetBooleanField(env, jcomm, jcomm_field_Comm_finished, JNI_TRUE);
	if (status == MSG_OK) {
		jcomm_bind_task(env,jcomm);
		return;
	}
	else {
		jmsg_throw_status(env,status);
	}


}
