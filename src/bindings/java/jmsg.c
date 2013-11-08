/* Java Wrappers to the MSG API.                                            */

/* Copyright (c) 2007-2013. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include <msg/msg.h>
#include <simgrid/simix.h>
#include <surf/surfxml_parse.h>
#include <locale.h>

#include "smx_context_java.h"
#include "smx_context_cojava.h"

#include "jmsg_process.h"

#include "jmsg_as.h"

#include "jmsg_host.h"
#include "jmsg_task.h"
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

static int create_jprocess(int argc, char *argv[]);

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(jmsg);

JavaVM *__java_vm = NULL;

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

void jmsg_throw_status(JNIEnv *env, msg_error_t status) {
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
        jxbt_throw_native(env,xbt_strdup("communication failed"));
  }
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

  XBT_LOG_CONNECT(jmsg);
#ifdef HAVE_TRACING
  XBT_LOG_CONNECT(jtrace);
#endif

  (*env)->GetJavaVM(env, &__java_vm);

  if ((*env)->FindClass(env, "java/dyn/Coroutine")) {
  	XBT_INFO("Using Coroutines. Your simulation is on steroid.");
  	smx_factory_initializer_to_use = SIMIX_ctx_cojava_factory_init;
  }
  else {
  	XBT_INFO("Using regular java threads. Coroutines could speed your simulation up.");
  	smx_factory_initializer_to_use = SIMIX_ctx_java_factory_init;
  }
  jthrowable exc = (*env)->ExceptionOccurred(env);
  if (exc) {
    (*env)->ExceptionClear(env);
  }

  setlocale(LC_NUMERIC,"C");

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

  MSG_init(&argc, argv);

  for (index = 0; index < argc; index++)
    free(argv[index]);

  free(argv);
}

JNIEXPORT void JNICALL
    JNICALL Java_org_simgrid_msg_Msg_run(JNIEnv * env, jclass cls)
{
  msg_error_t rv;
  int index;
  xbt_dynar_t hosts;
  jobject jhost;

  /* Run everything */
  XBT_DEBUG("Ready to run MSG_MAIN");
  rv = MSG_main();
  XBT_DEBUG("Done running MSG_MAIN");
  jxbt_check_res("MSG_main()", rv, MSG_OK,
                 xbt_strdup("unexpected error : MSG_main() failed .. please report this bug "));

  XBT_INFO("MSG_main finished; Cleaning up the simulation...");
  /* Cleanup java hosts */
  hosts = MSG_hosts_as_dynar();
  for (index = 0; index < xbt_dynar_length(hosts) - 1; index++) {
    jhost = (jobject) MSG_host_get_data(xbt_dynar_get_as(hosts,index,msg_host_t));
    if (jhost)
      jhost_unref(env, jhost);

  }
  xbt_dynar_free(&hosts);
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

JNIEXPORT jobject JNICALL
Java_org_simgrid_msg_Msg_environmentGetRoutingRoot(JNIEnv * env, jclass cls)
{
  msg_as_t as = MSG_environment_get_routing_root();
  jobject jas = jas_new_instance(env);
  if (!jas) {
    jxbt_throw_jni(env, "java As instantiation failed");
    return NULL;
  }
  jas = jas_ref(env, jas);
  if (!jas) {
    jxbt_throw_jni(env, "new global ref allocation failed");
    return NULL;
  }
  jas_bind(jas, as, env);

  return (jobject) jas;
}

