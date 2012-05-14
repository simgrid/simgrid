/* Java Wrappers to the MSG API.                                            */

/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include <msg/msg.h>
#include <simgrid/simix.h>
#include <surf/surfxml_parse.h>
#include <locale.h>

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
Java_org_simgrid_msg_Msg_info(JNIEnv * env, jclass cls, jstring js)
{
  const char *s = (*env)->GetStringUTFChars(env, js, 0);
  XBT_INFO("%s", s);
  (*env)->ReleaseStringUTFChars(env, js, s);
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
