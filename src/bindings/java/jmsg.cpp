/* Java Wrappers to the MSG API.                                            */

/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/msg.h>
#include <simgrid/simix.h>
#include <locale.h>
#include <src/simix/smx_private.h>

#include "jmsg_process.h"
#include "jmsg_as.h"
#include "jmsg_host.h"
#include "jmsg_storage.h"
#include "jmsg_task.h"
#include "jxbt_utilities.h"
#include "jmsg.h"

#include "JavaContext.hpp"

/* Shut up some errors in eclipse online compiler. I wish such a pimple wouldn't be needed */
#ifndef JNIEXPORT
#define JNIEXPORT
#endif
#ifndef JNICALL
#define JNICALL
#endif
/* end of eclipse-mandated pimple */

SG_BEGIN_DECL()

int JAVA_HOST_LEVEL;
int JAVA_STORAGE_LEVEL;

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(jmsg);

JavaVM *__java_vm = NULL;

JavaVM *get_java_VM(void)
{
  return __java_vm;
}

JNIEnv *get_current_thread_env(void)
{
  JNIEnv *env;

  __java_vm->AttachCurrentThread((void **) &env, NULL);
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
    case MSG_TASK_CANCELED:
        jxbt_throw_task_cancelled(env,NULL);
    break;
    default:
        jxbt_throw_native(env,xbt_strdup("undefined message failed "
          "(please see jmsg_throw_status function in jmsg.cpp)"));
  }
}

/***************************************************************************************
 * Unsortable functions                                                        *
 ***************************************************************************************/

JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Msg_getClock(JNIEnv * env, jclass cls)
{
  return (jdouble) MSG_get_clock();
}

static void __JAVA_host_priv_free(void *host)
{
}

