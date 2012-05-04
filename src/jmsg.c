/* Java Wrappers to the MSG API.                                            */

/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include <msg/msg.h>
#include <simgrid/simix.h>
#include <surf/surfxml_parse.h>


#include "smx_context_java.h"

#include "jmsg_process.h"

#include "jmsg_host.h"
#include "jmsg_task.h"
#include "jmsg_application_handler.h"
#include "jxbt_utilities.h"

#include "jmsg.h"

/* Shut up some errors in eclipse online compiler. I wish such a pimple wouldn't be needed */
#ifndef JNIEXPORT
#define JNIEXPORT
#endif
#ifndef JNICALL
#define JNICALL
#endif
/* end of eclipse-mandated pimple */

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(jmsg);

static JavaVM *__java_vm = NULL;


JavaVM *get_java_VM(void)
{
  return __java_vm;
}

JNIEnv *get_current_thread_env(void)
{
  JNIEnv *env;

  (*__java_vm)->AttachCurrentThread(__java_vm, (void **) &env, NULL);

  return env;
}

/*
 * The MSG process connected functions implementation.                                 
 */

JNIEXPORT void JNICALL
Java_org_simgrid_msg_MsgNative_processCreate(JNIEnv * env, jclass cls,
                                         jobject jprocess_arg,
                                         jobject jhostname)
{
     
   
  jobject jprocess;             /* the global reference to the java process instance    */
  jstring jname;                /* the name of the java process instance                */
  const char *name;             /* the C name of the process                            */
  const char *hostname;
  m_process_t process;          /* the native process to create                         */
  m_host_t host;                /* Where that process lives */
   
  hostname = (*env)->GetStringUTFChars(env, jhostname, 0);

  XBT_DEBUG("Java_org_simgrid_msg_MsgNative_processCreate(env=%p,cls=%p,jproc=%p,host=%s)",
	 env, cls, jprocess_arg, hostname);
   
   
  /* get the name of the java process */
  jname = jprocess_get_name(jprocess_arg, env);
  if (!jname) {
    jxbt_throw_null(env,
            xbt_strdup("Internal error: Process name cannot be NULL"));
    return;
  }

  /* bind/retrieve the msg host */
  host = MSG_get_host_by_name(hostname);

  if (!(host)) {    /* not binded */
    jxbt_throw_host_not_found(env, hostname);
    return;
  }

  /* create a global java process instance */
  jprocess = jprocess_new_global_ref(jprocess_arg, env);
  if (!jprocess) {
    jxbt_throw_jni(env, "Can't get a global ref to the java process");
    return;
  }

  /* build the C name of the process */
  name = (*env)->GetStringUTFChars(env, jname, 0);
  name = xbt_strdup(name);
  
  /* Actually build the MSG process */
  process = MSG_process_create_with_environment(name,
						(xbt_main_func_t) jprocess,
						/*data*/ NULL,
						host,
						/* kill_time */-1,
						/*argc, argv, properties*/
						0,NULL,NULL);
     
  MSG_process_set_data(process,&process);
   
  /* release our reference to the process name (variable name becomes invalid) */
  //FIXME : This line should be uncommented but with mac it doesn't work. BIG WARNING
  //(*env)->ReleaseStringUTFChars(env, jname, name);
  (*env)->ReleaseStringUTFChars(env, jhostname, hostname);
   
  /* bind the java process instance to the native process */
  jprocess_bind(jprocess, process, env);

}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_MsgNative_processSuspend(JNIEnv * env, jclass cls,
                                          jobject jprocess)
{
  m_process_t process = jprocess_to_native_process(jprocess, env);

  if (!process) {
    jxbt_throw_notbound(env, "process", jprocess);
    return;
  }

  /* try to suspend the process */
  MSG_error_t rv = MSG_process_suspend(process);

  jxbt_check_res("MSG_process_suspend()", rv, MSG_OK,
                 bprintf("unexpected error , please report this bug"));

}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_Process_simulatedSleep(JNIEnv * env, jobject jprocess,
                                           jdouble jseconds) {
	  m_process_t process = jprocess_to_native_process(jprocess, env);

	  if (!process) {
	    jxbt_throw_notbound(env, "process", jprocess);
	    return;
	  }
	  MSG_error_t rv = MSG_process_sleep((double)jseconds);

	  jxbt_check_res("MSG_process_sleep()", rv, MSG_OK,
	                 bprintf("unexpected error , please report this bug"));
}


