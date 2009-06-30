#include "jmsg_application_handler.h"

#include "jmsg.h"
  
#include "surf/surfxml_parse.h"
#include "jxbt_utilities.h"
void japplication_handler_on_start_document(void) 
{
  jclass cls;
  JNIEnv * env = get_current_thread_env();
  jmethodID id =
    jxbt_get_static_smethod(env, "simgrid/msg/ApplicationHandler",
                             "onStartDocument", "()V");
  if (!id
         || !(cls = jxbt_get_class(env, "simgrid/msg/ApplicationHandler")))
    return;
  (*env)->CallStaticVoidMethod(env, cls, id);
}

void japplication_handler_on_end_document(void) 
{
  jclass cls;
  JNIEnv * env = get_current_thread_env();
  jmethodID id =
    jxbt_get_static_smethod(env, "simgrid/msg/ApplicationHandler",
                             "onEndDocument", "()V");
  if (!id
         || !(cls = jxbt_get_class(env, "simgrid/msg/ApplicationHandler")))
    return;
  (*env)->CallStaticVoidMethod(env, cls, id);
}

void japplication_handler_on_begin_process(void) 
{
  jstring jhostName;
  jstring jfunction;
  jclass cls;
  JNIEnv * env = get_current_thread_env();
  jmethodID id =
    jxbt_get_static_smethod(env, "simgrid/msg/ApplicationHandler",
                             "onBeginProcess",
                             "(Ljava/lang/String;Ljava/lang/String;)V");
  if (!id
         || !(cls = jxbt_get_class(env, "simgrid/msg/ApplicationHandler")))
    return;
  jhostName = (jstring) (*env)->NewStringUTF(env, A_surfxml_process_host);
  jfunction = 
    (jstring) (*env)->NewStringUTF(env, A_surfxml_process_function);
  (*env)->CallStaticVoidMethod(env, cls, id, jhostName, jfunction);
}

void japplication_handler_on_process_arg(void) 
{
  jstring jarg;
  jclass cls;
  JNIEnv * env = get_current_thread_env();
  jmethodID id =
    jxbt_get_static_smethod(env, "simgrid/msg/ApplicationHandler",
                             "onProcessArg", "(Ljava/lang/String;)V");
  if (!id
         || !(cls = jxbt_get_class(env, "simgrid/msg/ApplicationHandler")))
    return;
  jarg = (jstring) (*env)->NewStringUTF(env, A_surfxml_argument_value);
  (*env)->CallStaticVoidMethod(env, cls, id, jarg);
}

void japplication_handler_on_property(void) 
{
  jstring jid;
  jstring jvalue;
  jclass cls;
  JNIEnv * env = get_current_thread_env();
  jmethodID id =
    jxbt_get_static_smethod(env, "simgrid/msg/ApplicationHandler",
                             "onProperty",
                             "(Ljava/lang/String;Ljava/lang/String;)V");
  if (!id
         || !(cls = jxbt_get_class(env, "simgrid/msg/ApplicationHandler")))
    return;
  jid = (jstring) (*env)->NewStringUTF(env, A_surfxml_prop_id);
  jvalue = (jstring) (*env)->NewStringUTF(env, A_surfxml_prop_value);
  (*env)->CallStaticVoidMethod(env, cls, id, jid, jvalue);
}

void japplication_handler_on_end_process(void) 
{
  JNIEnv * env = get_current_thread_env();
  jclass cls;
  jmethodID id =
    jxbt_get_static_smethod(env, "simgrid/msg/ApplicationHandler",
                             "onEndProcess", "()V");
  if (!id
         || !(cls = jxbt_get_class(env, "simgrid/msg/ApplicationHandler")))
    return;
  (*env)->CallStaticVoidMethod(env, cls, id);
}


