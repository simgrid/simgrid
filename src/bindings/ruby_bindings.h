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

//#include "msg/private.h"
//#include "msg/mailbox.h"
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
void  rb_process_kill(VALUE ruby_process);
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
void rb_processBind(VALUE ruby_class,m_process_t process);

// CreateProcess Method
void rb_processCreate(VALUE Class,VALUE rb_process,VALUE host);

// ProcessSuspend
void rb_processSuspend(VALUE Class,VALUE ruby_process);

// ProcessResume
void rb_processResume(VALUE Class,VALUE ruby_process);

//ProcessIsSuspend return Boolean ( Qtrue / Qfalse )
VALUE rb_processIsSuspend(VALUE Class,VALUE ruby_process);

//Processkill
void rb_processKill(VALUE Class,VALUE ruby_process);

//ProcessGetHost
VALUE rb_processGetHost(VALUE Class,VALUE ruby_process);

//ProcessExit
void rb_processExit(VALUE Class,VALUE ruby_process);

#endif /* RB_SG_BINDINGS */