JNIEXPORT void JNICALL
Java_org_simgrid_msg_MsgNative_processResume(JNIEnv * env, jclass cls,
                                         jobject jprocess)
{
  m_process_t process = jprocess_to_native_process(jprocess, env);

  if (!process) {
    jxbt_throw_notbound(env, "process", jprocess);
    return;
  }

  /* try to resume the process */
  MSG_error_t rv = MSG_process_resume(process);

  jxbt_check_res("MSG_process_resume()", rv, MSG_OK,
                 bprintf("unexpected error , please report this bug"));
}

JNIEXPORT jboolean JNICALL
Java_org_simgrid_msg_MsgNative_processIsSuspended(JNIEnv * env, jclass cls,
                                              jobject jprocess)
{
  m_process_t process = jprocess_to_native_process(jprocess, env);

  if (!process) {
    jxbt_throw_notbound(env, "process", jprocess);
    return 0;
  }

  /* true is the process is suspended, false otherwise */
  return (jboolean) MSG_process_is_suspended(process);
}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_MsgNative_processKill(JNIEnv * env, jclass cls,
                                       jobject jprocess)
{
  /* get the native instances from the java ones */
  m_process_t process = jprocess_to_native_process(jprocess, env);

  if (!process) {
    jxbt_throw_notbound(env, "process", jprocess);
    return;
  }

  /* kill the native process (this wrapper is call by the destructor of the java 
   * process instance)
   */
  MSG_process_kill(process);
}

JNIEXPORT jobject JNICALL
Java_org_simgrid_msg_MsgNative_processGetHost(JNIEnv * env, jclass cls,
                                          jobject jprocess)
{
  /* get the native instances from the java ones */
  m_process_t process = jprocess_to_native_process(jprocess, env);
  m_host_t host;

  if (!process) {
    jxbt_throw_notbound(env, "process", jprocess);
    return NULL;
  }

  host = MSG_process_get_host(process);
  jobject res = (jobject)MSG_host_get_data(host);

  if (!res) {
	  XBT_INFO("Binding error for host %s ",MSG_host_get_name(host));
	  jxbt_throw_jni(env, bprintf("Binding error for host %s ",MSG_host_get_name(host)));
	  return NULL;
  }

  /* return the global reference to the java host instance */
  return res;

}

JNIEXPORT jobject JNICALL
Java_org_simgrid_msg_MsgNative_processFromPID(JNIEnv * env, jclass cls,
                                          jint PID)
{
  m_process_t process = MSG_process_from_PID(PID);

  if (!process) {
    jxbt_throw_process_not_found(env, bprintf("PID = %d",(int) PID));
    return NULL;
  }

  if (!native_to_java_process(process)) {
    jxbt_throw_jni(env, "SIMIX_process_get_jprocess() failed");
    return NULL;
  }

  return (jobject) (native_to_java_process(process));
}


JNIEXPORT jint JNICALL
Java_org_simgrid_msg_MsgNative_processGetPID(JNIEnv * env, jclass cls,
                                         jobject jprocess)
{
  m_process_t process = jprocess_to_native_process(jprocess, env);

  if (!process) {
    jxbt_throw_notbound(env, "process", jprocess);
    return 0;
  }

  return (jint) MSG_process_get_PID(process);
}


JNIEXPORT jint JNICALL
Java_org_simgrid_msg_MsgNative_processGetPPID(JNIEnv * env, jclass cls,
                                          jobject jprocess)
{
  m_process_t process = jprocess_to_native_process(jprocess, env);

  if (!process) {
    jxbt_throw_notbound(env, "process", jprocess);
    return 0;
  }

  return (jint) MSG_process_get_PPID(process);
}

