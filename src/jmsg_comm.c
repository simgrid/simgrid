/* Functions related to the java comm instances 																*/

/* Copyright (c) 2012. The SimGrid Team. All rights reserved.                   */
#include "jmsg_comm.h"
#include "jxbt_utilities.h"
#include <msg/msg.h>
XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(jmsg);

static jfieldID jtask_field_Comm_task;
static jfieldID jcomm_field_Comm_bind;
static jfieldID jcomm_field_Comm_taskBind;
static jfieldID jcomm_field_Comm_receiving;

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
void jcomm_throw(JNIEnv *env, MSG_error_t status) {
	switch (status) {
		case MSG_TIMEOUT:
			jxbt_throw_time_out_failure(env,NULL);
		break;
		case MSG_TRANSFER_FAILURE:
			jxbt_throw_transfer_failure(env,NULL);
		break;
		case MSG_HOST_FAILURE:
			jxbt_throw_host_failure(env,NULL);
		break;
		default:
			jxbt_throw_native(env,bprintf("communication failed"));
	}
}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_Comm_nativeInit(JNIEnv *env, jclass cls) {
	jclass jfield_class_Comm = (*env)->FindClass(env, "org/simgrid/msg/Comm");
	jcomm_field_Comm_bind = jxbt_get_sfield(env, "org/simgrid/msg/Comm", "bind", "J");
	jcomm_field_Comm_taskBind  = jxbt_get_sfield(env, "org/simgrid/msg/Comm", "taskBind", "J");
	jcomm_field_Comm_receiving = jxbt_get_sfield(env, "org/simgrid/msg/Comm", "receiving", "Z");
	jtask_field_Comm_task = jxbt_get_jfield(env, jfield_class_Comm, "task", "Lorg/simgrid/msg/Task;");
}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_Comm_unbind(JNIEnv *env, jobject jcomm) {
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

	if (!comm) {
		jxbt_throw_native(env,bprintf("comm is null"));
		return JNI_FALSE;
	}
	if (MSG_comm_test(comm)) {
		MSG_error_t status = MSG_comm_get_status(comm);
		if (status == MSG_OK) {
			jcomm_bind_task(env,jcomm);
			return JNI_TRUE;
		}
		else {
			//send the correct exception
			jcomm_throw(env,status);
			return JNI_FALSE;
		}
	}
	else {
		return JNI_FALSE;
	}
}
JNIEXPORT void JNICALL
Java_org_simgrid_msg_Comm_waitCompletion(JNIEnv *env, jobject jcomm, jdouble timeout) {
	msg_comm_t comm = (msg_comm_t) (long) (*env)->GetLongField(env, jcomm, jcomm_field_Comm_bind);
	if (!comm) {
		jxbt_throw_native(env,bprintf("comm is null"));
		return;
	}

	MSG_error_t status = MSG_comm_wait(comm,(double)timeout);
	if (status == MSG_OK) {
		jcomm_bind_task(env,jcomm);
		return;
	}
	else {
		jcomm_throw(env,status);
	}


}