static void __JAVA_storage_priv_free(void *storage)
{
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_init(JNIEnv * env, jclass cls, jobjectArray jargs)
{
  char **argv = NULL;
  int index;
  int argc = 0;
  jstring jval;
  const char *tmp;

  XBT_LOG_CONNECT(jmsg);
  XBT_LOG_CONNECT(jtrace);

  env->GetJavaVM(&__java_vm);

  simgrid::simix::factory_initializer = simgrid::java::java_factory;
  jthrowable exc = env->ExceptionOccurred();
  if (exc) {
    env->ExceptionClear();
  }

  setlocale(LC_NUMERIC,"C");

  if (jargs)
    argc = (int) env->GetArrayLength(jargs);

  argc++;
  argv = xbt_new(char *, argc + 1);
  argv[0] = xbt_strdup("java");

  for (index = 0; index < argc - 1; index++) {
    jval = (jstring) env->GetObjectArrayElement(jargs, index);
    tmp = env->GetStringUTFChars(jval, 0);
    argv[index + 1] = xbt_strdup(tmp);
    env->ReleaseStringUTFChars(jval, tmp);
  }
  argv[argc] = NULL;

  MSG_init(&argc, argv);

  JAVA_HOST_LEVEL = simgrid::s4u::Host::extension_create(__JAVA_host_priv_free);
  JAVA_STORAGE_LEVEL = xbt_lib_add_level(storage_lib, __JAVA_storage_priv_free);

  for (index = 0; index < argc; index++)
    free(argv[index]);

  free(argv);
}

JNIEXPORT void JNICALL JNICALL Java_org_simgrid_msg_Msg_run(JNIEnv * env, jclass cls)
{
  /* Run everything */
  XBT_DEBUG("Ready to run MSG_MAIN");
  msg_error_t rv = MSG_main();
  XBT_DEBUG("Done running MSG_MAIN");
  jxbt_check_res("MSG_main()", rv, MSG_OK,
                 xbt_strdup("unexpected error : MSG_main() failed .. please report this bug "));

  XBT_INFO("MSG_main finished; Cleaning up the simulation...");
  /* Cleanup java hosts */
  xbt_dynar_t hosts = MSG_hosts_as_dynar();
  for (unsigned long index = 0; index < xbt_dynar_length(hosts) - 1; index++) {
    msg_host_t msg_host = xbt_dynar_get_as(hosts,index,msg_host_t);
    jobject jhost = (jobject) msg_host->extension(JAVA_HOST_LEVEL);
    if (jhost)
      jhost_unref(env, jhost);

  }
  xbt_dynar_free(&hosts);

  /* Cleanup java storages */
  xbt_dynar_t storages = MSG_storages_as_dynar();
  if(!xbt_dynar_is_empty(storages)){
    for (unsigned long index = 0; index < xbt_dynar_length(storages) - 1; index++) {
      jobject jstorage = (jobject) xbt_lib_get_level(xbt_dynar_get_as(storages,index,msg_storage_t), JAVA_STORAGE_LEVEL);
      if (jstorage)
        jstorage_unref(env, jstorage);
    }
  }
  xbt_dynar_free(&storages);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_createEnvironment(JNIEnv * env, jclass cls, jstring jplatformFile)
{
  const char *platformFile = env->GetStringUTFChars(jplatformFile, 0);

  MSG_create_environment(platformFile);

  env->ReleaseStringUTFChars(jplatformFile, platformFile);
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Msg_environmentGetRoutingRoot(JNIEnv * env, jclass cls)
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

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_debug(JNIEnv * env, jclass cls, jstring js)
{
  const char *s = env->GetStringUTFChars(js, 0);
  XBT_DEBUG("%s", s);
  env->ReleaseStringUTFChars(js, s);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_verb(JNIEnv * env, jclass cls, jstring js)
{
  const char *s = env->GetStringUTFChars(js, 0);
  XBT_VERB("%s", s);
  env->ReleaseStringUTFChars(js, s);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_info(JNIEnv * env, jclass cls, jstring js)
{
  const char *s = env->GetStringUTFChars(js, 0);
  XBT_INFO("%s", s);
  env->ReleaseStringUTFChars(js, s);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_warn(JNIEnv * env, jclass cls, jstring js)
{
  const char *s = env->GetStringUTFChars(js, 0);
  XBT_WARN("%s", s);
  env->ReleaseStringUTFChars(js, s);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_error(JNIEnv * env, jclass cls, jstring js)
{
  const char *s = env->GetStringUTFChars(js, 0);
  XBT_ERROR("%s", s);
  env->ReleaseStringUTFChars(js, s);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_critical(JNIEnv * env, jclass cls, jstring js)
{
  const char *s = env->GetStringUTFChars(js, 0);
  XBT_CRITICAL("%s", s);
  env->ReleaseStringUTFChars(js, s);
}

static int java_main(int argc, char *argv[]);

JNIEXPORT void JNICALL
Java_org_simgrid_msg_Msg_deployApplication(JNIEnv * env, jclass cls, jstring jdeploymentFile)
{
  const char *deploymentFile = env->GetStringUTFChars(jdeploymentFile, 0);

  SIMIX_function_register_default(java_main);
  MSG_launch_application(deploymentFile);
}

SG_END_DECL()

/** Run a Java org.simgrid.msg.Process
 *
 *  If needed, this waits for the process starting time.
 *  Then it calls the Process.run() method.
 */
static void run_jprocess(JNIEnv *env, jobject jprocess)
{
  xbt_assert(jprocess != nullptr, "Process not created...");
  //wait for the process to be able to begin
  //TODO: Cache it
  jfieldID jprocess_field_Process_startTime = jxbt_get_sfield(env, "org/simgrid/msg/Process", "startTime", "D");
  jdouble startTime = env->GetDoubleField(jprocess, jprocess_field_Process_startTime);
  if (startTime > MSG_get_clock())
    MSG_process_sleep(startTime - MSG_get_clock());
  //Execution of the "run" method.
  jmethodID id = jxbt_get_smethod(env, "org/simgrid/msg/Process", "run", "()V");
  xbt_assert( (id != nullptr), "Method not found...");
  env->CallVoidMethod(jprocess, id);
}

/** Create a Java org.simgrid.msg.Process with the arguments and run it */
static int java_main(int argc, char *argv[])
{
  JNIEnv *env = get_current_thread_env();
  simgrid::java::JavaContext* context = (simgrid::java::JavaContext*) SIMIX_context_self();

  //Change the "." in class name for "/".
  xbt_str_subst(argv[0],'.','/',0);
  jclass class_Process = env->FindClass(argv[0]);
  xbt_str_subst(argv[0],'/','.',0);
  //Retrieve the methodID for the constructor
  xbt_assert((class_Process != NULL), "Class not found (%s). The deployment file must use the fully qualified class name, including the package. The case is important.", argv[0]);
  jmethodID constructor_Process = env->GetMethodID(class_Process, "<init>", "(Lorg/simgrid/msg/Host;Ljava/lang/String;[Ljava/lang/String;)V");
  xbt_assert((constructor_Process != NULL), "Constructor not found for class %s. Is there a (Host, String ,String[]) constructor in your class ?", argv[0]);

  //Retrieve the name of the process.
  jstring jname = env->NewStringUTF(argv[0]);
  //Build the arguments
  jobjectArray args = (jobjectArray)env->NewObjectArray(argc - 1,
    env->FindClass("java/lang/String"),
    env->NewStringUTF(""));
  int i;
  for (i = 1; i < argc; i++)
      env->SetObjectArrayElement(args,i - 1, env->NewStringUTF(argv[i]));
  //Retrieve the host for the process.
  jstring jhostName = env->NewStringUTF(MSG_host_get_name(MSG_host_self()));
  jobject jhost = Java_org_simgrid_msg_Host_getByName(env, NULL, jhostName);
  //creates the process
  jobject jprocess = env->NewObject(class_Process, constructor_Process, jhost, jname, args);
  xbt_assert((jprocess != NULL), "Process allocation failed.");
  jprocess = env->NewGlobalRef(jprocess);
  //bind the process to the context
  msg_process_t process = MSG_process_self();

  context->jprocess = jprocess;
  /* sets the PID and the PPID of the process */
  env->SetIntField(jprocess, jprocess_field_Process_pid,(jint) MSG_process_get_PID(process));
  env->SetIntField(jprocess, jprocess_field_Process_ppid, (jint) MSG_process_get_PPID(process));
  jprocess_bind(jprocess, process, env);

  run_jprocess(env, context->jprocess);
  return 0;
}

namespace simgrid {
namespace java {

/** Run the Java org.simgrid.msg.Process */
void java_main_jprocess(jobject jprocess)
{
  JNIEnv *env = get_current_thread_env();
  simgrid::java::JavaContext* context = (simgrid::java::JavaContext*) SIMIX_context_self();
  context->jprocess = jprocess;
  smx_process_t process = SIMIX_process_self();
  jprocess_bind(context->jprocess, process, env);

  // Adrien, ugly path, just to bypass creation of context at low levels (i.e such as for the VM migration for instance)
  if (context->jprocess == nullptr)
    return;
  else
    run_jprocess(env, context->jprocess);
}

}
}

#include "simgrid/plugins/energy.h"
JNIEXPORT void JNICALL
Java_org_simgrid_msg_Msg_energyInit(void) {
  sg_energy_plugin_init();
}
