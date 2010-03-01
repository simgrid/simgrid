/* SimGrid -- Ruby bindings */

/* Copyright (c) 2010, the SimGrid team. All right reserved */

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

#include "msg/mailbox.h" /* MAX_ALIAS_NAME (FIXME: kill it)*/
#include "surf/surfxml_parse.h"
#include "simix/simix.h"
#include "simix/private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/asserts.h"
//#include "rb_msg_host.h"

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
  SMX_CTX_BASE_T;
  VALUE process;   // The  Ruby Process Instance
  //...
}s_smx_ctx_ruby_t,*smx_ctx_ruby_t;
void SIMIX_ctx_ruby_factory_init(smx_context_factory_t *factory);


void initRuby(void); // Mandatory to call Ruby methods from C

/* *********************************************** *
 * Functions for Ruby Process Management (Up Call) *
 *                                                 *
 * Independent Methods                             *
 * *********************************************** */

VALUE rb_process_getName( VALUE ruby_process );
VALUE rb_process_getID(VALUE ruby_process);
VALUE rb_process_getBind(VALUE ruby_class);
void  rb_process_setBind(VALUE ruby_class,long bind);
VALUE rb_process_isAlive(VALUE ruby_process);
void  rb_process_kill_up(VALUE ruby_process);
void  rb_process_join( VALUE ruby_process );
void  rb_process_unschedule( VALUE ruby_process );
void  rb_process_schedule( VALUE ruby_process );


/* ********************************************** *
 * Function for Native Process (Bound) Management *
 *                                                *
 * Methods Belonging to The MSG Module            *
 * ********************************************** */

// ProcessBind Method ; Process Ruby >> Process C

//friend Method // Not belong to the Class but Called within !!
m_process_t rb_process_to_native(VALUE ruby_process);
// Binding Process >> Friend Method
void rb_process_bind(VALUE ruby_class,m_process_t process);
void rb_process_create(VALUE Class,VALUE rb_process,VALUE host);
void rb_process_suspend(VALUE Class,VALUE ruby_process);
void rb_process_resume(VALUE Class,VALUE ruby_process);
// Returns Boolean ( Qtrue / Qfalse )
VALUE rb_process_isSuspended(VALUE Class,VALUE ruby_process);
void rb_process_kill_down(VALUE Class,VALUE ruby_process);
VALUE rb_process_getHost(VALUE Class,VALUE ruby_process);
void rb_process_exit(VALUE Class,VALUE ruby_process);

/* Functions related to hosts */
void  rb_host_free(m_host_t ht);
VALUE rb_host_get_by_name(VALUE Class, VALUE name);
VALUE rb_host_name(VALUE Class,VALUE host);
VALUE rb_host_number(VALUE Class);
VALUE rb_host_speed(VALUE Class,VALUE host);
void  rb_host_set_data(VALUE Class,VALUE host,VALUE data);
VALUE rb_host_get_data(VALUE Class,VALUE host);
VALUE rb_host_is_avail(VALUE Class,VALUE host);

/* Functions related to tasks */
void rb_task_free(m_task_t tk);
// New Method  >>> Data NULL
VALUE rb_task_new(VALUE Class, VALUE name,VALUE comp_size,VALUE comm_size);
VALUE rb_task_comp(VALUE Class,VALUE task); // Get Computation Size
VALUE rb_task_name(VALUE Class,VALUE task);
VALUE rb_task_execute(VALUE Class,VALUE task);
void  rb_task_send(VALUE Class,VALUE task,VALUE mailbox);
VALUE rb_task_receive(VALUE Class,VALUE mailbox);// Receive : return a task
void  rb_task_receive2(VALUE Class,VALUE task,VALUE mailbox);// Receive Task 2 <<  Not Appreciated
VALUE rb_task_sender(VALUE Class,VALUE task);
VALUE rb_task_source(VALUE Class,VALUE task);
VALUE rb_task_listen(VALUE Class,VALUE task,VALUE alias); //Listen From Alias (=mailbox)
VALUE rb_task_listen_host(VALUE Class,VALUE task,VALUE alias,VALUE host); //Listen from Host

#endif /* RB_SG_BINDINGS */
