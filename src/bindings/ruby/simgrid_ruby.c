/* SimGrid Ruby bindings                                                    */

/* Copyright (c) 2010, the SimGrid team. All right reserved.                */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt.h"
#include "bindings/ruby_bindings.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(ruby);

// MSG Module
VALUE rb_msg;
// MSG Classes
VALUE rb_task;
VALUE rb_host;

//Init Msg  From Ruby
static void msg_init(VALUE Class,VALUE args)
{
  char **argv=NULL;
  const char *tmp;
  int argc,type,i;
  VALUE *ptr ;
  // Testing The Args Type
  type =  TYPE(args);
  if (type != T_ARRAY ) {
    rb_raise(rb_eRuntimeError,"Bad arguments to msg_init (expecting an array)");
    return;
  }
  ptr= RARRAY(args)->ptr;
  argc= RARRAY(args)->len;
  //  Create C Array to Hold Data_Get_Struct
  argc++;
  argv = xbt_new0(char *, argc);
  argv[0] = strdup("ruby");
  for (i=0;i<argc-1;i++) {
    VALUE value = ptr[i];
    type = TYPE(value);
    //  if (type == T_STRING)
    tmp = RSTRING(value)->ptr;
    argv[i+1] = strdup(tmp);
  }
  // Calling C Msg_Init Method
  MSG_global_init(&argc,argv);

  // Cleanups
  for (i=0;i<argc;i++)
    free(argv[i]) ;
  free (argv);
}
//Init Msg_Run From Ruby
static void msg_run(VALUE class) {
  DEBUG0("Start Running...");
  m_host_t *hosts;
  int cpt,host_count;
  VALUE rbHost;
  // Let's Run
  //printf("msg_run3\n");
  if (MSG_OK != MSG_main()){
    rb_raise(rb_eRuntimeError,"MSG_main() failed");
  }

  DEBUG0
  ("MSG_main finished. Bail out before cleanup since there is a bug in this part.");
  /* Cleanup Ruby hosts */
  DEBUG0("Clean Ruby World (TODO) ");
  hosts = MSG_get_host_table();
  host_count = MSG_get_host_number();
  for (cpt=0;cpt<host_count;cpt++) {
    rbHost = (VALUE)((hosts[cpt])->data);
  }
  return;
}

static void msg_clean(VALUE class)
{
   if (MSG_OK != MSG_clean())
    rb_raise(rb_eRuntimeError,"MSG_clean() failed");
  
}
static void msg_createEnvironment(VALUE class,VALUE plateformFile) {

  int type = TYPE(plateformFile);
  if ( type != T_STRING )
    rb_raise(rb_eRuntimeError,"Bad Argument's Type");
  const char * platform =  RSTRING(plateformFile)->ptr;
  MSG_create_environment(platform);
  DEBUG1("Create Environment (%s)...Done",platform);
}

//deploy Application
static void msg_deployApplication(VALUE class,VALUE deploymentFile ) {

  int type = TYPE(deploymentFile);
  if ( type != T_STRING )
    rb_raise(rb_eRuntimeError,"Bad Argument's Type for deployApplication ");
  const char *dep_file = RSTRING(deploymentFile)->ptr;
  surf_parse_reset_parser();
  surfxml_add_callback(STag_surfxml_process_cb_list,
      rb_application_handler_on_begin_process);
  surfxml_add_callback(ETag_surfxml_argument_cb_list,
      rb_application_handler_on_process_arg);

  surfxml_add_callback(STag_surfxml_prop_cb_list,
      rb_application_handler_on_property);

  surfxml_add_callback(ETag_surfxml_process_cb_list,
      rb_application_handler_on_end_process);

  surf_parse_open(dep_file);
  rb_application_handler_on_start_document();
  if(surf_parse())
    rb_raise(rb_eRuntimeError,"surf_parse() failed");
  surf_parse_close();
  
  rb_application_handler_on_end_document();

  DEBUG1("Deploy Application(%s)...Done",dep_file);
}

