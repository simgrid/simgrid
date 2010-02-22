#ifndef RB_MSG
#define RB_MSG
#include <stdio.h>
#include "msg/msg.h"
#include <ruby.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
                             "Messages specific for this msg example");

// #include "msg/private.h"
// #include "simix/private.h"
// #include "simix/smx_context_ruby.h"

// MSG Module
VALUE rb_msg;
// MSG Classes
VALUE rb_task;
VALUE rb_host;

//init_Msg Called When The Ruby Interpreter loads this C extension
void Init_msg();

// Msg_Init From Ruby
static void msg_init(VALUE Class,VALUE args);

// Msg_Run From Ruby
static void msg_run(VALUE Class);

// Create Environment
static void msg_createEnvironment(VALUE Class,VALUE plateformFile);

// deploy Application
static void msg_deployApplication(VALUE Class,VALUE deploymntFile);

// Tools
static void msg_info(VALUE Class,VALUE msg);

//get Clock  
static void msg_get_clock(VALUE Class);

//pajeOutput
static void msg_paje_output(VALUE Class,VALUE pajeFile);

// Ruby Introspection : To instanciate a Ruby Class from its Name
static VALUE msg_new_ruby_instance(VALUE Class,VALUE className);

// The Same ... This Time with Args
static VALUE msg_new_ruby_instance_with_args(VALUE Class,VALUE className,VALUE args);

#endif