JNIEXPORT jobject JNICALL
Java_org_simgrid_msg_MsgNative_processSelf(JNIEnv * env, jclass cls)
{
  m_process_t process = MSG_process_self();
  jobject jprocess;

  if (!process) {
    jxbt_throw_jni(env, xbt_strdup("MSG_process_self() failed"));
    return NULL;
  }

  jprocess = native_to_java_process(process);

  if (!jprocess)
    jxbt_throw_jni(env, xbt_strdup("SIMIX_process_get_jprocess() failed"));

  return jprocess;
}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_MsgNative_processMigrate(JNIEnv * env, jclass cls,
                                             jobject jprocess, jobject jhost)
{
  m_process_t process = jprocess_to_native_process(jprocess, env);

  if (!process) {
    jxbt_throw_notbound(env, "process", jprocess);
    return;
  }

  m_host_t host = jhost_get_native(env, jhost);

  if (!host) {
    jxbt_throw_notbound(env, "host", jhost);
    return;
  }

  /* try to change the host of the process */
  MSG_error_t rv = MSG_process_migrate(process, host);
  jxbt_check_res("MSG_process_migrate()", rv, MSG_OK,
                 bprintf("unexpected error , please report this bug"));

}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_MsgNative_processWaitFor(JNIEnv * env, jclass cls,
                                          jdouble seconds)
{
  MSG_error_t rv = MSG_process_sleep((double) seconds);

  jxbt_check_res("MSG_process_sleep()", rv, MSG_HOST_FAILURE,
                 bprintf("while process was waiting for %f seconds",
                         (double) seconds));

}


/***************************************************************************************
 * The MSG host connected functions implementation.                                    *
 ***************************************************************************************/

JNIEXPORT jint JNICALL
Java_org_simgrid_msg_MsgNative_hostGetLoad(JNIEnv * env, jclass cls,
                                       jobject jhost)
{
  m_host_t host = jhost_get_native(env, jhost);

  if (!host) {
    jxbt_throw_notbound(env, "host", jhost);
    return -1;
  }

  return (jint) MSG_get_host_msgload(host);
}


/***************************************************************************************
 * Unsortable functions                                                        *
 ***************************************************************************************/

JNIEXPORT jdouble JNICALL
Java_org_simgrid_msg_Msg_getClock(JNIEnv * env, jclass cls)
{
  return (jdouble) MSG_get_clock();
}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_Msg_init(JNIEnv * env, jclass cls, jobjectArray jargs)
{
  char **argv = NULL;
  int index;
  int argc = 0;
  jstring jval;
  const char *tmp;

  smx_factory_initializer_to_use = SIMIX_ctx_java_factory_init;

  if (jargs)
    argc = (int) (*env)->GetArrayLength(env, jargs);

  argc++;
  argv = xbt_new(char *, argc + 1);
  argv[0] = strdup("java");

  for (index = 0; index < argc - 1; index++) {
    jval = (jstring) (*env)->GetObjectArrayElement(env, jargs, index);
    tmp = (*env)->GetStringUTFChars(env, jval, 0);
    argv[index + 1] = strdup(tmp);
    (*env)->ReleaseStringUTFChars(env, jval, tmp);
  }
  argv[argc] = NULL;

  MSG_global_init(&argc, argv);

  for (index = 0; index < argc; index++)
    free(argv[index]);

  free(argv);

  (*env)->GetJavaVM(env, &__java_vm);
}

JNIEXPORT void JNICALL
    JNICALL Java_org_simgrid_msg_Msg_run(JNIEnv * env, jclass cls)
{
  MSG_error_t rv;
  int index;
  xbt_dynar_t hosts;
  jobject jhost;

  /* Run everything */
  XBT_INFO("Ready to run MSG_MAIN");
  rv = MSG_main();
  XBT_INFO("Done running MSG_MAIN");
  jxbt_check_res("MSG_main()", rv, MSG_OK,
                 bprintf
                 ("unexpected error : MSG_main() failed .. please report this bug "));

  XBT_INFO("MSG_main finished");

  XBT_INFO("Clean java world");
  /* Cleanup java hosts */
  hosts = MSG_hosts_as_dynar();
  for (index = 0; index < xbt_dynar_length(hosts) - 1; index++) {
    jhost = (jobject) MSG_host_get_data(xbt_dynar_get_as(hosts,index,m_host_t));
    if (jhost)
      jhost_unref(env, jhost);

  }
  xbt_dynar_free(&hosts);
  XBT_INFO("Clean native world");
}
JNIEXPORT void JNICALL
    JNICALL Java_org_simgrid_msg_Msg_clean(JNIEnv * env, jclass cls)
{
  /* cleanup native stuff. Calling it is ... useless since leaking memory at the end of the simulation is a non-issue */
  MSG_error_t rv = MSG_OK != MSG_clean();
  jxbt_check_res("MSG_clean()", rv, MSG_OK,
                 bprintf
                 ("unexpected error : MSG_clean() failed .. please report this bug "));
}
   
