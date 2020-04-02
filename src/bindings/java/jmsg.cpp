/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <algorithm>
#include <clocale>
#include <memory>
#include <string>
#include <vector>

#include "simgrid/Exception.hpp"
#include "simgrid/plugins/energy.h"
#include "simgrid/plugins/file_system.h"
#include "simgrid/plugins/live_migration.h"
#include "simgrid/plugins/load.h"
#include "simgrid/simix.h"

#include "simgrid/s4u/Host.hpp"

#include "src/simix/smx_private.hpp"

#include "jmsg.hpp"
#include "jmsg_as.hpp"
#include "jmsg_host.h"
#include "jmsg_process.h"
#include "jmsg_storage.h"
#include "jmsg_task.h"
#include "jxbt_utilities.hpp"

#include "JavaContext.hpp"

/* Shut up some errors in eclipse online compiler. I wish such a pimple wouldn't be needed */
#ifndef JNIEXPORT
#define JNIEXPORT
#endif
#ifndef JNICALL
#define JNICALL
#endif
/* end of eclipse-mandated pimple */

int JAVA_HOST_LEVEL = -1;

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(java);

JavaVM *__java_vm = nullptr;

JNIEnv *get_current_thread_env()
{
  using simgrid::kernel::context::JavaContext;
  const JavaContext* ctx = static_cast<JavaContext*>(simgrid::kernel::context::Context::self());
  if (ctx)
    return ctx->jenv_;
  else
    return nullptr;
}

void jmsg_throw_status(JNIEnv *env, msg_error_t status) {
  switch (status) {
    case MSG_TIMEOUT:
      jxbt_throw_time_out_failure(env, "");
      break;
    case MSG_TRANSFER_FAILURE:
      jxbt_throw_transfer_failure(env, "");
      break;
    case MSG_HOST_FAILURE:
      jxbt_throw_host_failure(env, "");
      break;
    case MSG_TASK_CANCELED:
      jxbt_throw_task_cancelled(env, "");
      break;
    default:
      xbt_die("undefined message failed (please see jmsg_throw_status function in jmsg.cpp)");
  }
}

/***************************************************************************************
 * Unsortable functions                                                        *
 ***************************************************************************************/

