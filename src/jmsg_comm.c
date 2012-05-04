/* Functions related to the java comm instances 																*/

/* Copyright (c) 2012. The SimGrid Team. All rights reserved.                   */
#include "jmsg_comm.h"
#include "jxbt_utilities.h"
#include <msg/msg.h>
XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(jmsg);

JNIEXPORT void JNICALL
Java_org_simgrid_msg_Comm_unbind(JNIEnv *env, jobject jcomm) {
	jfieldID id = jxbt_get_sfield(env, "org/simgrid/msg/Comm", "bind", "J");
	jfieldID id_task = jxbt_get_sfield(env, "org/simgrid/msg/Comm", "bindTask", "J");
	msg_comm_t comm;
	m_task_t *task_received;
	if (!id || !id_task)
		return;

	task_received = (m_task_t*)  (long) (*env)->GetLongField(env, jcomm, id_task);
	if (task_received != NULL) {
		xbt_free(task_received);
	}

	comm = (msg_comm_t) (long) (*env)->GetLongField(env, jcomm, id);
	MSG_comm_destroy(comm);
}

JNIEXPORT jboolean JNICALL
Java_org_simgrid_msg_Comm_test(JNIEnv *env, jobject jcomm) {
	msg_comm_t comm;

	jfieldID idComm = jxbt_get_sfield(env, "org/simgrid/msg/Comm", "bind", "J");
	jclass jclass = jxbt_get_class(env,"org/simgrid/msg/Comm");
	jfieldID idTask = jxbt_get_jfield(env, jclass, "task", "Lorg/simgrid/msg/Task;");
	jfieldID id_receiving = jxbt_get_sfield(env, "org/simgrid/msg/Comm", "receiving", "Z");


	if (!idComm || !idTask || !id_receiving) {
		jxbt_throw_native(env,bprintf("idTask or idComm is null"));
		return JNI_FALSE;
	}

	comm = (msg_comm_t) (long) (*env)->GetLongField(env, jcomm, idComm);
	if (!comm) {
		jxbt_throw_native(env,bprintf("comm is null"));
		return JNI_FALSE;
	}
	if (MSG_comm_test(comm)) {
		MSG_error_t status = MSG_comm_get_status(comm);

		if (status == MSG_OK) {
			//test if we are receiving or sending a task.
			jboolean jreceiving = (*env)->GetBooleanField(env, jcomm, id_receiving);
			if (jreceiving == JNI_TRUE) {
				//bind the task object.
				m_task_t task = MSG_comm_get_task(comm);
				xbt_assert(task != NULL, "Task is NULL");
				jobject jtask_global = MSG_task_get_data(task);
				//case where the data has already been retrieved
				if (jtask_global == NULL)
				{
					return JNI_TRUE;
				}

				//Make sure the data will be correctly gc.
				jobject jtask_local = (*env)->NewLocalRef(env, jtask_global);
				(*env)->DeleteGlobalRef(env, jtask_global);

				(*env)->SetObjectField(env, jcomm, idTask, jtask_local);

				MSG_task_set_data(task, NULL);
			}
			return JNI_TRUE;
		}
		else {
			//send the correct exception
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
					jxbt_throw_native(env,bprintf("receive failed"));
			}
			return JNI_FALSE;
		}
	}
	else {
		return JNI_FALSE;
	}
}