JNIEXPORT jint JNICALL
Java_org_simgrid_msg_MsgNative_processKillAll(JNIEnv * env, jclass cls,
                                          jint jresetPID)
{
  return (jint) MSG_process_killall((int) jresetPID);
}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_Msg_createEnvironment(JNIEnv * env, jclass cls,
                                       jstring jplatformFile)
{

  const char *platformFile =
      (*env)->GetStringUTFChars(env, jplatformFile, 0);

  MSG_create_environment(platformFile);

  (*env)->ReleaseStringUTFChars(env, jplatformFile, platformFile);
}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_MsgNative_processExit(JNIEnv * env, jclass cls,
                                       jobject jprocess)
{

  m_process_t process = jprocess_to_native_process(jprocess, env);

  if (!process) {
    jxbt_throw_notbound(env, "process", jprocess);
    return;
  }

  smx_ctx_java_stop(MSG_process_get_smx_ctx(process));
}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_Msg_info(JNIEnv * env, jclass cls, jstring js)
{
  const char *s = (*env)->GetStringUTFChars(env, js, 0);
  XBT_INFO("%s", s);
  (*env)->ReleaseStringUTFChars(env, js, s);
}

JNIEXPORT jobjectArray JNICALL
Java_org_simgrid_msg_MsgNative_allHosts(JNIEnv * env, jclass cls_arg)
{
  int index;
  jobjectArray jtable;
  jobject jhost;
  jstring jname;
  m_host_t host;

  xbt_dynar_t table =  MSG_hosts_as_dynar();
  int count = xbt_dynar_length(table);

  jclass cls = jxbt_get_class(env, "org/simgrid/msg/Host");

  if (!cls) {
    return NULL;
  }

  jtable = (*env)->NewObjectArray(env, (jsize) count, cls, NULL);

  if (!jtable) {
    jxbt_throw_jni(env, "Hosts table allocation failed");
    return NULL;
  }

  for (index = 0; index < count; index++) {
    host = xbt_dynar_get_as(table,index,m_host_t);
    jhost = (jobject) (MSG_host_get_data(host));

    if (!jhost) {
      jname = (*env)->NewStringUTF(env, MSG_host_get_name(host));

      jhost =
      		Java_org_simgrid_msg_Host_getByName(env, cls_arg, jname);
      /* FIXME: leak of jname ? */
    }

    (*env)->SetObjectArrayElement(env, jtable, index, jhost);
  }
  xbt_dynar_free(&table);
  return jtable;
}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_Msg_deployApplication(JNIEnv * env, jclass cls,
                                       jstring jdeploymentFile)
{

  const char *deploymentFile =
      (*env)->GetStringUTFChars(env, jdeploymentFile, 0);

  surf_parse_reset_callbacks();

  surfxml_add_callback(STag_surfxml_process_cb_list,
                       japplication_handler_on_begin_process);

  surfxml_add_callback(ETag_surfxml_argument_cb_list,
                       japplication_handler_on_process_arg);

  surfxml_add_callback(STag_surfxml_prop_cb_list,
                       japplication_handler_on_property);

  surfxml_add_callback(ETag_surfxml_process_cb_list,
                       japplication_handler_on_end_process);

  surf_parse_open(deploymentFile);

  japplication_handler_on_start_document();

  if (surf_parse())
    jxbt_throw_jni(env, "surf_parse() failed");

  surf_parse_close();

  japplication_handler_on_end_document();

  (*env)->ReleaseStringUTFChars(env, jdeploymentFile, deploymentFile);
}
