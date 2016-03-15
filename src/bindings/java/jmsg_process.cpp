/* Functions related to the java process instances.                         */

/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "jmsg_process.h"

#include "jmsg.h"
#include "jmsg_host.h"
#include "jxbt_utilities.h"
#include "JavaContext.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(jmsg);

extern "C" {

jfieldID jprocess_field_Process_bind;
jfieldID jprocess_field_Process_host;
jfieldID jprocess_field_Process_killTime;
jfieldID jprocess_field_Process_id;
jfieldID jprocess_field_Process_name;
jfieldID jprocess_field_Process_pid;
jfieldID jprocess_field_Process_ppid;

JNIEXPORT void JNICALL
Java_org_simgrid_msg_Process_exit(JNIEnv *env, jobject jprocess) {

}

jobject native_to_java_process(msg_process_t process)
{
  simgrid::java::JavaContext* context = (simgrid::java::JavaContext*) MSG_process_get_smx_ctx(process);
  return context->jprocess;
}

jobject jprocess_new_global_ref(jobject jprocess, JNIEnv * env)
{
  return env->NewGlobalRef(jprocess);
}

void jprocess_delete_global_ref(jobject jprocess, JNIEnv * env)
{
  env->DeleteGlobalRef(jprocess);
}

void jprocess_join(jobject jprocess, JNIEnv * env)
{
  msg_process_t process = jprocess_to_native_process(jprocess,env);
  simgrid::java::JavaContext* context = (simgrid::java::JavaContext*) MSG_process_get_smx_ctx(process);
  xbt_os_thread_join(context->thread,NULL);
}

msg_process_t jprocess_to_native_process(jobject jprocess, JNIEnv * env)
{
  return (msg_process_t)(intptr_t)env->GetLongField(jprocess, jprocess_field_Process_bind);
}

void jprocess_bind(jobject jprocess, msg_process_t process, JNIEnv * env)
{
  env->SetLongField(jprocess, jprocess_field_Process_bind, (intptr_t)process);
}

jlong jprocess_get_id(jobject jprocess, JNIEnv * env)
{
  return (intptr_t)env->GetLongField(jprocess, jprocess_field_Process_id);
}

jstring jprocess_get_name(jobject jprocess, JNIEnv * env)
{
  jstring jname = (jstring) env->GetObjectField(jprocess, jprocess_field_Process_name);
  return (jstring) env->NewGlobalRef(jname);
}

jboolean jprocess_is_valid(jobject jprocess, JNIEnv * env)
{
  jfieldID id = jxbt_get_sfield(env, "org/simgrid/msg/Process", "bind", "J");

  if (!id)
    return JNI_FALSE;

  return env->GetLongField(jprocess, id) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_nativeInit(JNIEnv *env, jclass cls) {
  jclass jprocess_class_Process = env->FindClass("org/simgrid/msg/Process");

  jprocess_field_Process_name = jxbt_get_jfield(env, jprocess_class_Process, "name", "Ljava/lang/String;");
  jprocess_field_Process_bind = jxbt_get_jfield(env, jprocess_class_Process, "bind", "J");
  jprocess_field_Process_id = jxbt_get_jfield(env, jprocess_class_Process, "id", "J");
  jprocess_field_Process_pid = jxbt_get_jfield(env, jprocess_class_Process, "pid", "I");
  jprocess_field_Process_ppid = jxbt_get_jfield(env, jprocess_class_Process, "ppid", "I");
  jprocess_field_Process_host = jxbt_get_jfield(env, jprocess_class_Process, "host", "Lorg/simgrid/msg/Host;");
  jprocess_field_Process_killTime = jxbt_get_jfield(env, jprocess_class_Process, "killTime", "D");
  if (!jprocess_class_Process || !jprocess_field_Process_id || !jprocess_field_Process_name ||
      !jprocess_field_Process_pid || !jprocess_field_Process_ppid || !jprocess_field_Process_host) {
    jxbt_throw_native(env,bprintf("Can't find some fields in Java class. You should report this bug."));
  }
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_create(JNIEnv * env, jobject jprocess_arg, jobject jhostname)
{
  jobject jprocess;             /* the global reference to the java process instance    */
  jstring jname;                /* the name of the java process instance                */
  const char *name;             /* the C name of the process                            */
  const char *hostname;
  msg_process_t process;          /* the native process to create                         */
  msg_host_t host;                /* Where that process lives */

  hostname = env->GetStringUTFChars((jstring) jhostname, 0);

  /* get the name of the java process */
  jname = jprocess_get_name(jprocess_arg, env);
  if (!jname) {
    jxbt_throw_null(env,
            xbt_strdup("Internal error: Process name cannot be NULL"));
    return;
  }

  /* bind/retrieve the msg host */
  host = MSG_host_by_name(hostname);

  if (!(host)) {    /* not bound */
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
  name = env->GetStringUTFChars(jname, 0);
  name = xbt_strdup(name);

  /* Retrieve the kill time from the process */
  jdouble jkill = env->GetDoubleField(jprocess, jprocess_field_Process_killTime);
  /* Actually build the MSG process */
  process = MSG_process_create_with_environment(name, [](int argc, char** argv) -> int {
              smx_process_t process = SIMIX_process_self();
              // This is the jprocess passed as environment.
              // It would be simplet if we could use a closure.
              jobject jprocess = (jobject) MSG_process_get_data(process);
              simgrid::java::java_main_jprocess(jprocess);
              return 0;
            }, jprocess,
            host,
            /*argc, argv, properties*/
            0, NULL, NULL);
  MSG_process_set_kill_time(process, (double)jkill);
  /* bind the java process instance to the native process */
  jprocess_bind(jprocess, process, env);

  /* release our reference to the process name (variable name becomes invalid) */
  //FIXME : This line should be uncommented but with mac it doesn't work. BIG WARNING
  //env->ReleaseStringUTFChars(jname, name);
  env->ReleaseStringUTFChars((jstring) jhostname, hostname);

  /* sets the PID and the PPID of the process */
  env->SetIntField(jprocess, jprocess_field_Process_pid,(jint) MSG_process_get_PID(process));
  env->SetIntField(jprocess, jprocess_field_Process_ppid, (jint) MSG_process_get_PPID(process));
  /* sets the Host of the process */
  jobject jhost = Java_org_simgrid_msg_Host_getByName(env,NULL, (jstring)jhostname);

  env->SetObjectField(jprocess, jprocess_field_Process_host, jhost);
}

JNIEXPORT jint JNICALL Java_org_simgrid_msg_Process_killAll(JNIEnv * env, jclass cls, jint jresetPID)
{
  return (jint) MSG_process_killall((int) jresetPID);
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Process_fromPID(JNIEnv * env, jclass cls, jint PID)
{
  msg_process_t process = MSG_process_from_PID(PID);

  if (!process) {
    jxbt_throw_process_not_found(env, bprintf("PID = %d",(int) PID));
    return NULL;
  }

  jobject jprocess = native_to_java_process(process);

  if (!jprocess) {
    jxbt_throw_jni(env, "get process failed");
    return NULL;
  }

  return jprocess;
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Process_getProperty(JNIEnv *env, jobject jprocess, jobject jname) {
  msg_process_t process = jprocess_to_native_process(jprocess, env);

  if (!process) {
    jxbt_throw_notbound(env, "process", jprocess);
    return NULL;
  }
  const char *name = env->GetStringUTFChars((jstring)jname, 0);

  const char *property = MSG_process_get_property_value(process, name);
  if (!property) {
    return NULL;
  }

  jobject jproperty = env->NewStringUTF(property);

  env->ReleaseStringUTFChars((jstring)jname, name);

  return jproperty;
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Process_getCurrentProcess(JNIEnv * env, jclass cls)
{
  msg_process_t process = MSG_process_self();
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

JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_suspend(JNIEnv * env, jobject jprocess)
{
  msg_process_t process = jprocess_to_native_process(jprocess, env);

  if (!process) {
    jxbt_throw_notbound(env, "process", jprocess);
    return;
  }

  /* try to suspend the process */
  msg_error_t rv = MSG_process_suspend(process);

  jxbt_check_res("MSG_process_suspend()", rv, MSG_OK, bprintf("unexpected error , please report this bug"));
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_resume(JNIEnv * env, jobject jprocess)
{
  msg_process_t process = jprocess_to_native_process(jprocess, env);

  if (!process) {
    jxbt_throw_notbound(env, "process", jprocess);
    return;
  }

  /* try to resume the process */
  msg_error_t rv = MSG_process_resume(process);

  jxbt_check_res("MSG_process_resume()", rv, MSG_OK, bprintf("unexpected error , please report this bug"));
}
JNIEXPORT void
JNICALL Java_org_simgrid_msg_Process_setAutoRestart (JNIEnv *env, jobject jprocess, jboolean jauto_restart) {
  msg_process_t process = jprocess_to_native_process(jprocess, env);
  xbt_ex_t e;

  int auto_restart = jauto_restart == JNI_TRUE ? 1 : 0;

  if (!process) {
    jxbt_throw_notbound(env, "process", jprocess);
    return;
  }

  TRY {
    MSG_process_auto_restart_set(process,auto_restart);
  }
  CATCH (e) {
    xbt_ex_free(e);
  }
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_restart (JNIEnv *env, jobject jprocess) {
  msg_process_t process = jprocess_to_native_process(jprocess, env);
  xbt_ex_t e;

  if (!process) {
    jxbt_throw_notbound(env, "process", jprocess);
    return;
  }

  TRY {
    MSG_process_restart(process);
  }
  CATCH (e) {
    xbt_ex_free(e);
  }

}
JNIEXPORT jboolean JNICALL Java_org_simgrid_msg_Process_isSuspended(JNIEnv * env, jobject jprocess)
{
  msg_process_t process = jprocess_to_native_process(jprocess, env);

  if (!process) {
    jxbt_throw_notbound(env, "process", jprocess);
    return 0;
  }

  /* true is the process is suspended, false otherwise */
  return (jboolean) MSG_process_is_suspended(process);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_sleep(JNIEnv *env, jclass cls, jlong jmillis, jint jnanos)
 {
  double time =  ((double)jmillis) / 1000 + ((double)jnanos) / 1000000000;
  msg_error_t rv;
  rv = MSG_process_sleep(time);
  if (rv != MSG_OK) {
    XBT_DEBUG("Something during the MSG_process_sleep invocation was wrong, trigger a HostFailureException");

    //jmsg_throw_status(env,rv);

    // adsein, the code above as been replaced by the code below. Indeed, according to the documentation, a sleep can only
    // trigger a host_failure exception. When the sleep crashes due to a host shutdown, the exception thrown by smx_context_java.c
    // is a cancelled_error, see bindings/java/smx_context_java.c, function void smx_ctx_java_stop(smx_context_t context) and src/msg/msg_gos.c
    // function  msg_error_t MSG_process_sleep(double nb_sec)

    jxbt_throw_host_failure(env,NULL);
  }
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_waitFor(JNIEnv * env, jobject jprocess, jdouble jseconds)
{
  msg_error_t rv;
  rv = MSG_process_sleep((double)jseconds);
  if (env->ExceptionOccurred())
    return;
  if (rv != MSG_OK) {
    XBT_DEBUG("Status NOK");
    jmsg_throw_status(env,rv);
  }
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_kill(JNIEnv * env, jobject jprocess)
{
  /* get the native instances from the java ones */
  msg_process_t process = jprocess_to_native_process(jprocess, env);
  if (!process) {
    jxbt_throw_notbound(env, "process", jprocess);
    return;
  }

  MSG_process_kill(process);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_migrate(JNIEnv * env, jobject jprocess, jobject jhost)
{
  msg_process_t process = jprocess_to_native_process(jprocess, env);

  if (!process) {
    jxbt_throw_notbound(env, "process", jprocess);
    return;
  }

  msg_host_t host = jhost_get_native(env, jhost);

  if (!host) {
    jxbt_throw_notbound(env, "host", jhost);
    return;
  }

  /* try to change the host of the process */
  msg_error_t rv = MSG_process_migrate(process, host);
  if (rv != MSG_OK) {
    jmsg_throw_status(env,rv);
    return;
  }
  /* change the host java side */
  env->SetObjectField(jprocess, jprocess_field_Process_host, jhost);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Process_setKillTime (JNIEnv *env , jobject jprocess, jdouble jkilltime) {
  msg_process_t process = jprocess_to_native_process(jprocess, env);
  MSG_process_set_kill_time(process, (double)jkilltime);
}

JNIEXPORT jint JNICALL Java_org_simgrid_msg_Process_getCount(JNIEnv * env, jclass cls) {
  return (jint) MSG_process_get_number();
}

}
