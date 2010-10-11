/* SimGrid -- Ruby bindings */

/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#ifndef RB_SG_BINDINGS
#define RB_SG_BINDINGS
/*
 * There is 4 sections in this file:
 *  - Header loading (ruby makes it quite difficult, damn it)
 *  - definitions of ruby contextes for use in simix
 *  - Functions to Manage The Ruby Process  (named Up Calls)
 *  - Functions to Manage The Native Process Simulation Bound (named Down Calls)
 */

#include "msg/msg.h"
#include "msg/datatypes.h"

#include "surf/surfxml_parse.h"
#include "simix/simix.h"
#include "simix/private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/asserts.h"

#include "simix/smx_context_private.h"

/* Damn Ruby. They load their full config.h, which breaks since we also load ours.
 * So, we undef the offending defines
 */
#undef PACKAGE_VERSION
#undef PACKAGE_NAME
#undef PACKAGE_TARNAME
#undef PACKAGE_STRING
#undef PACKAGE_BUGREPORT
#undef _GNU_SOURCE
#include <ruby.h>

/* ********************* *
 * Context related stuff *
 * ********************* */
typedef struct s_smx_ctx_ruby {
  s_smx_ctx_base_t super;       /* Fields of super implementation */
  VALUE process;                // The  Ruby Process Instance
  //...
} s_smx_ctx_ruby_t, *smx_ctx_ruby_t;
void SIMIX_ctx_ruby_factory_init(smx_context_factory_t * factory);

void Init_libsimgrid(void);     /* Load the bindings */
void initRuby(void);            // Mandatory to call Ruby methods from C

/* *********************************************** *
 * Functions for Ruby Process Management (Up Call) *
 *                                                 *
 * Independent Methods                             *
 * *********************************************** */

VALUE rb_process_getName(VALUE ruby_process);
VALUE rb_process_getID(VALUE ruby_process);
VALUE rb_process_getBind(VALUE ruby_class);
void rb_process_setBind(VALUE ruby_class, long bind);
VALUE rb_process_isAlive(VALUE ruby_process);
void rb_process_kill_up(VALUE ruby_process);
void rb_process_join(VALUE ruby_process);
void rb_process_unschedule(VALUE ruby_process);
void rb_process_schedule(VALUE ruby_process);

/* ********************************************** *
 * Function for Native Process (Bound) Management *
 *                                                *
 * Methods Belonging to The MSG Module            *
 * ********************************************** */

// ProcessBind Method ; Process Ruby >> Process C

//friend Method // Not belong to the Class but Called within !!
m_process_t rb_process_to_native(VALUE ruby_process);
// Binding Process >> Friend Method
void rb_process_bind(VALUE ruby_class, m_process_t process);
void rb_process_create(VALUE Class, VALUE rb_process, VALUE host);
void rb_process_suspend(VALUE Class, VALUE ruby_process);
void rb_process_resume(VALUE Class, VALUE ruby_process);
// Returns Boolean ( Qtrue / Qfalse )
VALUE rb_process_isSuspended(VALUE Class, VALUE ruby_process);
void rb_process_kill_down(VALUE Class, VALUE ruby_process);
VALUE rb_process_getHost(VALUE Class, VALUE ruby_process);
void rb_process_exit(VALUE Class, VALUE ruby_process);

/* Functions related to hosts */
void rb_host_free(m_host_t ht);
VALUE rb_host_get_by_name(VALUE Class, VALUE name);
VALUE rb_host_name(VALUE Class, VALUE host);
VALUE rb_host_number(VALUE Class);
VALUE rb_host_speed(VALUE Class, VALUE host);
void rb_host_set_data(VALUE Class, VALUE host, VALUE data);
VALUE rb_host_get_data(VALUE Class, VALUE host);
VALUE rb_host_is_avail(VALUE Class, VALUE host);
VALUE rb_host_process(VALUE Class, VALUE process);
VALUE rb_host_get_all_hosts(VALUE Class);

/* Functions related to tasks */

typedef struct ruby_data {
  void *ruby_task;              // Pointer to send the ruby_task
  void *user_data;              // Pointer on the user data
} s_ruby_data_t, *rb_data_t;

void rb_task_free(m_task_t tk);
VALUE rb_task_new(VALUE Class, VALUE name, VALUE comp_size,
                  VALUE comm_size);
VALUE rb_task_comp(VALUE Class, VALUE task);    // Get Computation Size
VALUE rb_task_name(VALUE Class, VALUE task);
VALUE rb_task_execute(VALUE Class, VALUE task);
void rb_task_send(VALUE Class, VALUE task, VALUE mailbox);
VALUE rb_task_receive(VALUE Class, VALUE mailbox);      // Receive : return a task
void rb_task_receive2(VALUE Class, VALUE task, VALUE mailbox);  // Receive Task 2 <<  Not Appreciated
VALUE rb_task_sender(VALUE Class, VALUE task);
VALUE rb_task_source(VALUE Class, VALUE task);
VALUE rb_task_listen(VALUE Class, VALUE task, VALUE alias);     //Listen From Alias (=mailbox)
VALUE rb_task_listen_host(VALUE Class, VALUE task, VALUE alias, VALUE host);    //Listen from Host
void rb_task_set_priority(VALUE Class, VALUE task, VALUE priority);     // Set Priority
void rb_task_cancel(VALUE Class, VALUE task);   // Cancel
VALUE rb_task_has_data(VALUE Class, VALUE task);        // check if the task contains a data
VALUE rb_task_get_data(VALUE Class, VALUE task);        // get data
void rb_task_set_data(VALUE Class, VALUE task, VALUE data);     // set data

/* Upcalls for the application handler */
void rb_application_handler_on_start_document(void);
void rb_application_handler_on_end_document(void);
void rb_application_handler_on_begin_process(void);
void rb_application_handler_on_process_arg(void);
void rb_application_handler_on_property(void);
void rb_application_handler_on_end_process(void);

#endif                          /* RB_SG_BINDINGS */