// INFO
static void msg_info(VALUE class,VALUE msg) {
  const char *s = RSTRING(msg)->ptr;
  INFO1("%s",s);
}
static void msg_debug(VALUE class,VALUE msg) {
  const char *s = RSTRING(msg)->ptr;
  DEBUG1("%s",s);
}

// get Clock
static VALUE msg_get_clock(VALUE class) {

  return rb_float_new(MSG_get_clock());

}


extern const char*xbt_ctx_factory_to_use; /*Hack: let msg load directly the right factory */

typedef VALUE(*rb_meth)(ANYARGS);
void Init_simgrid_ruby() {
  xbt_ctx_factory_to_use = "ruby";

  // Modules
  rb_msg = rb_define_module("MSG");
  //Associated Environment Methods
  rb_define_module_function(rb_msg,"init",(rb_meth)msg_init,1);
  rb_define_module_function(rb_msg,"run",(rb_meth)msg_run,0);
  rb_define_module_function(rb_msg,"createEnvironment",(rb_meth)msg_createEnvironment,1);
  rb_define_module_function(rb_msg,"deployApplication",(rb_meth)msg_deployApplication,1);
  rb_define_module_function(rb_msg,"info",(rb_meth)msg_info,1);
  rb_define_module_function(rb_msg,"debug",(rb_meth)msg_debug,1);
  rb_define_module_function(rb_msg,"getClock",(rb_meth)msg_get_clock,0);
  rb_define_module_function(rb_msg,"exit",(rb_meth)msg_clean,0);

  //Associated Process Methods
  rb_define_method(rb_msg,"processSuspend",(rb_meth)rb_process_suspend,1);
  rb_define_method(rb_msg,"processResume",(rb_meth)rb_process_resume,1);
  rb_define_method(rb_msg,"processIsSuspend",(rb_meth)rb_process_isSuspended,1);
  rb_define_method(rb_msg,"processKill",(rb_meth)rb_process_kill_up,1);
  rb_define_method(rb_msg,"processKillDown",(rb_meth)rb_process_kill_down,1);
  rb_define_method(rb_msg,"processGetHost",(rb_meth)rb_process_getHost,1);
  rb_define_method(rb_msg,"processExit",(rb_meth)rb_process_exit,1);

  //Classes
  rb_task = rb_define_class_under(rb_msg,"RbTask",rb_cObject);
  rb_host = rb_define_class_under(rb_msg,"RbHost",rb_cObject);

  //Task Methods 
  rb_define_module_function(rb_task,"new",(rb_meth)rb_task_new,3);
  rb_define_module_function(rb_task,"compSize",(rb_meth)rb_task_comp,1);
  rb_define_module_function(rb_task,"name",(rb_meth)rb_task_name,1);
  rb_define_module_function(rb_task,"execute",(rb_meth)rb_task_execute,1);
  rb_define_module_function(rb_task,"send",(rb_meth)rb_task_send,2);
  rb_define_module_function(rb_task,"receive",(rb_meth)rb_task_receive,1);
  rb_define_module_function(rb_task,"sender",(rb_meth)rb_task_sender,1);
  rb_define_module_function(rb_task,"source",(rb_meth)rb_task_source,1);
  rb_define_module_function(rb_task,"listen",(rb_meth)rb_task_listen,2);
  rb_define_module_function(rb_task,"listenFromHost",(rb_meth)rb_task_listen_host,3);

  //Host Methods
  rb_define_module_function(rb_host,"getByName",(rb_meth)rb_host_get_by_name,1);
  rb_define_module_function(rb_host,"name",(rb_meth)rb_host_name,1);
  rb_define_module_function(rb_host,"speed",(rb_meth)rb_host_speed,1);
  rb_define_module_function(rb_host,"number",(rb_meth)rb_host_number,0);
  rb_define_module_function(rb_host,"setData",(rb_meth)rb_host_set_data,2);
  rb_define_module_function(rb_host,"getData",(rb_meth)rb_host_get_data,1);
  rb_define_module_function(rb_host,"isAvail",(rb_meth)rb_host_is_avail,1);
}