JNIEXPORT void JNICALL
Java_org_simgrid_msg_Msg_debug(JNIEnv * env, jclass cls, jstring js)
{
  const char *s = (*env)->GetStringUTFChars(env, js, 0);
  XBT_DEBUG("%s", s);
  (*env)->ReleaseStringUTFChars(env, js, s);
}
JNIEXPORT void JNICALL
Java_org_simgrid_msg_Msg_verb(JNIEnv * env, jclass cls, jstring js)
{
  const char *s = (*env)->GetStringUTFChars(env, js, 0);
  XBT_VERB("%s", s);
  (*env)->ReleaseStringUTFChars(env, js, s);
}
JNIEXPORT void JNICALL
Java_org_simgrid_msg_Msg_info(JNIEnv * env, jclass cls, jstring js)
{
  const char *s = (*env)->GetStringUTFChars(env, js, 0);
  XBT_INFO("%s", s);
  (*env)->ReleaseStringUTFChars(env, js, s);
}
JNIEXPORT void JNICALL
Java_org_simgrid_msg_Msg_warn(JNIEnv * env, jclass cls, jstring js)
{
  const char *s = (*env)->GetStringUTFChars(env, js, 0);
  XBT_WARN("%s", s);
  (*env)->ReleaseStringUTFChars(env, js, s);
}
JNIEXPORT void JNICALL
Java_org_simgrid_msg_Msg_error(JNIEnv * env, jclass cls, jstring js)
{
  const char *s = (*env)->GetStringUTFChars(env, js, 0);
  XBT_ERROR("%s", s);
  (*env)->ReleaseStringUTFChars(env, js, s);
}
JNIEXPORT void JNICALL
Java_org_simgrid_msg_Msg_critical(JNIEnv * env, jclass cls, jstring js)
{
  const char *s = (*env)->GetStringUTFChars(env, js, 0);
  XBT_CRITICAL("%s", s);
  (*env)->ReleaseStringUTFChars(env, js, s);
}
JNIEXPORT void JNICALL
Java_org_simgrid_msg_Msg_deployApplication(JNIEnv * env, jclass cls,
                                       jstring jdeploymentFile)
{

  const char *deploymentFile =
      (*env)->GetStringUTFChars(env, jdeploymentFile, 0);

  SIMIX_function_register_default(create_jprocess);
  MSG_launch_application(deploymentFile);
}
/**
 * Function called when there is the need to create the java Process object
 * (when we are using deployement files).
 * it HAS to be executed on the process context, else really bad things will happen.
 */
static int create_jprocess(int argc, char *argv[]) {
  JNIEnv *env = get_current_thread_env();
  //Change the "." in class name for "/".
  xbt_str_subst(argv[0],'.','/',0);
  jclass class_Process = (*env)->FindClass(env, argv[0]);
  xbt_str_subst(argv[0],'/','.',0);
  //Retrieve the methodID for the constructor
  xbt_assert((class_Process != NULL), "Class not found (%s).", argv[0]);
  jmethodID constructor_Process = (*env)->GetMethodID(env, class_Process, "<init>", "(Lorg/simgrid/msg/Host;Ljava/lang/String;[Ljava/lang/String;)V");
  xbt_assert((constructor_Process != NULL), "Constructor not found for class %s. Is there a (Host, String ,String[]) constructor in your class ?", argv[0]);

  //Retrieve the name of the process.
  jstring jname = (*env)->NewStringUTF(env, argv[0]);
  //Build the arguments
  jobjectArray args = (jobjectArray)((*env)->NewObjectArray(env,argc - 1,
  (*env)->FindClass(env,"java/lang/String"),
  (*env)->NewStringUTF(env,"")));
  int i;
  for (i = 1; i < argc; i++)
      (*env)->SetObjectArrayElement(env,args,i - 1,(*env)->NewStringUTF(env, argv[i]));
  //Retrieve the host for the process.
  jstring jhostName = (*env)->NewStringUTF(env, MSG_host_get_name(MSG_host_self()));
  jobject jhost = Java_org_simgrid_msg_Host_getByName(env, NULL, jhostName);
  //creates the process
  jobject jprocess = (*env)->NewObject(env, class_Process, constructor_Process, jhost, jname, args);
  xbt_assert((jprocess != NULL), "Process allocation failed.");
  jprocess = (*env)->NewGlobalRef(env, jprocess);
  //bind the process to the context
  msg_process_t process = MSG_process_self();
  smx_ctx_java_t context = (smx_ctx_java_t)MSG_process_get_smx_ctx(process);
  context->jprocess = jprocess;
/* sets the PID and the PPID of the process */
(*env)->SetIntField(env, jprocess, jprocess_field_Process_pid,(jint) MSG_process_get_PID(process));
(*env)->SetIntField(env, jprocess, jprocess_field_Process_ppid, (jint) MSG_process_get_PPID(process));
  jprocess_bind(jprocess, process, env);

  return 0;
}