JNIEXPORT jdouble JNICALL Java_org_simgrid_msg_Msg_getClock(JNIEnv*, jclass)
{
  return (jdouble)simgrid_get_clock();
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_init(JNIEnv* env, jclass, jobjectArray jargs)
{
  env->GetJavaVM(&__java_vm);

  simgrid::kernel::context::factory_initializer = &simgrid::kernel::context::java_factory;
  const _jthrowable* exc                        = env->ExceptionOccurred();
  if (exc) {
    env->ExceptionClear();
  }

  setlocale(LC_NUMERIC,"C");

  int argc = 1;
  if (jargs)
    argc += static_cast<int>(env->GetArrayLength(jargs));
  xbt_assert(argc > 0);

  // Need a static storage because the XBT layer saves the arguments in xbt::binary_name and xbt::cmdline.
  static std::vector<std::string> args;
  args.reserve(argc);

  args.emplace_back("java");
  for (int index = 1; index < argc; index++) {
    jstring jval    = (jstring)env->GetObjectArrayElement(jargs, index - 1);
    const char* tmp = env->GetStringUTFChars(jval, 0);
    args.emplace_back(tmp);
    env->ReleaseStringUTFChars(jval, tmp);
  }

  std::unique_ptr<char* []> argv(new char*[argc + 1]);
  std::transform(begin(args), end(args), argv.get(), [](std::string& s) { return &s.front(); });
  argv[argc] = nullptr;

  int argc2 = argc;
  MSG_init(&argc2, argv.get());
  xbt_assert(argc2 <= argc);

  for (int index = 1; index < argc2; index++)
    env->SetObjectArrayElement(jargs, index - 1, (jstring)env->NewStringUTF(argv[index]));

  sg_vm_live_migration_plugin_init();
  JAVA_HOST_LEVEL = simgrid::s4u::Host::extension_create(nullptr);
}

JNIEXPORT void JNICALL JNICALL Java_org_simgrid_msg_Msg_run(JNIEnv* env, jclass)
{
  /* Run everything */
  XBT_DEBUG("Ready to run");
  simgrid_run();
  XBT_DEBUG("Done running");
  XBT_INFO("Terminating the simulation...");
  /* Cleanup java hosts */
  sg_host_t* hosts  = sg_host_list();
  size_t host_count = sg_host_count();
  for (size_t index = 0; index < host_count - 1; index++) {
    jobject jhost = (jobject)hosts[index]->extension(JAVA_HOST_LEVEL);
    if (jhost)
      jhost_unref(env, jhost);
  }
  xbt_free(hosts);

  /* Cleanup java storages */
  for (auto const& elm : java_storage_map)
    jstorage_unref(env, elm.second);

  /* Display the status of remaining threads. None should survive, but who knows */
  jclass clsProcess = jxbt_get_class(env, "org/simgrid/msg/Process");
  jmethodID idDebug = jxbt_get_static_jmethod(env, clsProcess, "debugAllThreads", "()V");
  xbt_assert(idDebug != nullptr, "Method Process.debugAllThreads() not found...");
  env->CallStaticVoidMethod(clsProcess, idDebug);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_createEnvironment(JNIEnv* env, jclass, jstring jplatformFile)
{
  const char *platformFile = env->GetStringUTFChars(jplatformFile, 0);

  simgrid_load_platform(platformFile);

  env->ReleaseStringUTFChars(jplatformFile, platformFile);
}

JNIEXPORT jobject JNICALL Java_org_simgrid_msg_Msg_environmentGetRoutingRoot(JNIEnv* env, jclass)
{
  sg_netzone_t as  = sg_zone_get_root();
  jobject jas      = jnetzone_new_instance(env);
  if (not jas) {
    jxbt_throw_jni(env, "java As instantiation failed");
    return nullptr;
  }
  jas = jnetzone_ref(env, jas);
  if (not jas) {
    jxbt_throw_jni(env, "new global ref allocation failed");
    return nullptr;
  }
  jnetzone_bind(jas, as, env);

  return (jobject) jas;
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_debug(JNIEnv* env, jclass, jstring js)
{
  const char *s = env->GetStringUTFChars(js, 0);
  XBT_DEBUG("%s", s);
  env->ReleaseStringUTFChars(js, s);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_verb(JNIEnv* env, jclass, jstring js)
{
  const char *s = env->GetStringUTFChars(js, 0);
  XBT_VERB("%s", s);
  env->ReleaseStringUTFChars(js, s);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_info(JNIEnv* env, jclass, jstring js)
{
  const char *s = env->GetStringUTFChars(js, 0);
  XBT_INFO("%s", s);
  env->ReleaseStringUTFChars(js, s);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_warn(JNIEnv* env, jclass, jstring js)
{
  const char *s = env->GetStringUTFChars(js, 0);
  XBT_WARN("%s", s);
  env->ReleaseStringUTFChars(js, s);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_error(JNIEnv* env, jclass, jstring js)
{
  const char *s = env->GetStringUTFChars(js, 0);
  XBT_ERROR("%s", s);
  env->ReleaseStringUTFChars(js, s);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_critical(JNIEnv* env, jclass, jstring js)
{
  const char *s = env->GetStringUTFChars(js, 0);
  XBT_CRITICAL("%s", s);
  env->ReleaseStringUTFChars(js, s);
}

static void java_main(int argc, char* argv[]);

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_deployApplication(JNIEnv* env, jclass, jstring jdeploymentFile)
{
  const char *deploymentFile = env->GetStringUTFChars(jdeploymentFile, 0);

  simgrid_register_default(java_main);
  simgrid_load_deployment(deploymentFile);
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_energyInit() {
  sg_host_energy_plugin_init();
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_fileSystemInit()
{
  sg_storage_file_system_init();
}

JNIEXPORT void JNICALL Java_org_simgrid_msg_Msg_loadInit() {
    sg_host_load_plugin_init();
}
/** Run a Java org.simgrid.msg.Process
 *
 *  If needed, this waits for the process starting time.
 *  Then it calls the Process.run() method.
 */
static void run_jprocess(JNIEnv *env, jobject jprocess)
{
  // wait for the process's start time
  jfieldID jprocess_field_Process_startTime = jxbt_get_sfield(env, "org/simgrid/msg/Process", "startTime", "D");
  jdouble startTime = env->GetDoubleField(jprocess, jprocess_field_Process_startTime);
  if (startTime > simgrid_get_clock())
    simgrid::s4u::this_actor::sleep_for(startTime - simgrid_get_clock());

  //Execution of the "run" method.
  jmethodID id = jxbt_get_smethod(env, "org/simgrid/msg/Process", "run", "()V");
  xbt_assert((id != nullptr), "Method Process.run() not found...");

  env->CallVoidMethod(jprocess, id);
  if (env->ExceptionOccurred()) {
    XBT_DEBUG("Something went wrong in this Java actor, forget about it.");
    env->ExceptionClear();
    XBT_ATTRIB_UNUSED jint error = __java_vm->DetachCurrentThread();
    xbt_assert(error == JNI_OK, "Cannot detach failing thread");
    simgrid::ForcefulKillException::do_throw();
  }
}

/** Create a Java org.simgrid.msg.Process with the arguments and run it */
static void java_main(int argc, char* argv[])
{
  JNIEnv *env = get_current_thread_env();
  simgrid::kernel::context::JavaContext* context =
      static_cast<simgrid::kernel::context::JavaContext*>(simgrid::kernel::context::Context::self());

  //Change the "." in class name for "/".
  std::string arg0 = argv[0];
  std::replace(begin(arg0), end(arg0), '.', '/');
  jclass class_Process = env->FindClass(arg0.c_str());
  //Retrieve the methodID for the constructor
  xbt_assert((class_Process != nullptr), "Class not found (%s). The deployment file must use the fully qualified class name, including the package. The case is important.", argv[0]);
  jmethodID constructor_Process = env->GetMethodID(class_Process, "<init>", "(Lorg/simgrid/msg/Host;Ljava/lang/String;[Ljava/lang/String;)V");
  xbt_assert((constructor_Process != nullptr), "Constructor not found for class %s. Is there a (Host, String ,String[]) constructor in your class ?", argv[0]);

  //Retrieve the name of the process.
  jstring jname = env->NewStringUTF(argv[0]);
  //Build the arguments
  jobjectArray args = static_cast<jobjectArray>(env->NewObjectArray(argc - 1, env->FindClass("java/lang/String"),
                                                                    env->NewStringUTF("")));
  for (int i = 1; i < argc; i++)
      env->SetObjectArrayElement(args,i - 1, env->NewStringUTF(argv[i]));
  //Retrieve the host for the process.
  jstring jhostName = env->NewStringUTF(simgrid::s4u::Host::current()->get_cname());
  jobject jhost = Java_org_simgrid_msg_Host_getByName(env, nullptr, jhostName);
  //creates the process
  jobject jprocess = env->NewObject(class_Process, constructor_Process, jhost, jname, args);
  xbt_assert((jprocess != nullptr), "Process allocation failed.");
  jprocess = env->NewGlobalRef(jprocess);
  //bind the process to the context
  const_sg_actor_t actor = sg_actor_self();

  context->jprocess_ = jprocess;
  /* sets the PID and the PPID of the process */
  env->SetIntField(jprocess, jprocess_field_Process_pid, static_cast<jint>(actor->get_pid()));
  env->SetIntField(jprocess, jprocess_field_Process_ppid, static_cast<jint>(actor->get_ppid()));
  jprocess_bind(jprocess, actor, env);

  run_jprocess(env, context->jprocess_);
}

namespace simgrid {
namespace kernel {
namespace context {

/** Run the Java org.simgrid.msg.Process */
void java_main_jprocess(jobject jprocess)
{
  JNIEnv *env = get_current_thread_env();
  JavaContext* context = static_cast<JavaContext*>(Context::self());
  context->jprocess_   = jprocess;
  jprocess_bind(context->jprocess_, sg_actor_self(), env);

  run_jprocess(env, context->jprocess_);
}
} // namespace context
} // namespace kernel
} // namespace simgrid
